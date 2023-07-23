#include <QMetaEnum>
#include <QDialog>
#include <algorithm>
#include "hmcchannelwidget.h"
#include "valuesetdialog.h"
#include "ui_hmcchannelwidget.h"

const QList<double> HMCChannelWidget::_voltagePresets{1.0, 3.3, 5.0, 9.0, 12.0, 15.0, 24.0, 30.0};
const QList<double> HMCChannelWidget::_currentPresets{0.1, 0.25, 0.5, 1.0, 2.0, 3.0};


/**
 * @brief HMCChannelWidget::HMCChannelWidget
 * @param parent
 */
HMCChannelWidget::HMCChannelWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HMCChannelWidget)
{
  ui->setupUi(this);
}

/**
 * @brief HMCChannelWidget::~HMCChannelWidget
 */
HMCChannelWidget::~HMCChannelWidget()
{
  delete ui;
}

/**
 * @brief HMCChannelWidget::setupWidget
 * @param hmcCtrl
 * @param channel
 * @param channelName
 */
void HMCChannelWidget::setupWidget(HMCSupplyCtrl *hmcCtrl, HMCSupplyCtrl::HMCChannel channel, const QString &channelName)
{
  Q_ASSERT(hmcCtrl != nullptr);
  _hmcCtrl = hmcCtrl;
  _channel = channel;
  _channelName = channelName;

  ui->gbChannel->setTitle(_channelName);

  createConnections();

  ui->lcdCurrent->setStyleSheet("QLCDNumber{color: red; background-color: rgb(40,40,40);}");
  ui->lcdVoltage->setStyleSheet("QLCDNumber{color: rgb(0,128,0); background-color: rgb(40,40,40);}");
  ui->lcdPower->setStyleSheet("QLCDNumber{color: rgb(255,128,0); background-color: rgb(40,40,40);}");

  deviceDisconnected();
}

/**
 * @brief HMCChannelWidget::createConnections
 */
void HMCChannelWidget::createConnections()
{
  connect(ui->btnSetVoltage, &QPushButton::clicked, this, &HMCChannelWidget::btnSetVoltageClicked);
  connect(ui->btnSetCurrent, &QPushButton::clicked, this, &HMCChannelWidget::btnSetCurrentClicked);
  connect(ui->cbOutEnable, &QCheckBox::clicked, this, &HMCChannelWidget::cbChannelOutEnableClicked);

  connect(_hmcCtrl, &HMCSupplyCtrl::deviceDisconnected, this, &HMCChannelWidget::deviceDisconnected);
  connect(_hmcCtrl, &HMCSupplyCtrl::channelCurrentChanged, this, &HMCChannelWidget::channelCurrentChanged);
  connect(_hmcCtrl, &HMCSupplyCtrl::channelVoltageChanged, this, &HMCChannelWidget::channelVoltageChanged);
  connect(_hmcCtrl, &HMCSupplyCtrl::channelTargetCurrentChanged, this, &HMCChannelWidget::channelTargetCurrentChanged);
  connect(_hmcCtrl, &HMCSupplyCtrl::channelTargetVoltageChanged, this, &HMCChannelWidget::channelTargetVoltageChanged);
  connect(_hmcCtrl, &HMCSupplyCtrl::channelOutEnableChanged, this, &HMCChannelWidget::channelOutEnableChanged);

  connect(this, &HMCChannelWidget::setChannelVoltage, _hmcCtrl, &HMCSupplyCtrl::setChannelVoltage);
  connect(this, &HMCChannelWidget::setChannelCurrent, _hmcCtrl, &HMCSupplyCtrl::setChannelCurrent);
  connect(this, &HMCChannelWidget::setChannelOutEnable, _hmcCtrl, &HMCSupplyCtrl::setChannelOutEnable);

}

/**
 * @brief HMCChannelWidget::btnSetVoltageClicked
 */
