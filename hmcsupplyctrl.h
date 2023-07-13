#ifndef HMCSUPPLYCTRL_H
#define HMCSUPPLYCTRL_H

#include <QObject>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <array>

#define HMC_SCPI_PORT 5025


class HMCSupplyCtrl : public QObject
{
  Q_OBJECT
public:
  enum HMCChannel {
    NoChannel = 0,
    Channel1  = 1,
    Channel2,
    Channel3
  };
  Q_ENUM(HMCChannel);
  #define HMCChannelCount 3
  static const std::array<HMCChannel, HMCChannelCount> hmcChannels;

  explicit HMCSupplyCtrl(QObject *parent = nullptr);
  ~HMCSupplyCtrl();
  double getChannelTargetVoltage(HMCChannel chNr) const;
  double getChannelTargetCurrent(HMCChannel chNr) const;
  bool isChannelEnabled(HMCChannel chNr);

private:
  QTcpSocket *_tcpSock = nullptr;
  QThread _thread;
  QTimer *_periodicUpdateTmr = nullptr;
  bool _periodicUpdateEnable = false;
  HMCChannel _selChannel = NoChannel;
  std::array<bool, HMCChannelCount> _channelEnabled;
  std::array<double, HMCChannelCount> _channelTargetVoltage;
  std::array<double, HMCChannelCount> _channelTargetCurrent;
  bool _masterOutEnabled = false;

  void initObjects();
  void createConnections();
  void createSocketConnections();
  QString sendCmdLine(QString cmd);
  void channelSelect(HMCChannel chNr);

public slots:
  void cleanup();
  void deviceConnect(const QHostAddress &addr);
  void deviceDisconnect();
  void updateChannelVoltage(HMCChannel chNr);
  void updateChannelCurrent(HMCChannel chNr);
  void updateChannelTargetVoltage(HMCChannel chNr);
  void updateChannelTargetCurrent(HMCChannel chNr);
  void updateChannelOutEnable(HMCChannel chNr);
  void updateMasterOutEnable();
  void setChannelVoltage(HMCChannel chNr, double voltage);
  void setChannelCurrent(HMCChannel chNr, double current);
  void setChannelOutEnable(HMCChannel chNr, bool enable);
  void setMasterOutEnable(bool enable);
  void setPeriodicUpdateEnable(bool enable);

private slots:
  void threadStarted();
  void socketConnected();
  void socketDisconnected();
  void socketError(QAbstractSocket::SocketError err);
  void onPeriodicTimer();

signals:
  void deviceConnected();
  void deviceConnectionError();
  void deviceDisconnected();
  void channelVoltageChanged(HMCSupplyCtrl::HMCChannel chNr, double voltage);
  void channelCurrentChanged(HMCSupplyCtrl::HMCChannel chNr, double current);
  void channelTargetVoltageChanged(HMCSupplyCtrl::HMCChannel chNr, double voltage);
  void channelTargetCurrentChanged(HMCSupplyCtrl::HMCChannel chNr, double current);
  void channelOutEnableChanged(HMCSupplyCtrl::HMCChannel chNr, bool enabled);
  void masterOutEnableChanged(bool enabled);


};

#endif // HMCSUPPLYCTRL_H
