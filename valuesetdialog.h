#ifndef VALUESETDIALOG_H
#define VALUESETDIALOG_H

#include <QDialog>

namespace Ui {
class ValueSetDialog;
}

class ValueSetDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ValueSetDialog(QWidget *parent = nullptr);
  ~ValueSetDialog();
  void setUnitString(const QString &name);
  double getValue() const;

private:
  Ui::ValueSetDialog *ui;
  double _value;
  QString _unitString;

  bool parseValue();

private slots:
  void editingFinished();
  void cbPresetsIndexChanged(int index);
};

#endif // VALUESETDIALOG_H
