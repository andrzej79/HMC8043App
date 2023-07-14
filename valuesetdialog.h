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
  void setValue(double value);
  void setPresets(const QList<double> &presetList, int prec = 1);

private:
  Ui::ValueSetDialog *ui;
  double _value;
  QString _unitString;
  QList<double> _presets;

  bool parseValue();

private slots:
  void editingFinished();
  void cbPresetsIndexChanged(int index);
};

#endif // VALUESETDIALOG_H
