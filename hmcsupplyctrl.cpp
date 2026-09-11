#include <QApplication>
#include <QDeadlineTimer>
#include <QDebug>
#include "hmcsupplyctrl.h"
#include "hmcappglobal.h"

#define SOCK_TIMEOUT_MS             4000
#define PERIODIC_UPDATE_INTERVAL_MS 500
/* Granularity at which the blocking socket waits check the abort flag. */
#define ABORT_POLL_SLICE_MS         100

/* HMCChannel is a public Q_ENUM whose zero value, NoChannel, would index -1.
 * Route every subscript through here so an invalid channel cannot reach the arrays. */
static inline int chToArrayIndex(HMCSupplyCtrl::HMCChannel chNr)
{
  Q_ASSERT(HMCSupplyCtrl::isValidChannel(chNr));
  return static_cast<int>(chNr) - 1;
}

const std::array<HMCSupplyCtrl::HMCChannel, HMCChannelCount> HMCSupplyCtrl::hmcChannels{Channel1, Channel2, Channel3};


/**
 * @brief HMCSupplyCtrl::isValidChannel
 * @param chNr
 * @return true for Channel1..Channel3
 */
bool HMCSupplyCtrl::isValidChannel(HMCChannel chNr)
{
  return chNr >= Channel1 && chNr <= Channel3;
}

/**
 * @brief HMCSupplyCtrl::abortPendingIo
 *
 * Thread-safe by design - see the declaration.
 */
void HMCSupplyCtrl::abortPendingIo()
{
  _abortRequested.store(true, std::memory_order_release);
}

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
  /* quit() only posts to the worker's event loop, so it has no effect while the
   * worker sits in a blocking socket wait. If the join times out we must not fall
   * through to ~QThread on a still-running thread - that aborts the process. */
  _thread.quit();
  if(!_thread.wait(QDeadlineTimer(5000))) {
    qCritical() << Q_FUNC_INFO << "worker thread did not stop, terminating it";
    _thread.terminate();
    _thread.wait();
  }
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
 * @param status
 * @return reply string or empty
 */
QString HMCSupplyCtrl::sendCmdLine(QString cmd, bool *status)
{
  bool statTmp = false;
  QString response;

  if(_linkLostReported) {
    if(status) {
      *status = false;
    }
    return response;
  }

  /* isWritable() only reflects the QIODevice open mode - a socket that failed to
   * connect or was dropped by the peer still reports true. Gate on the actual
   * connection state instead. */
  if(_tcpSock != nullptr && _tcpSock->state() == QAbstractSocket::ConnectedState) {
    /* A query is recognised by its header (first token), not by the end of the line:
     * "VOLT? MAX" is a query too. Missing that treats it as a write, leaving its reply
     * in the buffer to be read as the answer to whatever is sent next. */
    const bool waitForResponse = cmd.trimmed().section(QLatin1Char(' '), 0, 0).endsWith(QLatin1Char('?'));

    /* Anything still buffered belongs to an earlier exchange - a reply that
     * arrived after its own timeout. Drop it, or it would be consumed as the
     * answer to this command and desynchronise the stream for good. */
    const auto stale = _tcpSock->bytesAvailable();
    if(stale > 0) {
      qWarning() << Q_FUNC_INFO << "discarding" << stale << "stale byte(s)";
      _tcpSock->readAll();
    }

    cmd.append('\n');
    _tcpSock->write(cmd.toLocal8Bit());
    QDeadlineTimer wrDeadline(SOCK_TIMEOUT_MS);
    if(waitForWriteInterruptible(wrDeadline)) {
      if(waitForResponse) {
        response = readReplyLine(&statTmp);
      } else {
        statTmp = true;
      }
    } else {
      qWarning() << Q_FUNC_INFO << "write timeout:" << cmd.trimmed();
    }
  } else {
    qCritical() << Q_FUNC_INFO << "TCP socket is not connected!";
  }

  /* A command that ran out its whole deadline means the instrument is gone - e.g.
   * powered off, which produces no socket error until the OS gives up much later.
   * (reportLinkLost() ignores this while connecting and after a requested abort.) */
  if(!statTmp) {
    reportLinkLost("command timed out");
  }

  if(status) {
    *status = statTmp;
  }
  return response;
}

/**
 * @brief HMCSupplyCtrl::waitForWriteInterruptible
 * @param deadline
 * @return true once the bytes are on the wire
 *
 * waitForBytesWritten()/waitForReadyRead() do not dispatch queued slot calls, so a
 * single long wait makes the worker deaf to everything - including shutdown - for
 * its full duration. Wait in slices instead and bail out when abort is requested.
 */
