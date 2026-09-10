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
  double value() const;
  void setValue(double value);
  void setRange(double min, double max);
  void setPresets(const QList<double> &presetList, int prec = 1);

public slots:
  void accept() override;

private:
  Ui::ValueSetDialog *ui;
  double _value = 0.0;
  double _min = 0.0;
  double _max = 0.0;
  QString _unitString;

  bool parseValue();

private slots:
  void editingFinished();
  void cbPresetsIndexChanged(int index);
};

#endif // VALUESETDIALOG_H
