#include <QApplication>
#include <QDebug>
#include "hmcsupplyctrl.h"

#define SOCK_TIMEOUT_MS             4000
#define PERIODIC_UPDATE_INTERVAL_MS 500
#define CH_TO_ARRAY_INDEX(x) ((int)x - 1)

const std::array<HMCSupplyCtrl::HMCChannel, HMCChannelCount> HMCSupplyCtrl::hmcChannels{Channel1, Channel2, Channel3};


/**
 * @brief HMCSupplyCtrl::HMCSupplyCtrl
 * @param parent
 */
HMCSupplyCtrl::HMCSupplyCtrl(QObject *parent)
    : QObject{parent}
{
  initObjects();
  createConnections();

  moveToThread(&_thread);
  _thread.start();
}

/**
 * @brief HMCSupplyCtrl::~HMCSupplyCtrl
 */
HMCSupplyCtrl::~HMCSupplyCtrl()
{
  _thread.quit();
  _thread.wait(QDeadlineTimer(5000));
}

/**
 * @brief HMCSupplyCtrl::getChannelTargetVoltage
 * @param chNr
 * @return channel target voltage
 */
double HMCSupplyCtrl::getChannelTargetVoltage(HMCChannel chNr) const
{
  return _channelTargetVoltage[CH_TO_ARRAY_INDEX(chNr)];
}

/**
 * @brief HMCSupplyCtrl::getChannelTargetCurrent
 * @param chNr
 * @return channel target current
 */
double HMCSupplyCtrl::getChannelTargetCurrent(HMCChannel chNr) const
{
  return _channelTargetCurrent[CH_TO_ARRAY_INDEX(chNr)];
}

/**
 * @brief HMCSupplyCtrl::isChannelEnabled
 * @param chNr
 * @return true if enabled
 */
bool HMCSupplyCtrl::isChannelEnabled(HMCChannel chNr)
{
  return _channelEnabled[CH_TO_ARRAY_INDEX(chNr)];
}

/**
 * @brief HMCSupplyCtrl::initObjects
 */
void HMCSupplyCtrl::initObjects()
{
  _periodicUpdateTmr = new QTimer(this);
  _periodicUpdateTmr->setSingleShot(true);
}

/**
 * @brief HMCSupplyCtrl::createConnections
 */
void HMCSupplyCtrl::createConnections()
{
  connect(&_thread, &QThread::started, this, &HMCSupplyCtrl::threadStarted);
  connect(_periodicUpdateTmr, &QTimer::timeout, this, &HMCSupplyCtrl::onPeriodicTimer);
}

/**
 * @brief HMCSupplyCtrl::createSocketConnections
 */
void HMCSupplyCtrl::createSocketConnections()
{
  connect(_tcpSock, &QTcpSocket::connected, this, &HMCSupplyCtrl::socketConnected);
  connect(_tcpSock, &QTcpSocket::disconnected, this, &HMCSupplyCtrl::socketDisconnected);
  connect(_tcpSock, &QAbstractSocket::errorOccurred, this, &HMCSupplyCtrl::socketError);
}

/**
 * @brief HMCSupplyCtrl::sendCmdLine
 * @param cmd
 * @return device response string
 */
QString HMCSupplyCtrl::sendCmdLine(QString cmd)
{
  QString response;

  if(_tcpSock == nullptr || _tcpSock->isWritable() == false) {
    qCritical() << Q_FUNC_INFO << "TCP socket is not ready wor write!";
    return response;
  }
  bool waitForResponse = cmd.endsWith("?");
  cmd.append('\n');
  _tcpSock->write(cmd.toLocal8Bit());
  _tcpSock->waitForBytesWritten(SOCK_TIMEOUT_MS);
  if(waitForResponse) {
    _tcpSock->waitForReadyRead(SOCK_TIMEOUT_MS);
    response = QString::fromLocal8Bit(_tcpSock->readAll());
  }
  return response;
}

/**
 * @brief HMCSupplyCtrl::channelSelect
 * @param chNr
 */