bool HMCSupplyCtrl::waitForWriteInterruptible(QDeadlineTimer &deadline)
{
  while(!deadline.hasExpired()) {
    if(_abortRequested.load(std::memory_order_acquire)) {
      return false;
    }
    const int slice = static_cast<int>(qMin<qint64>(ABORT_POLL_SLICE_MS, deadline.remainingTime()));
    if(_tcpSock->waitForBytesWritten(slice)) {
      return true;
    }
    if(_tcpSock->state() != QAbstractSocket::ConnectedState) {
      return false;
    }
  }
  return false;
}

/**
 * @brief HMCSupplyCtrl::waitForReadInterruptible
 * @param deadline
 * @return true once at least one byte is readable
 */
bool HMCSupplyCtrl::waitForReadInterruptible(QDeadlineTimer &deadline)
{
  while(!deadline.hasExpired()) {
    if(_abortRequested.load(std::memory_order_acquire)) {
      return false;
    }
    const int slice = static_cast<int>(qMin<qint64>(ABORT_POLL_SLICE_MS, deadline.remainingTime()));
    if(_tcpSock->waitForReadyRead(slice)) {
      return true;
    }
    if(_tcpSock->state() != QAbstractSocket::ConnectedState) {
      return false;
    }
  }
  return false;
}

/**
 * @brief HMCSupplyCtrl::readReplyLine
 * @param status set to true only when a complete, newline-terminated reply arrived
 * @return the reply without its terminator
 *
 * TCP carries no message framing and waitForReadyRead() returns on the first byte,
 * so a single readAll() can hand back a truncated reply - and a truncated number
 * still parses. Keep reading until the terminator shows up or the deadline expires.
 */
QString HMCSupplyCtrl::readReplyLine(bool *status)
{
  QByteArray buf;
  QDeadlineTimer deadline(SOCK_TIMEOUT_MS);

  while(!deadline.hasExpired()) {
    if(_tcpSock->bytesAvailable() == 0) {
      if(!waitForReadInterruptible(deadline)) {
        break;
      }
    }
    buf.append(_tcpSock->readAll());
    const auto nlIdx = buf.indexOf('\n');
    if(nlIdx >= 0) {
      if(nlIdx + 1 < buf.size()) {
        qWarning() << Q_FUNC_INFO << "extra bytes after reply terminator, discarding";
      }
      if(status) {
        *status = true;
      }
      return QString::fromLocal8Bit(buf.left(nlIdx)).trimmed();
    }
  }
  qWarning() << Q_FUNC_INFO << "incomplete reply," << buf.size() << "byte(s), no terminator";
  if(status) {
    *status = false;
  }
  return QString();
}

/**
 * @brief HMCSupplyCtrl::sendCmdAndParseReply
 * @param cmd
 * @param val
 * @return true if successfull
 */
bool HMCSupplyCtrl::sendCmdAndParseReply(QString cmd, double *val)
{
  Q_ASSERT(val != nullptr);
  bool parseOk = false;
  bool cmdOk = false;
  QString replyStr;
  double tmp;

  replyStr = sendCmdLine(cmd, &cmdOk);
  if(!cmdOk) {
    return false;
  }
  tmp = replyStr.toDouble(&parseOk);
  if(parseOk) {
    *val = tmp;
    return true;
  }
  return false;
}

/**
 * @brief HMCSupplyCtrl::sendChannelCmdAndParseReply
 * @param chNr
 * @param cmd
 * @param val
 * @return true if successfull
 */
bool HMCSupplyCtrl::sendChannelCmdAndParseReply(HMCChannel chNr, QString cmd, double *val)
{
  if(!channelSelect(chNr)) {
    return false;
  }
  return sendCmdAndParseReply(cmd, val);
}

/**
 * @brief HMCSupplyCtrl::sendCmdAndParseReply
 * @param cmd
 * @param val
 * @return true if successfull
 */
bool HMCSupplyCtrl::sendCmdAndParseReply(QString cmd, int *val)
{
  Q_ASSERT(val != nullptr);
  bool parseOk = false;
  bool cmdOk = false;
  QString replyStr;
  int tmp;

  replyStr = sendCmdLine(cmd, &cmdOk);
  if(!cmdOk) {
    return false;
  }
  tmp = replyStr.toInt(&parseOk);
  if(parseOk) {
    *val = tmp;
    return true;
  }
  return false;
}

/**
 * @brief HMCSupplyCtrl::sendChannelCmdAndParseReply
 * @param chNr
 * @param cmd
 * @param val
 * @return true if successfull
 */
