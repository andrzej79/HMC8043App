#include <QCoreApplication>
#include <QHostAddress>
#include <QLocale>
#include <QSettings>
#include <QtNumeric>
#include <algorithm>
#include "hmcappglobal.h"
#include "hmcpresets.h"
#include "supplybackend.h"

/**
 * @brief SupplyBackend::SupplyBackend
 * @param parent
 */
SupplyBackend::SupplyBackend(QObject *parent)
    : QObject{parent}
{
  /* Register before any queued connection is made - the driver's thread is
   * already running at this point (HMCSupplyCtrl starts it in its constructor). */
  qRegisterMetaType<HMCSupplyCtrl::HMCChannel>("HMCSupplyCtrl::HMCChannel");

  for(auto ch : HMCSupplyCtrl::hmcChannels) {
    _channels.append(new ChannelState(static_cast<int>(ch), this));
  }

  QSettings s;
  _hostAddress = s.value(SKEY_HOSTADDR, DEFAULT_HOST).toString();
  _autoConnect = s.value(SKEY_AUTOCONNECT, false).toBool();

  createConnections();
  connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
          this, &SupplyBackend::shutdown);
}

/**
 * @brief SupplyBackend::~SupplyBackend
 */
SupplyBackend::~SupplyBackend()
{
  shutdown();
}

/**
 * @brief SupplyBackend::createConnections
 *
 * The driver emits on its own thread; with `this` as context these lambdas are
 * queued onto the GUI thread, so the ChannelState objects are only ever touched here.
 */
void SupplyBackend::createConnections()
{
  connect(&_ctrl, &HMCSupplyCtrl::deviceConnected, this, [this]() {
    setConnectionState(true, false);
    QMetaObject::invokeMethod(&_ctrl, [this]() { _ctrl.setPeriodicUpdateEnable(true); });
  });
  connect(&_ctrl, &HMCSupplyCtrl::deviceConnectionFailed, this, [this]() {
    setConnectionState(false, false);
    if(!_errorReported && !_shutdownDone) {
      _errorReported = true;
      emit errorOccurred(tr("Connection failed"),
                         tr("No HMC8043 answered at %1.").arg(_hostAddress));
    }
  });
  connect(&_ctrl, &HMCSupplyCtrl::deviceConnectionError, this, [this]() {
    QMetaObject::invokeMethod(&_ctrl, [this]() { _ctrl.deviceDisconnect(); });
    resetState();
    if(!_errorReported && !_shutdownDone) {
      _errorReported = true;
      emit errorOccurred(tr("Connection lost"),
                         tr("The connection to the power supply was lost."));
    }
  });
  connect(&_ctrl, &HMCSupplyCtrl::deviceDisconnected, this, &SupplyBackend::resetState);
  connect(&_ctrl, &HMCSupplyCtrl::deviceIdentified, this, [this](const QString &idn) {
    _deviceIdentity = idn;
    emit deviceIdentityChanged();
  });
  connect(&_ctrl, &HMCSupplyCtrl::masterOutEnableChanged, this, &SupplyBackend::setMasterOutputState);

  connect(&_ctrl, &HMCSupplyCtrl::channelVoltageChanged, this,
          [this](HMCSupplyCtrl::HMCChannel ch, double v) { if(auto *s = channelFor(ch)) s->setVoltage(v); });
  connect(&_ctrl, &HMCSupplyCtrl::channelCurrentChanged, this,
          [this](HMCSupplyCtrl::HMCChannel ch, double a) { if(auto *s = channelFor(ch)) s->setCurrent(a); });
  connect(&_ctrl, &HMCSupplyCtrl::channelTargetVoltageChanged, this,
          [this](HMCSupplyCtrl::HMCChannel ch, double v) { if(auto *s = channelFor(ch)) s->setTargetVoltage(v); });
  connect(&_ctrl, &HMCSupplyCtrl::channelTargetCurrentChanged, this,
          [this](HMCSupplyCtrl::HMCChannel ch, double a) { if(auto *s = channelFor(ch)) s->setTargetCurrent(a); });
  connect(&_ctrl, &HMCSupplyCtrl::channelOutEnableChanged, this,
          [this](HMCSupplyCtrl::HMCChannel ch, bool e) { if(auto *s = channelFor(ch)) s->setOutputEnabled(e); });
  connect(&_ctrl, &HMCSupplyCtrl::channelLimitsChanged, this,
          [this](HMCSupplyCtrl::HMCChannel ch, double maxV, double maxA) {
            if(auto *s = channelFor(ch)) s->setLimits(maxV, maxA); });
}

QQmlListProperty<ChannelState> SupplyBackend::channels()
{
  return QQmlListProperty<ChannelState>(this, &_channels);
}

void SupplyBackend::setHostAddress(const QString &address)
{
  const QString trimmed = address.trimmed();
  if(!isValidHostAddress(trimmed) || trimmed == _hostAddress) {
    return;
  }
  _hostAddress = trimmed;
  QSettings().setValue(SKEY_HOSTADDR, _hostAddress);
  emit hostAddressChanged();
}

void SupplyBackend::setAutoConnect(bool enable)
{
  if(enable == _autoConnect) {
    return;
  }
  _autoConnect = enable;
  QSettings().setValue(SKEY_AUTOCONNECT, _autoConnect);
  emit autoConnectChanged();
}

QList<double> SupplyBackend::voltagePresets() const
{
  return HMC_VOLTAGE_PRESETS;
}

