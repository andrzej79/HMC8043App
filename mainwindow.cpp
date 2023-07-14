#include <QCloseEvent>
#include <QHostAddress>
#include <QMessageBox>
#include <QSettings>
#include "mainwindow.h"
#include "hmcappglobal.h"
#include "hostaddrconfigdialog.h"
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

  ui->wdgChannel1->setEnabled(false);
  ui->wdgChannel2->setEnabled(false);
  ui->wdgChannel3->setEnabled(false);
  ui->btnMasterOutEnable->setEnabled(false);
  ui->btnDisconnect->setEnabled(false);
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
  connect(ui->actionSetHostAddress, &QAction::triggered, this, &MainWindow::actSetHostAddress);

  connect(&_hmcCtrl, &HMCSupplyCtrl::deviceConnected, this, &MainWindow::deviceConnected);
  connect(&_hmcCtrl, &HMCSupplyCtrl::deviceConnectionFailed, this, &MainWindow::deviceConnectionFailed);
  connect(&_hmcCtrl, &HMCSupplyCtrl::deviceDisconnected, this, &MainWindow::deviceDisconnected);
  connect(&_hmcCtrl, &HMCSupplyCtrl::masterOutEnableChanged, this, &MainWindow::masterOutEnableChanged);

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
  ui->btnConnect->setEnabled(false);
  QSettings s;
  QHostAddress addr;

  addr = QHostAddress(s.value(SKEY_HOSTADDR, DEFAULT_HOST).toString());

  emit deviceConnect(addr);
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
 * @brief MainWindow::actSetHostAddress
 */
void MainWindow::actSetHostAddress()
{
  QSettings s;
  qDebug() << Q_FUNC_INFO;
  HostAddrConfigDialog *dlg = new HostAddrConfigDialog(this);
  dlg->setCurrentAddr(s.value(SKEY_HOSTADDR, DEFAULT_HOST).toString());

  auto res = dlg->exec();
  if(res == QDialog::Accepted) {
    QHostAddress addr(dlg->getAddr());
    qDebug() << Q_FUNC_INFO << "configuring Host Address:" << addr << addr.isNull() << addr.isUniqueLocalUnicast();
    if(addr.isNull() == false) {
      qDebug() << Q_FUNC_INFO << "new configured Host Address:" << addr;
      s.setValue(SKEY_HOSTADDR, addr.toString());
    }
  }

  dlg->deleteLater();
}

/**
 * @brief MainWindow::deviceConnected
 */
void MainWindow::deviceConnected()
{
  emit setPeriodicUpdateEnable(true);
  ui->wdgChannel1->setEnabled(true);
  ui->wdgChannel2->setEnabled(true);
  ui->wdgChannel3->setEnabled(true);
  ui->btnMasterOutEnable->setEnabled(true);
  ui->btnConnect->setEnabled(false);
  ui->btnDisconnect->setEnabled(true);
}

/**
 * @brief MainWindow::deviceConnectionFailed
 */
void MainWindow::deviceConnectionFailed()
{
  QMessageBox::critical(this, "Error!", "Device Connection Failed!");
  ui->btnConnect->setEnabled(true);
}

/**
 * @brief MainWindow::deviceDisconnected
 */
void MainWindow::deviceDisconnected()
{
  emit setPeriodicUpdateEnable(false);
  ui->wdgChannel1->setEnabled(false);
  ui->wdgChannel2->setEnabled(false);
  ui->wdgChannel3->setEnabled(false);
  ui->btnMasterOutEnable->setEnabled(false);
  ui->btnMasterOutEnable->setChecked(false);
  ui->btnConnect->setEnabled(true);
  ui->btnDisconnect->setEnabled(false);
}

/**
 * @brief MainWindow::masterOutEnableChanged
 * @param enabled
 */
void MainWindow::masterOutEnableChanged(bool enabled)
{
  QSignalBlocker block(ui->btnMasterOutEnable);
  ui->btnMasterOutEnable->setChecked(enabled);
}

