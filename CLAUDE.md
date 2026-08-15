# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

HMC8043App is a Qt 6 (Widgets) desktop application for remotely controlling a Rohde & Schwarz
HMC8043 triple-channel bench power supply over its SCPI/LXI network interface (raw TCP on port
5025). It lets a user connect to a supply by IP address, view/set each channel's voltage and
current, toggle per-channel and master output enable, and see live measured values.

## Build

Standard Qt 6 CMake project.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Requires Qt 6 (or Qt 5, still supported by the CMakeLists.txt) with the `Widgets` and `Network`
components. `CMakeLists.txt.user` is a local Qt Creator artifact — don't edit it as part of
feature work. There is no test suite, lint config, or CI in this repo.

## Architecture

Four-layer split, small enough to hold in your head, but the threading boundary matters:

- **`HMCSupplyCtrl`** (`hmcsupplyctrl.h/.cpp`) — the device driver. Owns the `QTcpSocket` and
  speaks SCPI text commands (`VOLT?`, `CURR?`, `OUTP:CHAN ON`, `INST OUT<n>`, etc.) synchronously
  using `waitForBytesWritten`/`waitForReadyRead`. **It moves itself to its own `QThread`
  (`_thread`) in its constructor** so these blocking socket calls never stall the UI thread. All
  interaction with it from the GUI thread must go through queued signal/slot connections (see
  `MainWindow::createConnections`) — never call its public methods directly from GUI code, and
  never assume synchronous return values across the thread boundary.
  - The instrument protocol is stateful: only one channel can be "selected" at a time
    (`channelSelect()` tracks `_selChannel` and issues `INST OUT<n>` only when it changes) before
    per-channel commands like `VOLT`/`CURR`/`OUTP:CHAN` are valid. Any new channel-scoped command
    must go through `sendChannelCmdAndParseReply` to preserve this invariant.
  - A single-shot `QTimer` (`_periodicUpdateTmr`) re-arms itself at the end of
    `onPeriodicTimer()` to poll live voltage/current every `PERIODIC_UPDATE_INTERVAL_MS` (500ms)
    while `_periodicUpdateEnable` is set — deliberately not a repeating timer, so a slow/blocked
    socket round-trip can't cause overlapping polls.
  - `HMCChannel` is a `Q_ENUM` with `NoChannel = 0` and channels numbered 1-3; array storage uses
    `CH_TO_ARRAY_INDEX(x)` (`x - 1`) to map channel enum to `std::array` index. `hmcChannels` is
    the canonical iteration order (`Channel1, Channel2, Channel3`).
  - Cleanup is intentionally asymmetric: `cleanup()` (invoked via `Qt::BlockingQueuedConnection`
    from `MainWindow::closeEvent`) moves the object back to the GUI thread so the destructor
    (which calls `_thread.quit()`/`_thread.wait()`) runs safely from the thread that owns it.

- **`MainWindow`** (`mainwindow.h/.cpp`) — top-level window. Owns the `HMCSupplyCtrl` instance
  directly (not via pointer/thread management at this layer) and wires GUI actions to it purely
  through signals/slots, never direct calls, since `HMCSupplyCtrl` lives on another thread.
  Manages connect/disconnect lifecycle and enables/disables channel widgets and the master-out
  button based on connection state. Host address is persisted via `QSettings` under key
  `SKEY_HOSTADDR` (`hmcappglobal.h`), defaulting to `DEFAULT_HOST` (`192.168.12.100`).

- **`HMCChannelWidget`** (`hmcchannelwidget.h/.cpp`) — one instance per physical channel,
  configured post-construction via `setupWidget(hmcCtrl, channel, name)` rather than through the
  constructor (so it can be placed in the `.ui` file as a plain widget and wired up afterward). It
  connects directly to the shared `HMCSupplyCtrl` instance's per-channel signals, filtering on its
  own `_channel` to ignore updates for other channels. Voltage/current entry goes through
  `ValueSetDialog`, invoked from `btnSetVoltageClicked`/`btnSetCurrentClicked`, with per-widget
  static preset lists (`_voltagePresets`, `_currentPresets`).

- **`ValueSetDialog`** / **`HostAddrConfigDialog`** — small reusable modal dialogs (numeric
  value entry with unit label and presets; host address entry) with no dependency on
  `HMCSupplyCtrl`, driven purely through their public getter/setter API and `QDialog::exec()`.

### Threading model recap

GUI thread (`MainWindow`, `HMCChannelWidget`, dialogs) <-> queued signals/slots <-> worker thread
(`HMCSupplyCtrl`, blocking SCPI I/O). When adding new device functionality, add a slot to
`HMCSupplyCtrl` plus a matching signal on the caller side (mirroring the existing
`setChannelVoltage`/`channelTargetVoltageChanged` pattern) rather than exposing new synchronous
getters that would be called across threads.

### UI files

`.ui` files (`mainwindow.ui`, `hmcchannelwidget.ui`, `valuesetdialog.ui`,
`hostaddrconfigdialog.ui`) are edited with Qt Designer/Creator; `CMAKE_AUTOUIC` generates
`ui_*.h` headers at build time — don't hand-edit generated `ui_*.h` files.

### Dark theme

`main.cpp` sets a hardcoded dark `QPalette` on the Fusion style application-wide; there is no
light-theme path, so new widgets should rely on palette roles rather than hardcoded colors.
