#include <QCloseEvent>
#include <QHostAddress>
#include "mainwindow.h"
#include "./ui_mainwindow.h"

#define HMC_UPDATE_INTERVAL 500

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  createConnections();

  registerMetaTypes();

  ui->wdgChannel1->setupWidget(&_hmcCtrl, HMCSupplyCtrl::Channel1, "Channel 1");
  ui->wdgChannel2->setupWidget(&_hmcCtrl, HMCSupplyCtrl::Channel2, "Channel 2");
  ui->wdgChannel3->setupWidget(&_hmcCtrl, HMCSupplyCtrl::Channel3, "Channel 3");
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::closeEvent(QCloseEvent *evt)
{
  btnDisconnectClicked();
  emit cleanupRequst();
  evt->accept();
}

void MainWindow::createConnections()
{
  connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::btnConnectClicked);
  connect(ui->btnDisconnect, &QPushButton::clicked, this, &MainWindow::btnDisconnectClicked);
  connect(ui->btnMasterOutEnable, &QToolButton::clicked, this, &MainWindow::btnMasterOutEnableClicked);

  connect(&_devUpdateTmr, &QTimer::timeout, ui->wdgChannel1, &HMCChannelWidget::refreshHMCData);
  connect(&_devUpdateTmr, &QTimer::timeout, ui->wdgChannel2, &HMCChannelWidget::refreshHMCData);
  connect(&_devUpdateTmr, &QTimer::timeout, ui->wdgChannel3, &HMCChannelWidget::refreshHMCData);

  connect(&_hmcCtrl, &HMCSupplyCtrl::deviceConnected, this, &MainWindow::deviceConnected);
  connect(&_hmcCtrl, &HMCSupplyCtrl::deviceDisconnected, this, &MainWindow::deviceDisconnected);
  connect(this, &MainWindow::cleanupRequst, &_hmcCtrl, &HMCSupplyCtrl::cleanup, Qt::BlockingQueuedConnection);
  connect(this, &MainWindow::deviceConnect, &_hmcCtrl, &HMCSupplyCtrl::deviceConnect);
  connect(this, &MainWindow::deviceDisconnect, &_hmcCtrl, &HMCSupplyCtrl::deviceDisconnect);
  connect(this, &MainWindow::setMasterOutEnable, &_hmcCtrl, &HMCSupplyCtrl::setMasterOutEnable);
}

void MainWindow::registerMetaTypes()
{
  qRegisterMetaType<QHostAddress>("QHostAddress");
  qRegisterMetaType<HMCSupplyCtrl::HMCChannel>("HMCSupplyCtrl::HMCChannel");
}

void MainWindow::btnConnectClicked()
{
  emit deviceConnect(QHostAddress("192.168.12.100"));
}

void MainWindow::btnDisconnectClicked()
{
  _devUpdateTmr.stop();
  emit deviceDisconnect();
}

void MainWindow::btnMasterOutEnableClicked()
{
  emit setMasterOutEnable(ui->btnMasterOutEnable->isChecked());
}

void MainWindow::deviceConnected()
{
  _devUpdateTmr.start(HMC_UPDATE_INTERVAL);
}

void MainWindow::deviceDisconnected()
{
}

