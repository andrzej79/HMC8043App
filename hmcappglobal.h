#ifndef HMCAPPGLOBAL_H
#define HMCAPPGLOBAL_H

#define SKEY_HOSTADDR "HostAddress"
#define DEFAULT_HOST  "192.168.12.100"

/* Fallback per-channel limits, used only when the instrument does not answer
 * "VOLT? MAX" / "CURR? MAX". Datasheet figures for the HMC8043. */
#define FALLBACK_MAX_VOLTAGE 32.05
#define FALLBACK_MAX_CURRENT 3.0

#endif // HMCAPPGLOBAL_H
