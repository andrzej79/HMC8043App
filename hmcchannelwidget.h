#ifndef HMCCHANNELWIDGET_H
#define HMCCHANNELWIDGET_H

#include <QWidget>
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
  HMCSupplyCtrl::HMCChannel _channel;
  QString _channelName;

  void createConnections();

private slots:
  void btnSetVoltageClicked();
  void btnSetCurrentClicked();
  void cbChannelOutEnableClicked();
  void channelVoltageChanged(HMCSupplyCtrl::HMCChannel chNr, double voltage);
  void channelCurrentChanged(HMCSupplyCtrl::HMCChannel chNr, double current);

signals:
  void updateChannelVoltage(HMCSupplyCtrl::HMCChannel chNr);
  void updateChannelCurrent(HMCSupplyCtrl::HMCChannel chNr);
  void setChannelVoltage(HMCSupplyCtrl::HMCChannel chNr, double voltage);
  void setChannelCurrent(HMCSupplyCtrl::HMCChannel chNr, double current);
  void setChannelOutEnable(HMCSupplyCtrl::HMCChannel chNr, bool enable);
};

#endif // HMCCHANNELWIDGET_H
