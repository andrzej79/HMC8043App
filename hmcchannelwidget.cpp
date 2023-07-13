#include <QMetaEnum>
#include <QDialog>
#include <algorithm>
#include "hmcchannelwidget.h"
#include "valuesetdialog.h"
#include "ui_hmcchannelwidget.h"

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

  ui->lcdCurrent->setStyleSheet("QLCDNumber{color: red;}");
  ui->lcdVoltage->setStyleSheet("QLCDNumber{color: rgb(0,128,0)}");
}

/**
 * @brief HMCChannelWidget::createConnections
 */
void HMCChannelWidget::createConnections()
{
  connect(ui->btnSetVoltage, &QPushButton::clicked, this, &HMCChannelWidget::btnSetVoltageClicked);
  connect(ui->btnSetCurrent, &QPushButton::clicked, this, &HMCChannelWidget::btnSetCurrentClicked);
  connect(ui->cbOutEnable, &QCheckBox::clicked, this, &HMCChannelWidget::cbChannelOutEnableClicked);

  connect(_hmcCtrl, &HMCSupplyCtrl::channelCurrentChanged, this, &HMCChannelWidget::channelCurrentChanged);
  connect(_hmcCtrl, &HMCSupplyCtrl::channelVoltageChanged, this, &HMCChannelWidget::channelVoltageChanged);

  connect(this, &HMCChannelWidget::updateChannelVoltage, _hmcCtrl, &HMCSupplyCtrl::updateChannelVoltage);
  connect(this, &HMCChannelWidget::updateChannelCurrent, _hmcCtrl, &HMCSupplyCtrl::updateChannelCurrent);
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
 * @brief HMCChannelWidget::channelVoltageChanged
 * @param chNr
 * @param voltage
 */
void HMCChannelWidget::channelVoltageChanged(HMCSupplyCtrl::HMCChannel chNr, double voltage)
{
  if(chNr != _channel) {
    return;
  }
  ui->lcdVoltage->display(QString::asprintf("%.2f", voltage));
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
  ui->lcdCurrent->display(QString::asprintf("%.3f", current));
}
