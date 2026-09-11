#ifndef SUPPLYBACKEND_H
#define SUPPLYBACKEND_H

#include <QList>
#include <QObject>
#include <QQmlListProperty>
#include <QtQmlIntegration/qqmlintegration.h>
#include "channelstate.h"
#include "hmcsupplyctrl.h"

/* The QML-facing side of the application. Lives on the GUI thread, owns the driver,
 * and mirrors its state into properties. Every call into the driver is marshalled onto
 * the driver's thread with QMetaObject::invokeMethod - QML never sees HMCSupplyCtrl. */
class SupplyBackend : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(QQmlListProperty<ChannelState> channels READ channels CONSTANT)
  Q_PROPERTY(bool connected READ isConnected NOTIFY connectionStateChanged)
  Q_PROPERTY(bool connecting READ isConnecting NOTIFY connectionStateChanged)
  Q_PROPERTY(bool masterOutputEnabled READ masterOutputEnabled NOTIFY masterOutputEnabledChanged)
  Q_PROPERTY(QString hostAddress READ hostAddress WRITE setHostAddress NOTIFY hostAddressChanged)
  Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY autoConnectChanged)
  Q_PROPERTY(QString deviceIdentity READ deviceIdentity NOTIFY deviceIdentityChanged)
  Q_PROPERTY(QList<double> voltagePresets READ voltagePresets CONSTANT)
  Q_PROPERTY(QList<double> currentPresets READ currentPresets CONSTANT)

public:
  explicit SupplyBackend(QObject *parent = nullptr);
  ~SupplyBackend() override;

  QQmlListProperty<ChannelState> channels();
  bool isConnected() const { return _connected; }
  bool isConnecting() const { return _connecting; }
  bool masterOutputEnabled() const { return _masterOutputEnabled; }
  QString hostAddress() const { return _hostAddress; }
  void setHostAddress(const QString &address);
  bool autoConnect() const { return _autoConnect; }
  void setAutoConnect(bool enable);
  QString deviceIdentity() const { return _deviceIdentity; }
  QList<double> voltagePresets() const;
  QList<double> currentPresets() const;

  Q_INVOKABLE void connectDevice();
  Q_INVOKABLE void disconnectDevice();
  Q_INVOKABLE void setMasterOutputEnabled(bool enable);
  Q_INVOKABLE void setChannelVoltage(int channel, double voltage);
  Q_INVOKABLE void setChannelCurrent(int channel, double current);
  Q_INVOKABLE void setChannelOutputEnabled(int channel, bool enable);
  Q_INVOKABLE bool isValidHostAddress(const QString &address) const;
  Q_INVOKABLE double parseNumber(const QString &text) const;

signals:
  void connectionStateChanged();
  void masterOutputEnabledChanged();
  void hostAddressChanged();
  void autoConnectChanged();
  void deviceIdentityChanged();
  void errorOccurred(const QString &title, const QString &message);

private:
  void createConnections();
  void setConnectionState(bool connected, bool connecting);
  void setMasterOutputState(bool enabled);
  void resetState();
  void shutdown();
  ChannelState *channelFor(HMCSupplyCtrl::HMCChannel chNr) const;

  HMCSupplyCtrl _ctrl;
  QList<ChannelState *> _channels;
  bool _connected = false;
  bool _connecting = false;
  bool _masterOutputEnabled = false;
  bool _errorReported = false;
  bool _shutdownDone = false;
  bool _autoConnect = false;
  QString _hostAddress;
  QString _deviceIdentity;
};

#endif // SUPPLYBACKEND_H
