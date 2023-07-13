#include "valuesetdialog.h"
#include "ui_valuesetdialog.h"

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
}

ValueSetDialog::~ValueSetDialog()
{
  delete ui;
}

void ValueSetDialog::setUnitString(const QString &name)
{
  _unitString = name;
  ui->lbValueUnits->setText(_unitString);
}

double ValueSetDialog::getValue() const
{
  return _value;
}

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

void ValueSetDialog::editingFinished()
{
  parseValue();
}

void ValueSetDialog::cbPresetsIndexChanged(int index)
{
  ui->edValue->setText(ui->cbPresets->currentText());
  parseValue();
}