void HMCSupplyCtrl::channelSelect(HMCChannel chNr)
{
  if(_selChannel != chNr) {
    sendCmdLine(QString("INST OUT%1").arg(chNr));
    _selChannel = chNr;
  }
}

/**
 * @brief HMCSupplyCtrl::cleanup
 */
void HMCSupplyCtrl::cleanup()
{
  moveToThread(qApp->thread());
}

/**
 * @brief HMCSupplyCtrl::deviceConnect
 * @param addr
 */
void HMCSupplyCtrl::deviceConnect(const QHostAddress &addr)
{
  if(_tcpSock) {
    _tcpSock->close();
    _tcpSock->deleteLater();
  }
  _selChannel = NoChannel;
  _tcpSock = new QTcpSocket(this);
  createSocketConnections();
  _tcpSock->connectToHost(addr, HMC_SCPI_PORT);
  if(_tcpSock->waitForConnected(SOCK_TIMEOUT_MS)) {
    qDebug() << Q_FUNC_INFO << "socket successfuly connected";
  } else {
    qWarning() << Q_FUNC_INFO << "socket connect failed!";
    emit deviceConnectionFailed();
  }
}

/**
 * @brief HMCSupplyCtrl::deviceDisconnect
 */
void HMCSupplyCtrl::deviceDisconnect()
{
  if(_tcpSock) {
    _tcpSock->disconnectFromHost();
    if(_tcpSock->state() != QTcpSocket::UnconnectedState) {
      _tcpSock->waitForDisconnected(SOCK_TIMEOUT_MS);
    }
    _tcpSock->close();
    _tcpSock->deleteLater();
    _tcpSock = nullptr;
  }
}

/**
 * @brief HMCSupplyCtrl::updateChannelVoltage
 * @param chNr
 */
void HMCSupplyCtrl::updateChannelVoltage(HMCChannel chNr)
{
  bool parseOk = false;
  QString strCurr;
  double volt;

  channelSelect(chNr);
  strCurr = sendCmdLine("MEAS:VOLT?");
  volt = strCurr.toDouble(&parseOk);
  emit channelVoltageChanged(chNr, volt);
}

/**
 * @brief HMCSupplyCtrl::updateChannelCurrent
 * @param chNr
 */
void HMCSupplyCtrl::updateChannelCurrent(HMCChannel chNr)
{
  bool parseOk = false;
  QString strCurr;
  double curr;

  channelSelect(chNr);
  strCurr = sendCmdLine("MEAS:CURR?");
  curr = strCurr.toDouble(&parseOk);
  emit channelCurrentChanged(chNr, curr);
}

/**
 * @brief HMCSupplyCtrl::updateChannelTargetVoltage
 * @param chNr
 */
void HMCSupplyCtrl::updateChannelTargetVoltage(HMCChannel chNr)
{
  bool parseOk = false;
  QString strVal;
  double val;
  channelSelect(chNr);
  strVal = sendCmdLine("VOLT?");
  val = strVal.toDouble(&parseOk);
  _channelTargetVoltage[CH_TO_ARRAY_INDEX(chNr)] = val;
  emit channelTargetVoltageChanged(chNr, val);
}

/**
 * @brief HMCSupplyCtrl::updateChannelTargetCurrent
 * @param chNr
 */
void HMCSupplyCtrl::updateChannelTargetCurrent(HMCChannel chNr)
{
  bool parseOk = false;
  QString strVal;
  double val;
  channelSelect(chNr);
  strVal = sendCmdLine("CURR?");
  val = strVal.toDouble(&parseOk);
  _channelTargetCurrent[CH_TO_ARRAY_INDEX(chNr)] = val;
  emit channelTargetCurrentChanged(chNr, val);
}

/**
 * @brief HMCSupplyCtrl::updateChannelOutEnable
 * @param chNr
 */
void HMCSupplyCtrl::updateChannelOutEnable(HMCChannel chNr)
{
  bool parseOk = false;
  QString strVal;
  double val;
  channelSelect(chNr);
  strVal = sendCmdLine("OUTP:CHAN?");
  val = strVal.toDouble(&parseOk);
  _channelEnabled[CH_TO_ARRAY_INDEX(chNr)] = val;
  emit channelOutEnableChanged(chNr, val);
}