bool HMCSupplyCtrl::sendChannelCmdAndParseReply(HMCChannel chNr, QString cmd, int *val)
{
  if(!channelSelect(chNr)) {
    return false;
  }
  return sendCmdAndParseReply(cmd, val);
}

/**
 * @brief HMCSupplyCtrl::channelSelect
 * @param chNr
 * @return true if operation successfull
 */
bool HMCSupplyCtrl::channelSelect(HMCChannel chNr)
{
  if(chNr == NoChannel) {
    qCritical() << Q_FUNC_INFO << "refusing to select NoChannel";
    return false;
  }
  if(_selChannel == chNr) {
    return true;
  }
  bool status = false;
  sendCmdLine(QString("INST OUT%1").arg(chNr), &status);
  /* Cache the selection only once it actually reached the instrument. Caching it
   * after a failed write would make every later call for this channel a no-op,
   * and the channel-scoped command that follows would be applied to whatever
   * channel the instrument still has selected. */
  _selChannel = status ? chNr : NoChannel;
  return status;
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
  _abortRequested.store(false, std::memory_order_release);
  _linkLostReported = false;
  _handshakeOk = false;
  _connectInProgress = true;
  _tcpSock->connectToHost(addr, HMC_SCPI_PORT);
  /* socketConnected() runs nested inside this call and performs the *IDN?
   * handshake, so both the socket and the handshake must have succeeded. */
  const bool connected = _tcpSock->waitForConnected(SOCK_TIMEOUT_MS);
  _connectInProgress = false;
  if(connected && _handshakeOk) {
    qDebug() << Q_FUNC_INFO << "socket successfuly connected";
  } else {
    /* The 4 s wait is shorter than the OS connect timeout, so without abort()
     * the attempt stays pending and can succeed after the user was already told
     * it failed - silently arming the app behind an error dialog. */
    qWarning() << Q_FUNC_INFO << "socket connect failed!";
    if(_tcpSock) {
      _tcpSock->abort();
      _tcpSock->deleteLater();
      _tcpSock = nullptr;
    }
    emit deviceConnectionFailed();
  }
}

/**
 * @brief HMCSupplyCtrl::deviceDisconnect
 */
