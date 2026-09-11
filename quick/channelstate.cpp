#include "channelstate.h"
#include "hmcappglobal.h"

/**
 * @brief ChannelState::ChannelState
 * @param number channel number, 1-based as on the instrument
 * @param parent
 */
ChannelState::ChannelState(int number, QObject *parent)
    : QObject{parent}
    , _number(number)
    , _maxVoltage(FALLBACK_MAX_VOLTAGE)
    , _maxCurrent(FALLBACK_MAX_CURRENT)
{
}

void ChannelState::setVoltage(double voltage)
{
  if(_hasVoltage && qFuzzyCompare(_voltage, voltage)) {
    return;
  }
  _hasVoltage = true;
  _voltage = voltage;
  emit measurementChanged();
}

void ChannelState::setCurrent(double current)
{
  if(_hasCurrent && qFuzzyCompare(_current, current)) {
    return;
  }
  _hasCurrent = true;
  _current = current;
  emit measurementChanged();
}

void ChannelState::setTargetVoltage(double voltage)
{
  _hasTargetVoltage = true;
  _targetVoltage = voltage;
  emit targetsChanged();
}

void ChannelState::setTargetCurrent(double current)
{
  _hasTargetCurrent = true;
  _targetCurrent = current;
  emit targetsChanged();
}

void ChannelState::setLimits(double maxVoltage, double maxCurrent)
{
  _maxVoltage = maxVoltage;
  _maxCurrent = maxCurrent;
  emit limitsChanged();
}

void ChannelState::setOutputEnabled(bool enabled)
{
  if(_outputEnabled == enabled) {
    return;
  }
  _outputEnabled = enabled;
  emit outputEnabledChanged();
}

/**
 * @brief ChannelState::reset
 *
 * Back to "nothing known", so the UI shows placeholders rather than numbers from a
 * link that is gone. Limits are kept: they describe the instrument, not the session.
 */
void ChannelState::reset()
{
  _hasVoltage = _hasCurrent = false;
  _voltage = _current = 0.0;
  _hasTargetVoltage = _hasTargetCurrent = false;
  _targetVoltage = _targetCurrent = 0.0;
  emit measurementChanged();
  emit targetsChanged();
  setOutputEnabled(false);
}
