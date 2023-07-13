#include <QCloseEvent>
#include <QHostAddress>
#include "mainwindow.h"
#include "./ui_mainwindow.h"

/**
 * @brief MainWindow::MainWindow
 * @param parent
 */
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

/**
 * @brief MainWindow::~MainWindow
 */
MainWindow::~MainWindow()
{
  delete ui;
}

/**
 * @brief MainWindow::closeEvent
 * @param evt
 */
void MainWindow::closeEvent(QCloseEvent *evt)
{
  btnDisconnectClicked();
  emit cleanupRequst();
  evt->accept();
}

/**
 * @brief MainWindow::createConnections
 */
void MainWindow::createConnections()
{
  connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::btnConnectClicked);
  connect(ui->btnDisconnect, &QPushButton::clicked, this, &MainWindow::btnDisconnectClicked);
  connect(ui->btnMasterOutEnable, &QToolButton::clicked, this, &MainWindow::btnMasterOutEnableClicked);

  connect(&_hmcCtrl, &HMCSupplyCtrl::deviceConnected, this, &MainWindow::deviceConnected);
  connect(&_hmcCtrl, &HMCSupplyCtrl::deviceDisconnected, this, &MainWindow::deviceDisconnected);

  connect(this, &MainWindow::cleanupRequst, &_hmcCtrl, &HMCSupplyCtrl::cleanup, Qt::BlockingQueuedConnection);
  connect(this, &MainWindow::deviceConnect, &_hmcCtrl, &HMCSupplyCtrl::deviceConnect);
  connect(this, &MainWindow::deviceDisconnect, &_hmcCtrl, &HMCSupplyCtrl::deviceDisconnect);
  connect(this, &MainWindow::setMasterOutEnable, &_hmcCtrl, &HMCSupplyCtrl::setMasterOutEnable);
  connect(this, &MainWindow::setPeriodicUpdateEnable, &_hmcCtrl, &HMCSupplyCtrl::setPeriodicUpdateEnable);
}

/**
 * @brief MainWindow::registerMetaTypes
 */
void MainWindow::registerMetaTypes()
{
  qRegisterMetaType<QHostAddress>("QHostAddress");
  qRegisterMetaType<HMCSupplyCtrl::HMCChannel>("HMCSupplyCtrl::HMCChannel");
}

/**
 * @brief MainWindow::btnConnectClicked
 */
void MainWindow::btnConnectClicked()
{
  emit deviceConnect(QHostAddress("192.168.12.100"));
}

/**
 * @brief MainWindow::btnDisconnectClicked
 */
void MainWindow::btnDisconnectClicked()
{
  emit setPeriodicUpdateEnable(false);
  emit deviceDisconnect();
}

/**
 * @brief MainWindow::btnMasterOutEnableClicked
 */
void MainWindow::btnMasterOutEnableClicked()
{
  emit setMasterOutEnable(ui->btnMasterOutEnable->isChecked());
}

/**
 * @brief MainWindow::deviceConnected
 */
void MainWindow::deviceConnected()
{
  emit setPeriodicUpdateEnable(true);
}

/**
 * @brief MainWindow::deviceDisconnected
 */
void MainWindow::deviceDisconnected()
{
  emit setPeriodicUpdateEnable(false);
}

