#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
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
  void closeEvent(QCloseEvent *evt);

private:
  Ui::MainWindow *ui;
  HMCSupplyCtrl _hmcCtrl;
  QTimer _devUpdateTmr;

  void createConnections();
  void registerMetaTypes();

private slots:
  void btnConnectClicked();
  void btnDisconnectClicked();
  void btnMasterOutEnableClicked();

  void deviceConnected();
  void deviceDisconnected();

signals:
  void cleanupRequst();
  void deviceConnect(const QHostAddress &addr);
  void deviceDisconnect();
  void setMasterOutEnable(bool enable);
};
#endif // MAINWINDOW_H
