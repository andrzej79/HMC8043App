#ifndef HMCPRESETS_H
#define HMCPRESETS_H

#include <QList>

/* Quick-pick values offered by both UIs' value entry. */
inline const QList<double> HMC_VOLTAGE_PRESETS{1.0, 3.3, 5.0, 9.0, 12.0, 15.0, 24.0, 30.0};
inline const QList<double> HMC_CURRENT_PRESETS{0.1, 0.25, 0.5, 1.0, 2.0, 3.0};

#endif // HMCPRESETS_H
