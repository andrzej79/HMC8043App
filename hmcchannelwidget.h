#ifndef HMCCHANNELWIDGET_H
#define HMCCHANNELWIDGET_H

#include <QWidget>
#include "hmcappglobal.h"
#include "hmcsupplyctrl.h"

namespace Ui {
class HMCChannelWidget;
}

class HMCChannelWidget : public QWidget
{
  Q_OBJECT

public:
  explicit HMCChannelWidget(QWidget *parent = nullptr);
  ~HMCChannelWidget();
  void setupWidget(HMCSupplyCtrl *hmcCtrl, HMCSupplyCtrl::HMCChannel channel, const QString &channelName);

private:
  Ui::HMCChannelWidget *ui;
  HMCSupplyCtrl *_hmcCtrl = nullptr;
  HMCSupplyCtrl::HMCChannel _channel = HMCSupplyCtrl::NoChannel;
  QString _channelName;
  static const QList<double> _voltagePresets;
  static const QList<double> _currentPresets;
  double _voltage = 0.0;
  double _current = 0.0;
  /* GUI-thread copies of the last values the controller reported. The controller
   * lives on its own thread, so reading its members directly would be a race. */
  double _targetVoltage = 0.0;
  double _targetCurrent = 0.0;
  /* Per-channel limits as reported by the instrument; the constants are only a
   * fallback for a supply that does not answer the MAX queries. */
  double _maxVoltage = FALLBACK_MAX_VOLTAGE;
  double _maxCurrent = FALLBACK_MAX_CURRENT;

  void createConnections();

private slots:
  void btnSetVoltageClicked();
  void btnSetCurrentClicked();
  void cbChannelOutEnableClicked();

  void deviceDisconnected();
  void channelVoltageChanged(HMCSupplyCtrl::HMCChannel chNr, double voltage);
  void channelCurrentChanged(HMCSupplyCtrl::HMCChannel chNr, double current);
  void channelOutEnableChanged(HMCSupplyCtrl::HMCChannel chNr, bool enabled);
  void channelTargetVoltageChanged(HMCSupplyCtrl::HMCChannel chNr, double voltage);
  void channelTargetCurrentChanged(HMCSupplyCtrl::HMCChannel chNr, double current);
  void channelLimitsChanged(HMCSupplyCtrl::HMCChannel chNr, double maxVoltage, double maxCurrent);

signals:
  void setChannelVoltage(HMCSupplyCtrl::HMCChannel chNr, double voltage);
  void setChannelCurrent(HMCSupplyCtrl::HMCChannel chNr, double current);
  void setChannelOutEnable(HMCSupplyCtrl::HMCChannel chNr, bool enable);
};

#endif // HMCCHANNELWIDGET_H