void HMCChannelWidget::btnSetVoltageClicked()
{
  ValueSetDialog *dlg = new ValueSetDialog(this);
  dlg->setUnitString("V");
  dlg->setWindowTitle("Set Voltage Value");
  dlg->setPresets(_voltagePresets, 3);
  dlg->setValue(_hmcCtrl->getChannelTargetVoltage(_channel));
  auto dlgRes = dlg->exec();
  if(dlgRes == QDialog::Accepted) {
    auto v = std::clamp(dlg->getValue(), 0.0, 99.0);
    emit setChannelVoltage(_channel, v);
  }
  dlg->deleteLater();
}

/**
 * @brief HMCChannelWidget::btnSetCurrentClicked
 */
void HMCChannelWidget::btnSetCurrentClicked()
{
  ValueSetDialog *dlg = new ValueSetDialog(this);
  dlg->setUnitString("A");
  dlg->setWindowTitle("Set Current Value");
  dlg->setPresets(_currentPresets, 3);
  dlg->setValue(_hmcCtrl->getChannelTargetCurrent(_channel));
  auto dlgRes = dlg->exec();
  if(dlgRes == QDialog::Accepted) {
    auto v = std::clamp(dlg->getValue(), 0.0, 5.0);
    emit setChannelCurrent(_channel, v);
  }
  dlg->deleteLater();
}

/**
 * @brief HMCChannelWidget::cbChannelOutEnableClicked
 */
void HMCChannelWidget::cbChannelOutEnableClicked()
{
  emit setChannelOutEnable(_channel, ui->cbOutEnable->isChecked());
}

/**
 * @brief HMCChannelWidget::deviceDisconnected
 */
void HMCChannelWidget::deviceDisconnected()
{
  ui->lcdVoltage->display("-.---");
  ui->lcdCurrent->display("-.---");
  ui->lcdPower->display("-.-");
  ui->lbTargetCurrent->setText("-.---");
  ui->lbTargetVoltage->setText("-.---");
  ui->cbOutEnable->setChecked(false);
}

/**
 * @brief HMCChannelWidget::channelVoltageChanged
 * @param chNr
 * @param voltage
 */
void HMCChannelWidget::channelVoltageChanged(HMCSupplyCtrl::HMCChannel chNr, double voltage)
{
  if(chNr != _channel) {
    return;
  }
  _voltage = voltage;
  ui->lcdVoltage->display(QString::asprintf("%.2f", voltage));
  ui->lcdPower->display(QString::number(_voltage * _current, 'f', 1));
}

/**
 * @brief HMCChannelWidget::channelCurrentChanged
 * @param chNr
 * @param current
 */
void HMCChannelWidget::channelCurrentChanged(HMCSupplyCtrl::HMCChannel chNr, double current)
{
  if(chNr != _channel) {
    return;
  }
  _current = current;

  if(_current >= 1.0) {
    ui->lcdCurrent->display(QString::asprintf("%.3f", current));
    ui->lbCurrUnit->setText("A");
  } else {
    ui->lcdCurrent->display(QString::asprintf("%.1f", current * 1e3));
    ui->lbCurrUnit->setText("mA");
  }
  ui->lcdPower->display(QString::number(_voltage * _current, 'f', 1));
}

/**
 * @brief HMCChannelWidget::channelOutEnableChanged
 * @param chNr
 * @param enabled
 */
void HMCChannelWidget::channelOutEnableChanged(HMCSupplyCtrl::HMCChannel chNr, bool enabled)
{
  if(chNr != _channel) {
    return;
  }
  QSignalBlocker block(ui->cbOutEnable);
  ui->cbOutEnable->setChecked(enabled);
}

/**
 * @brief HMCChannelWidget::channelTargetVoltageChanged
 * @param chNr
 * @param voltage
 */
void HMCChannelWidget::channelTargetVoltageChanged(HMCSupplyCtrl::HMCChannel chNr, double voltage)
{
  if(chNr != _channel) {
    return;
  }
  ui->lbTargetVoltage->setText(QString::asprintf("%.3f V", voltage));
}

/**
 * @brief HMCChannelWidget::channelTargetCurrentChanged
 * @param chNr
 * @param current
 */
void HMCChannelWidget::channelTargetCurrentChanged(HMCSupplyCtrl::HMCChannel chNr, double current)
{
  if(chNr != _channel) {
    return;
  }
  ui->lbTargetCurrent->setText(QString::asprintf("%.3f A", current));
}


