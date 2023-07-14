#ifndef HOSTADDRCONFIGDIALOG_H
#define HOSTADDRCONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class HostAddrConfigDialog;
}

class HostAddrConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit HostAddrConfigDialog(QWidget *parent = nullptr);
  ~HostAddrConfigDialog();
  void setCurrentAddr(const QString &addr);
  QString getAddr();

private:
  Ui::HostAddrConfigDialog *ui;
};

#endif // HOSTADDRCONFIGDIALOG_H
