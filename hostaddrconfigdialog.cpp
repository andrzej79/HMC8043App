#include "hostaddrconfigdialog.h"
#include "ui_hostaddrconfigdialog.h"

HostAddrConfigDialog::HostAddrConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HostAddrConfigDialog)
{
  ui->setupUi(this);
}

HostAddrConfigDialog::~HostAddrConfigDialog()
{
  delete ui;
}

void HostAddrConfigDialog::setCurrentAddr(const QString &addr)
{
  ui->lineEdit->setText(addr);
}

QString HostAddrConfigDialog::getAddr()
{
  return ui->lineEdit->text();
}