void HMCSupplyCtrl::deviceDisconnect()
{
  _periodicUpdateEnable = false;
  _periodicUpdateTmr->stop();
  _selChannel = NoChannel;
  _channelEnabled.fill(false);
  _channelTargetVoltage.fill(0.0);
  _channelTargetCurrent.fill(0.0);
  _masterOutEnabled = false;
  if(_tcpSock) {
    if(_abortRequested.load(std::memory_order_acquire)) {
      /* Shutting down or the link is already gone - drop it immediately rather
       * than spending another socket timeout on a graceful close. */
      _tcpSock->abort();
    } else {
      _tcpSock->disconnectFromHost();
      if(_tcpSock->state() != QTcpSocket::UnconnectedState) {
        QDeadlineTimer deadline(SOCK_TIMEOUT_MS);
        while(!deadline.hasExpired() && _tcpSock->state() != QTcpSocket::UnconnectedState) {
          if(_abortRequested.load(std::memory_order_acquire)) {
            _tcpSock->abort();
            break;
          }
          _tcpSock->waitForDisconnected(ABORT_POLL_SLICE_MS);
        }
      }
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
  if(!isValidChannel(chNr)) {
    qCritical() << Q_FUNC_INFO << "invalid channel" << chNr;
    return;
  }
  double val;
  auto cmdRes = sendChannelCmdAndParseReply(chNr, "MEAS:VOLT?", &val);
  if(cmdRes) {
    emit channelVoltageChanged(chNr, val);
  }
}

/**
 * @brief HMCSupplyCtrl::updateChannelCurrent
 * @param chNr
 */
void HMCSupplyCtrl::updateChannelCurrent(HMCChannel chNr)
{
  if(!isValidChannel(chNr)) {
    qCritical() << Q_FUNC_INFO << "invalid channel" << chNr;
    return;
  }
  double val;
  auto cmdRes = sendChannelCmdAndParseReply(chNr, "MEAS:CURR?", &val);
  if(cmdRes) {
    emit channelCurrentChanged(chNr, val);
  }
}

/**
 * @brief HMCSupplyCtrl::updateChannelTargetVoltage
 * @param chNr
 */
void HMCSupplyCtrl::updateChannelTargetVoltage(HMCChannel chNr)
{
  if(!isValidChannel(chNr)) {
    qCritical() << Q_FUNC_INFO << "invalid channel" << chNr;
    return;
  }
  double val;
  auto cmdRes = sendChannelCmdAndParseReply(chNr, "VOLT?", &val);
  if(cmdRes) {
    _channelTargetVoltage[chToArrayIndex(chNr)] = val;
    emit channelTargetVoltageChanged(chNr, val);
  }
}

/**
 * @brief HMCSupplyCtrl::updateChannelTargetCurrent
 * @param chNr
 */
void HMCSupplyCtrl::updateChannelTargetCurrent(HMCChannel chNr)
{
  if(!isValidChannel(chNr)) {
    qCritical() << Q_FUNC_INFO << "invalid channel" << chNr;
    return;
  }
  double val;
  auto cmdRes = sendChannelCmdAndParseReply(chNr, "CURR?", &val);
  if(cmdRes) {
    _channelTargetCurrent[chToArrayIndex(chNr)] = val;
    emit channelTargetCurrentChanged(chNr, val);
  }
}

/**
 * @brief HMCSupplyCtrl::updateChannelOutEnable
 * @param chNr
 */
void HMCSupplyCtrl::updateChannelOutEnable(HMCChannel chNr)
{
  if(!isValidChannel(chNr)) {
    qCritical() << Q_FUNC_INFO << "invalid channel" << chNr;
    return;
  }
  int val;
  auto cmdRes = sendChannelCmdAndParseReply(chNr, "OUTP:CHAN?", &val);
  if(cmdRes) {
    _channelEnabled[chToArrayIndex(chNr)] = val;
    emit channelOutEnableChanged(chNr, val);
  }
}

/**
 * @brief HMCSupplyCtrl::updateChannelLimits
 * @param chNr
 * @return true if both limits were read back
 *
 * Ask the instrument what it actually accepts instead of hardcoding a guess, so the
 * UI can reject an out-of-range entry up front rather than sending a command the
 * supply silently refuses into an error queue nobody reads.
 */
bool HMCSupplyCtrl::updateChannelLimits(HMCChannel chNr)
{
  if(!isValidChannel(chNr)) {
    return false;
  }
  double maxV = FALLBACK_MAX_VOLTAGE;
  double maxC = FALLBACK_MAX_CURRENT;
  double val;

  if(sendChannelCmdAndParseReply(chNr, "VOLT? MAX", &val) && val > 0.0) {
    maxV = val;
  } else {
    qWarning() << Q_FUNC_INFO << "VOLT? MAX failed, using fallback" << maxV;
  }
  if(sendChannelCmdAndParseReply(chNr, "CURR? MAX", &val) && val > 0.0) {
    maxC = val;
  } else {
    qWarning() << Q_FUNC_INFO << "CURR? MAX failed, using fallback" << maxC;
  }
  _channelMaxVoltage[chToArrayIndex(chNr)] = maxV;
  _channelMaxCurrent[chToArrayIndex(chNr)] = maxC;
  emit channelLimitsChanged(chNr, maxV, maxC);
  return true;
}

/**
 * @brief HMCSupplyCtrl::updateMasterOutEnable
 */
void HMCSupplyCtrl::updateMasterOutEnable()
{
  int val;
  auto cmdRes = sendCmdAndParseReply("OUTP:MAST?", &val);
  if(cmdRes) {
    _masterOutEnabled = val;
    emit masterOutEnableChanged(_masterOutEnabled);
  }
}

/**
 * @brief HMCSupplyCtrl::setChannelVoltage
 * @param chNr
 * @param voltage
 */
void HMCSupplyCtrl::setChannelVoltage(HMCChannel chNr, double voltage)
{
  if(!isValidChannel(chNr)) {
    qCritical() << Q_FUNC_INFO << "invalid channel" << chNr;
    return;
  }
  if(!channelSelect(chNr)) {
    qCritical() << Q_FUNC_INFO << "channel select failed, voltage NOT set";
    reportLinkLost("command failed");
    return;
  }
  bool status = false;
  sendCmdLine(QString::asprintf("VOLT %.3f", voltage), &status);
  if(!status) {
    qCritical() << Q_FUNC_INFO << "voltage command failed";
    reportLinkLost("command failed");
    return;
  }
  updateChannelTargetVoltage(chNr);
}

/**
 * @brief HMCSupplyCtrl::setChannelCurrent
 * @param chNr
 * @param current
 */
void HMCSupplyCtrl::setChannelCurrent(HMCChannel chNr, double current)
{
  if(!isValidChannel(chNr)) {
    qCritical() << Q_FUNC_INFO << "invalid channel" << chNr;
    return;
  }
  if(!channelSelect(chNr)) {
    qCritical() << Q_FUNC_INFO << "channel select failed, current NOT set";
    reportLinkLost("command failed");
    return;
  }
  bool status = false;
  sendCmdLine(QString::asprintf("CURR %.3f", current), &status);
  if(!status) {
    qCritical() << Q_FUNC_INFO << "current command failed";
    reportLinkLost("command failed");
    return;
  }
  updateChannelTargetCurrent(chNr);
}

/**
 * @brief HMCSupplyCtrl::setChannelOutEnable
 * @param chNr
 * @param enable
 */
void HMCSupplyCtrl::setChannelOutEnable(HMCChannel chNr, bool enable)
{
  if(!isValidChannel(chNr)) {
    qCritical() << Q_FUNC_INFO << "invalid channel" << chNr;
    return;
  }
  if(!channelSelect(chNr)) {
    qCritical() << Q_FUNC_INFO << "channel select failed, output NOT switched";
    reportLinkLost("command failed");
    return;
  }
  bool status = false;
  sendCmdLine(QString::asprintf("OUTP:CHAN %s", (enable ? "ON" : "OFF")), &status);
  if(!status) {
    qCritical() << Q_FUNC_INFO << "output enable command failed";
    reportLinkLost("command failed");
    return;
  }
  /* Drive the UI from what the instrument reports, never from the click. */
  updateChannelOutEnable(chNr);
}

/**
 * @brief HMCSupplyCtrl::setMasterOutEnable
 * @param enable
 */
void HMCSupplyCtrl::setMasterOutEnable(bool enable)
{
  bool status = false;
  sendCmdLine(QString::asprintf("OUTP:MAST %s", (enable ? "ON" : "OFF")), &status);
  if(!status) {
    qCritical() << Q_FUNC_INFO << "master output command failed";
    reportLinkLost("command failed");
    return;
  }
  updateMasterOutEnable();
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

/**
 * @brief HMCSupplyCtrl::socketConnected
 */
void HMCSupplyCtrl::socketConnected()
{
  qDebug() << Q_FUNC_INFO << "socket connected";
  _selChannel = NoChannel;

  /* A raw SCPI socket sends no greeting, so *IDN? is the handshake: if the
   * instrument does not answer it, the TCP connection is up but the device is
   * not usable, and reporting a successful connect would arm a live control
   * surface over a link that does not work. */
  bool status = false;
  const QString idn = sendCmdLine("*IDN?", &status);
  if(!status || idn.isEmpty()) {
    /* Leave the failure report to deviceConnect(), which owns the socket
     * lifetime - emitting it here too would raise two error dialogs. */
    qCritical() << Q_FUNC_INFO << "no identification reply, aborting connect";
    _tcpSock->abort();
    return;
  }
  qDebug() << Q_FUNC_INFO << "device:" << idn;

  for(auto ch : hmcChannels) {
    updateChannelLimits(ch);
    updateChannelVoltage(ch);
    updateChannelCurrent(ch);
    updateChannelTargetVoltage(ch);
    updateChannelTargetCurrent(ch);
    updateChannelOutEnable(ch);
  }
  updateMasterOutEnable();
  _handshakeOk = true;
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
 * @brief HMCSupplyCtrl::reportLinkLost
 * @param why
 *
 * Declares the link dead once per session: stops the poll, drops the cached channel
 * selection and tells the UI. After this, sendCmdLine() fails fast, so the rest of an
 * in-flight poll tick does not spend a full socket timeout on each remaining query.
 */
void HMCSupplyCtrl::reportLinkLost(const char *why)
{
  _periodicUpdateEnable = false;
  _periodicUpdateTmr->stop();
  _selChannel = NoChannel;
  if(_connectInProgress || _linkLostReported || _abortRequested.load(std::memory_order_acquire)) {
    return;
  }
  _linkLostReported = true;
  qCritical() << Q_FUNC_INFO << why;
  emit deviceConnectionError();
}

/**
 * @brief HMCSupplyCtrl::socketError
 * @param err
 */
void HMCSupplyCtrl::socketError(QAbstractSocket::SocketError err)
{
  /* Every waitFor*() that times out makes QAbstractSocket emit SocketTimeoutError.
   * The waits run in ABORT_POLL_SLICE_MS slices, so treating that as fatal would
   * declare the link lost whenever a reply took longer than one slice. Whether a
   * timeout matters is decided in sendCmdLine(), against the full deadline. */
  if(err == QAbstractSocket::SocketTimeoutError) {
    return;
  }
  qCritical() << Q_FUNC_INFO << err;
  reportLinkLost("socket error");
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
  /* The tick may have stopped polling (link lost, disconnect) - don't undo that. */
  if(_periodicUpdateEnable) {
    _periodicUpdateTmr->start(PERIODIC_UPDATE_INTERVAL_MS);
  }
}
