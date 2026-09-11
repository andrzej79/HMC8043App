#ifndef HMCSUPPLYCTRL_H
#define HMCSUPPLYCTRL_H

#include <QObject>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <array>
#include <atomic>

#define HMC_SCPI_PORT 5025


class HMCSupplyCtrl : public QObject
{
  Q_OBJECT
public:
  enum HMCChannel : int {
    NoChannel = 0,
    Channel1  = 1,
    Channel2,
    Channel3
  };
  Q_ENUM(HMCChannel);
  #define HMCChannelCount 3
  static const std::array<HMCChannel, HMCChannelCount> hmcChannels;

  static bool isValidChannel(HMCChannel chNr);

  explicit HMCSupplyCtrl(QObject *parent = nullptr);
  ~HMCSupplyCtrl();

  /* The ONLY member safe to call from another thread: it just stores into an
   * atomic. Setting it makes the in-flight blocking socket waits unwind within
   * ABORT_POLL_SLICE_MS instead of running out their full timeout, which is what
   * keeps disconnect and shutdown from stalling the GUI. */
  void abortPendingIo();

private:
  QTcpSocket *_tcpSock = nullptr;
  QThread _thread;
  QTimer *_periodicUpdateTmr = nullptr;
  bool _periodicUpdateEnable = false;
  bool _connectInProgress = false;
  bool _handshakeOk = false;
  bool _linkLostReported = false;
  HMCChannel _selChannel = NoChannel;
  std::array<bool, HMCChannelCount> _channelEnabled{};
  std::array<double, HMCChannelCount> _channelTargetVoltage{};
  std::array<double, HMCChannelCount> _channelTargetCurrent{};
  bool _masterOutEnabled = false;
  std::array<double, HMCChannelCount> _channelMaxVoltage{};
  std::array<double, HMCChannelCount> _channelMaxCurrent{};
  std::atomic<bool> _abortRequested = false;

  void initObjects();
  void createConnections();
  void createSocketConnections();
  QString sendCmdLine(QString cmd, bool *status = nullptr);
  QString readReplyLine(bool *status);
  bool waitForWriteInterruptible(QDeadlineTimer &deadline);
  bool waitForReadInterruptible(QDeadlineTimer &deadline);
  bool updateChannelLimits(HMCChannel chNr);
  void reportLinkLost(const char *why);
  bool sendCmdAndParseReply(QString cmd, double *val);
  bool sendChannelCmdAndParseReply(HMCChannel chNr, QString cmd, double *val);
  bool sendCmdAndParseReply(QString cmd, int *val);
  bool sendChannelCmdAndParseReply(HMCChannel chNr, QString cmd, int *val);
  bool channelSelect(HMCChannel chNr);

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
  void deviceIdentified(const QString &idn);
  void deviceConnectionFailed();
  void deviceConnectionError();
  void deviceDisconnected();
  void channelVoltageChanged(HMCSupplyCtrl::HMCChannel chNr, double voltage);
  void channelCurrentChanged(HMCSupplyCtrl::HMCChannel chNr, double current);
  void channelTargetVoltageChanged(HMCSupplyCtrl::HMCChannel chNr, double voltage);
  void channelTargetCurrentChanged(HMCSupplyCtrl::HMCChannel chNr, double current);
  void channelOutEnableChanged(HMCSupplyCtrl::HMCChannel chNr, bool enabled);
  void masterOutEnableChanged(bool enabled);
  void channelLimitsChanged(HMCSupplyCtrl::HMCChannel chNr, double maxVoltage, double maxCurrent);


};

#endif // HMCSUPPLYCTRL_H
