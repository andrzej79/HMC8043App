#ifndef CHANNELSTATE_H
#define CHANNELSTATE_H

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

/* GUI-thread mirror of one output channel, as last reported by the driver.
 * QML binds to this and never touches HMCSupplyCtrl, which lives on its own thread. */
class ChannelState : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Provided by SupplyBackend.channels")
  Q_PROPERTY(int number READ number CONSTANT)
  Q_PROPERTY(bool hasVoltage READ hasVoltage NOTIFY measurementChanged)
  Q_PROPERTY(bool hasCurrent READ hasCurrent NOTIFY measurementChanged)
  Q_PROPERTY(bool hasPower READ hasPower NOTIFY measurementChanged)
  Q_PROPERTY(double voltage READ voltage NOTIFY measurementChanged)
  Q_PROPERTY(double current READ current NOTIFY measurementChanged)
  Q_PROPERTY(double power READ power NOTIFY measurementChanged)
  Q_PROPERTY(bool hasTargetVoltage READ hasTargetVoltage NOTIFY targetsChanged)
  Q_PROPERTY(bool hasTargetCurrent READ hasTargetCurrent NOTIFY targetsChanged)
  Q_PROPERTY(double targetVoltage READ targetVoltage NOTIFY targetsChanged)
  Q_PROPERTY(double targetCurrent READ targetCurrent NOTIFY targetsChanged)
  Q_PROPERTY(double maxVoltage READ maxVoltage NOTIFY limitsChanged)
  Q_PROPERTY(double maxCurrent READ maxCurrent NOTIFY limitsChanged)
  Q_PROPERTY(bool outputEnabled READ outputEnabled NOTIFY outputEnabledChanged)

public:
  explicit ChannelState(int number, QObject *parent = nullptr);

  int number() const { return _number; }
  bool hasVoltage() const { return _hasVoltage; }
  bool hasCurrent() const { return _hasCurrent; }
  bool hasPower() const { return _hasVoltage && _hasCurrent; }
  double voltage() const { return _voltage; }
  double current() const { return _current; }
  double power() const { return _voltage * _current; }
  bool hasTargetVoltage() const { return _hasTargetVoltage; }
  bool hasTargetCurrent() const { return _hasTargetCurrent; }
  double targetVoltage() const { return _targetVoltage; }
  double targetCurrent() const { return _targetCurrent; }
  double maxVoltage() const { return _maxVoltage; }
  double maxCurrent() const { return _maxCurrent; }
  bool outputEnabled() const { return _outputEnabled; }

  void setVoltage(double voltage);
  void setCurrent(double current);
  void setTargetVoltage(double voltage);
  void setTargetCurrent(double current);
  void setLimits(double maxVoltage, double maxCurrent);
  void setOutputEnabled(bool enabled);
  void reset();

signals:
  void measurementChanged();
  void targetsChanged();
  void limitsChanged();
  void outputEnabledChanged();

private:
  const int _number;
  bool _hasVoltage = false;
  bool _hasCurrent = false;
  double _voltage = 0.0;
  double _current = 0.0;
  bool _hasTargetVoltage = false;
  bool _hasTargetCurrent = false;
  double _targetVoltage = 0.0;
  double _targetCurrent = 0.0;
  double _maxVoltage;
  double _maxCurrent;
  bool _outputEnabled = false;
};

#endif // CHANNELSTATE_H
