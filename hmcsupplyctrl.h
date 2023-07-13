#ifndef HMCSUPPLYCTRL_H
#define HMCSUPPLYCTRL_H

#include <QObject>
#include <QTcpSocket>
#include <QThread>
#include <array>

#define HMC_SCPI_PORT 5025


class HMCSupplyCtrl : public QObject
{
  Q_OBJECT
public:
  enum HMCChannel {
    Channel1 = 1,
    Channel2,
    Channel3
  };
  Q_ENUM(HMCChannel);
  #define HMCChannelCount 3
  static const std::array<HMCChannel, HMCChannelCount> hmcChannels;

  explicit HMCSupplyCtrl(QObject *parent = nullptr);
  ~HMCSupplyCtrl();

private:
  QTcpSocket *_tcpSock = nullptr;
  QThread _thread;

  void createSocketConnections();
  QString sendCmdLine(QString cmd);

public slots:
  void cleanup();
  void deviceConnect(const QHostAddress &addr);
  void deviceDisconnect();
  void updateChannelVoltage(HMCChannel chNr);
  void updateChannelCurrent(HMCChannel chNr);
  void setChannelVoltage(HMCChannel chNr, double voltage);
  void setChannelCurrent(HMCChannel chNr, double current);
  void setChannelOutEnable(HMCChannel chNr, bool enable);
  void setMasterOutEnable(bool enable);

private slots:
  void threadStarted();
  void socketConnected();
  void socketDisconnected();
  void socketError(QAbstractSocket::SocketError err);
  void onRefreshTmr();

signals:
  void deviceConnected();
  void deviceConnectionError();
  void deviceDisconnected();
  void channelVoltageChanged(HMCSupplyCtrl::HMCChannel chNr, double voltage);
  void channelCurrentChanged(HMCSupplyCtrl::HMCChannel chNr, double current);

};

#endif // HMCSUPPLYCTRL_H