QList<double> SupplyBackend::currentPresets() const
{
  return HMC_CURRENT_PRESETS;
}

/**
 * @brief SupplyBackend::connectDevice
 */
void SupplyBackend::connectDevice()
{
  if(_connected || _connecting || _shutdownDone) {
    return;
  }
  const QHostAddress addr(_hostAddress);
  if(addr.isNull()) {
    emit errorOccurred(tr("Invalid address"),
                       tr("\"%1\" is not a valid IP address. Set it in Settings.").arg(_hostAddress));
    return;
  }
  _errorReported = false;
  setConnectionState(false, true);
  QMetaObject::invokeMethod(&_ctrl, [this, addr]() { _ctrl.deviceConnect(addr); });
}

/**
 * @brief SupplyBackend::disconnectDevice
 */
void SupplyBackend::disconnectDevice()
{
  if(_shutdownDone) {
    return;
  }
  /* A deliberate disconnect - including cancelling a connect in progress - must not
   * come back as a "connection failed/lost" error. */
  _errorReported = true;
  _ctrl.abortPendingIo();   /* thread-safe by design: a single atomic store */
  QMetaObject::invokeMethod(&_ctrl, [this]() {
    _ctrl.setPeriodicUpdateEnable(false);
    _ctrl.deviceDisconnect();
  });
  resetState();
}

void SupplyBackend::setMasterOutputEnabled(bool enable)
{
  if(!_connected || _shutdownDone) {
    return;
  }
  QMetaObject::invokeMethod(&_ctrl, [this, enable]() { _ctrl.setMasterOutEnable(enable); });
}

void SupplyBackend::setChannelVoltage(int channel, double voltage)
{
  const auto chNr = static_cast<HMCSupplyCtrl::HMCChannel>(channel);
  auto *state = channelFor(chNr);
  if(!_connected || _shutdownDone || !state) {
    return;
  }
  const double v = std::clamp(voltage, 0.0, state->maxVoltage());
  QMetaObject::invokeMethod(&_ctrl, [this, chNr, v]() { _ctrl.setChannelVoltage(chNr, v); });
}

void SupplyBackend::setChannelCurrent(int channel, double current)
{
  const auto chNr = static_cast<HMCSupplyCtrl::HMCChannel>(channel);
  auto *state = channelFor(chNr);
  if(!_connected || _shutdownDone || !state) {
    return;
  }
  const double a = std::clamp(current, 0.0, state->maxCurrent());
  QMetaObject::invokeMethod(&_ctrl, [this, chNr, a]() { _ctrl.setChannelCurrent(chNr, a); });
}

void SupplyBackend::setChannelOutputEnabled(int channel, bool enable)
{
  const auto chNr = static_cast<HMCSupplyCtrl::HMCChannel>(channel);
  if(!_connected || _shutdownDone || !channelFor(chNr)) {
    return;
  }
  QMetaObject::invokeMethod(&_ctrl, [this, chNr, enable]() { _ctrl.setChannelOutEnable(chNr, enable); });
}

bool SupplyBackend::isValidHostAddress(const QString &address) const
{
  return !QHostAddress(address.trimmed()).isNull();
}

/**
 * @brief SupplyBackend::parseNumber
 * @param text
 * @return the value, or NaN when the text is not a number
 *
 * Phone keyboards follow the system locale, so a Polish keyboard types "3,3".
 * Accept either separator; the instrument itself always gets C-locale formatting.
 */
double SupplyBackend::parseNumber(const QString &text) const
{
  QString t = text.trimmed();
  t.replace(QLatin1Char(','), QLatin1Char('.'));
  bool ok = false;
  const double v = QLocale::c().toDouble(t, &ok);
  return ok ? v : qQNaN();
}

void SupplyBackend::setConnectionState(bool connected, bool connecting)
{
  if(connected == _connected && connecting == _connecting) {
    return;
  }
  _connected = connected;
  _connecting = connecting;
  emit connectionStateChanged();
}

void SupplyBackend::setMasterOutputState(bool enabled)
{
  if(enabled == _masterOutputEnabled) {
    return;
  }
  _masterOutputEnabled = enabled;
  emit masterOutputEnabledChanged();
}

void SupplyBackend::resetState()
{
  setConnectionState(false, false);
  setMasterOutputState(false);
  for(auto *state : std::as_const(_channels)) {
    state->reset();
  }
  if(!_deviceIdentity.isEmpty()) {
    _deviceIdentity.clear();
    emit deviceIdentityChanged();
  }
}

/**
 * @brief SupplyBackend::shutdown
 *
 * Same handshake as MainWindow::closeEvent, and just as strictly once-only:
 * cleanup() moves the driver onto this thread, after which a second
 * BlockingQueuedConnection call would target its own thread and deadlock.
 */
void SupplyBackend::shutdown()
{
  if(_shutdownDone) {
    return;
  }
  _shutdownDone = true;
  _ctrl.abortPendingIo();
  QMetaObject::invokeMethod(&_ctrl, [this]() {
    _ctrl.setPeriodicUpdateEnable(false);
    _ctrl.deviceDisconnect();
  });
  QMetaObject::invokeMethod(&_ctrl, [this]() { _ctrl.cleanup(); }, Qt::BlockingQueuedConnection);
}

ChannelState *SupplyBackend::channelFor(HMCSupplyCtrl::HMCChannel chNr) const
{
  if(!HMCSupplyCtrl::isValidChannel(chNr)) {
    return nullptr;
  }
  return _channels.at(static_cast<int>(chNr) - 1);
}
