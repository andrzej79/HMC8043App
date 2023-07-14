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
 * @brief ValueSetDialog::getValue
 * @return
 */
double ValueSetDialog::getValue() const
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
  for(auto &v : presetList) {
    ui->cbPresets->addItem(QString::number(v, 'f', prec) + " " + _unitString, v);
  }
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
  accept();
}