/**
 * @brief HMCSupplyCtrl::updateMasterOutEnable
 */
void HMCSupplyCtrl::updateMasterOutEnable()
{
  bool parseOk = false;
  QString strVal;
  double val;
  strVal = sendCmdLine("OUTP:MAST?");
  val = strVal.toDouble(&parseOk);
  _masterOutEnabled = val;
  emit masterOutEnableChanged(_masterOutEnabled);
}

/**
 * @brief HMCSupplyCtrl::setChannelVoltage
 * @param chNr
 * @param voltage
 */
void HMCSupplyCtrl::setChannelVoltage(HMCChannel chNr, double voltage)
{
  channelSelect(chNr);
  sendCmdLine(QString::asprintf("VOLT %.3f", voltage));
  updateChannelTargetVoltage(chNr);
}

/**
 * @brief HMCSupplyCtrl::setChannelCurrent
 * @param chNr
 * @param current
 */
void HMCSupplyCtrl::setChannelCurrent(HMCChannel chNr, double current)
{
  channelSelect(chNr);
  sendCmdLine(QString::asprintf("CURR %.3f", current));
  updateChannelTargetCurrent(chNr);
}

/**
 * @brief HMCSupplyCtrl::setChannelOutEnable
 * @param chNr
 * @param enable
 */
void HMCSupplyCtrl::setChannelOutEnable(HMCChannel chNr, bool enable)
{
  channelSelect(chNr);
  sendCmdLine(QString::asprintf("OUTP:CHAN %s", (enable ? "ON" : "OFF")));
}

/**
 * @brief HMCSupplyCtrl::setMasterOutEnable
 * @param enable
 */
void HMCSupplyCtrl::setMasterOutEnable(bool enable)
{
  sendCmdLine(QString::asprintf("OUTP:MAST %s", (enable ? "ON" : "OFF")));
}

/**
 * @brief HMCSupplyCtrl::setPeriodicUpdateEnable
 * @param enable
 */
void HMCSupplyCtrl::setPeriodicUpdateEnable(bool enable)
{
  if(enable ==  _periodicUpdateEnable) {
    return;
  }
  _periodicUpdateEnable = enable;
  if(enable) {
    _periodicUpdateTmr->start(PERIODIC_UPDATE_INTERVAL_MS);
  } else {
    _periodicUpdateTmr->stop();
  }
}

/**
 * @brief HMCSupplyCtrl::threadStarted
 */
void HMCSupplyCtrl::threadStarted()
{
  qDebug() << Q_FUNC_INFO << "thread started";
}

void HMCSupplyCtrl::socketConnected()
{
  _tcpSock->waitForReadyRead(1000);
  _tcpSock->readAll();

  qDebug() << Q_FUNC_INFO << "socket connected";
  qDebug() << sendCmdLine("*IDN?");
  for(auto ch : hmcChannels) {
    updateChannelVoltage(ch);
    updateChannelCurrent(ch);
    updateChannelTargetVoltage(ch);
    updateChannelTargetCurrent(ch);
    updateChannelOutEnable(ch);
  }
  updateMasterOutEnable();
  emit deviceConnected();
}

/**
 * @brief HMCSupplyCtrl::socketDisconnected
 */
void HMCSupplyCtrl::socketDisconnected()
{
  qDebug() << Q_FUNC_INFO << "socket disconnected";
  emit deviceDisconnected();
}

/**
 * @brief HMCSupplyCtrl::socketError
 * @param err
 */
void HMCSupplyCtrl::socketError(QAbstractSocket::SocketError err)
{
  qCritical() << Q_FUNC_INFO << err;
}

/**
 * @brief HMCSupplyCtrl::onPeriodicTimer
 */
void HMCSupplyCtrl::onPeriodicTimer()
{
  if(!_periodicUpdateEnable) {
    return;
  }
  for(auto ch : hmcChannels) {
    updateChannelCurrent(ch);
    updateChannelVoltage(ch);
  }
  _periodicUpdateTmr->start(PERIODIC_UPDATE_INTERVAL_MS);
}
