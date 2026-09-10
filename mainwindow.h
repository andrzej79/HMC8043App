#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "hmcsupplyctrl.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

protected:
  void closeEvent(QCloseEvent *evt) override;

private:
  /* Declared before ui so the controller outlives the channel widgets that hold
   * a pointer to it (members are destroyed in reverse declaration order, and the
   * child widgets are only deleted later, by ~QObject). */
  HMCSupplyCtrl _hmcCtrl;
  Ui::MainWindow *ui;
  bool _shutdownRequested = false;
  bool _connErrorReported = false;

  void createConnections();
  void registerMetaTypes();

private slots:
  void btnConnectClicked();
  void btnDisconnectClicked();
  void btnMasterOutEnableClicked();
  void actSetHostAddress();

  void deviceConnected();
  void deviceConnectionFailed();
  void deviceConnectionError();
  void deviceDisconnected();
  void masterOutEnableChanged(bool enabled);

signals:
  void cleanupRequst();
  void deviceConnect(const QHostAddress &addr);
  void deviceDisconnect();
  void setMasterOutEnable(bool enable);
  void setPeriodicUpdateEnable(bool enable);
};
#endif // MAINWINDOW_H
