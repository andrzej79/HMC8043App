#include <QDoubleValidator>
#include <QMessageBox>
#include <QTimer>
#include "valuesetdialog.h"
#include "ui_valuesetdialog.h"

/**
 * @brief ValueSetDialog::ValueSetDialog
 * @param parent
 */
ValueSetDialog::ValueSetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ValueSetDialog)
{
  ui->setupUi(this);
#if QT_VERSION >= 0x060000
  connect(ui->cbPresets, &QComboBox::currentIndexChanged, this, &ValueSetDialog::cbPresetsIndexChanged);
#else
  connect(ui->cbPresets, qOverload<int>(&QComboBox::currentIndexChanged), this, &ValueSetDialog::cbPresetsIndexChanged);
#endif
  connect(ui->edValue, &QLineEdit::editingFinished, this, &ValueSetDialog::editingFinished);

  QTimer::singleShot(100, this, [this]() {
     ui->edValue->selectAll();
  });
}

/**
 * @brief ValueSetDialog::~ValueSetDialog
 */
ValueSetDialog::~ValueSetDialog()
{
  delete ui;
}

/**
 * @brief ValueSetDialog::setUnitString
 * @param name
 */
void ValueSetDialog::setUnitString(const QString &name)
{
  _unitString = name;
  ui->lbValueUnits->setText(_unitString);
}

/**
 * @brief ValueSetDialog::value
 * @return
 */
double ValueSetDialog::value() const
{
  return _value;
}

/**
 * @brief ValueSetDialog::setValue
 * @param value
 */
void ValueSetDialog::setValue(double value)
{
  QSignalBlocker block(ui->edValue);
  _value = value;
  ui->edValue->setText(QString::number(value, 'f', 3));
}

/**
 * @brief ValueSetDialog::setPresets
 * @param presetList
 */
void ValueSetDialog::setPresets(const QList<double> &presetList, int prec)
{
  ui->cbPresets->clear();
  for(const auto &v : presetList) {
    ui->cbPresets->addItem(QString::number(v, 'f', prec) + " " + _unitString, v);
  }
}

/**
 * @brief ValueSetDialog::setRange
 * @param min
 * @param max
 *
 * Bounds come from the instrument (VOLT? MAX / CURR? MAX), so the dialog can refuse
 * an out-of-range entry up front instead of sending a command the supply rejects
 * into an error queue the application never reads.
 */
void ValueSetDialog::setRange(double min, double max)
{
  _min = min;
  _max = max;
  auto *validator = new QDoubleValidator(min, max, 3, this);
  validator->setNotation(QDoubleValidator::StandardNotation);
  validator->setLocale(QLocale::c());   /* parseValue() uses toDouble(), which is C-locale */
  ui->edValue->setValidator(validator);
  ui->lbValueUnits->setToolTip(QString("%1 ... %2 %3")
                                .arg(min, 0, 'f', 3).arg(max, 0, 'f', 3).arg(_unitString));
}

/**
 * @brief ValueSetDialog::parseValue
 * @return
 */
bool ValueSetDialog::parseValue()
{
  auto str = ui->edValue->text();
  bool ok;
  double v = str.toDouble(&ok);
  if(ok) {
    _value = v;
  }
  return ok;
}

/**
 * @brief ValueSetDialog::accept
 *
 * The OK button is wired straight to accept() in the .ui file, and parseValue()
 * only ran as a side effect of editingFinished(). Unparseable input therefore
 * left _value at whatever setValue() had seeded - the channel's current setpoint -
 * and the dialog accepted anyway, re-sending the old value as if it were new.
 * Validate here so the dialog cannot return a value the user did not enter.
 */
void ValueSetDialog::accept()
{
  if(!parseValue()) {
    QMessageBox::warning(this, "Invalid value",
                         "Please enter a valid number, using '.' as the decimal separator.");
    ui->edValue->selectAll();
    ui->edValue->setFocus();
    return;
  }
  if(_max > _min && (_value < _min || _value > _max)) {
    QMessageBox::warning(this, "Out of range",
                         QString("The instrument accepts %1 ... %2 %3.")
                           .arg(_min, 0, 'f', 3).arg(_max, 0, 'f', 3).arg(_unitString));
    ui->edValue->selectAll();
    ui->edValue->setFocus();
    return;
  }
  QDialog::accept();
}

/**
 * @brief ValueSetDialog::editingFinished
 */
void ValueSetDialog::editingFinished()
{
  parseValue();
}

/**
 * @brief ValueSetDialog::cbPresetsIndexChanged
 * @param index
 */
void ValueSetDialog::cbPresetsIndexChanged(int index)
{
  _value = ui->cbPresets->itemData(index).toDouble();
  QSignalBlocker block(ui->edValue);
  ui->edValue->setText(QString::number(_value, 'f', 3));
  QDialog::accept();
}
