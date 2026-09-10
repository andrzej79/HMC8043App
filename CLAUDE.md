# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

HMC8043App is a Qt 6 (Widgets) desktop application for remotely controlling a Rohde & Schwarz
HMC8043 triple-channel bench power supply over its SCPI/LXI network interface (raw TCP on port
5025). It lets a user connect to a supply by IP address, view/set each channel's voltage and
current, toggle per-channel and master output enable, and see live measured values.

## Build and run

Standard Qt CMake project. Note the CMake project/target name is **`HMCSupplyApp`**, not
`HMC8043App` (that is only the directory and `QApplication::applicationName`).

All build output belongs under `./build/`. **Claude Code must always build into
`./build/claude/`**, never into `./build/` directly, so agent builds never collide with the
user's own Qt Creator builds:

```bash
cmake -S . -B build/claude -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$HOME/Qt/6.8.5/macos"
cmake --build build/claude
open build/claude/HMCSupplyApp.app   # macOS (MACOSX_BUNDLE TRUE); ./build/claude/HMCSupplyApp elsewhere
```

`CMAKE_PREFIX_PATH` is required — Qt is installed under `~/Qt/<version>/macos` and is not on the
default CMake search path on this machine.

**Use Qt 6.8.x, not the 6.2.13 kit Qt Creator is configured with.** Under the current macOS SDK
(MacOSX26.5) every source still compiles against 6.2.13, but linking fails with
`ld: framework 'AGL' not found` — Apple dropped AGL and Qt 6.2.x still references it. That is an
SDK/Qt-version mismatch, not a defect in this code, so don't chase it in the sources. Verified
working: Qt 6.8.5.

Requires Qt 6 (or Qt 5, still supported by `CMakeLists.txt` — see the `QT_VERSION` branches in
`CMakeLists.txt` and `valuesetdialog.cpp`) with the `Widgets` and `Network` components. New
source files must be added to `PROJECT_SOURCES` in `CMakeLists.txt` by hand.

`build/` is gitignored and already contains Qt Creator's own configured build dirs
(`Desktop_Qt_6_2_13_clang_64bit-{Debug,Release}`); leave those alone, and don't edit the local
Qt Creator artifact `CMakeLists.txt.user` as part of feature work. There is no test suite, lint
config, or CI in this repo, so verify changes by building and, when hardware is available,
running against a supply.

## Architecture

Four-layer split, small enough to hold in your head, but the threading boundary matters:

- **`HMCSupplyCtrl`** (`hmcsupplyctrl.h/.cpp`) — the device driver. Owns the `QTcpSocket` and
  speaks SCPI text commands (`VOLT?`, `CURR?`, `OUTP:CHAN ON`, `INST OUT<n>`, etc.) synchronously
  using `waitForBytesWritten`/`waitForReadyRead` with a 4s `SOCK_TIMEOUT_MS`. **It moves itself
  to its own `QThread` (`_thread`) in its constructor** so these blocking socket calls never
  stall the UI thread. Drive it from the GUI thread through queued signal/slot connections (see
  `MainWindow::createConnections` and `HMCChannelWidget::createConnections`) — a slot invoked
  this way runs on the worker thread, which is what makes the blocking I/O safe.
  - **The one exception is `abortPendingIo()`**, which only stores into a `std::atomic<bool>` and
    is therefore safe to call directly from any thread. It exists because `waitFor*` calls don't
    dispatch queued slots: a disconnect or shutdown request would otherwise sit behind an
    in-flight wait for its full 4 s timeout. All socket waits go through
    `waitForWriteInterruptible`/`waitForReadInterruptible`, which wait in `ABORT_POLL_SLICE_MS`
    (100 ms) slices and bail out when the flag is set. `MainWindow::btnDisconnectClicked` calls it
    before emitting anything; `deviceConnect()` clears it. Don't add a second directly-callable
    member — anything that isn't a lone atomic store belongs behind a queued slot.
  - `sendCmdLine()` decides whether to read a reply purely from `cmd.endsWith("?")`. A query
    without the `?` writes and returns an empty string; the reply then desynchronizes the stream
    for the *next* command. Every new query string must keep the `?` suffix. Replies are framed
    by `readReplyLine()`, which reads until `'\n'` or the deadline — TCP gives no message
    boundaries and a truncated number still parses, so never go back to a bare `readAll()`.
    `sendCmdLine` also drops anything left in the read buffer before writing, so a reply that
    arrived after its own timeout cannot be consumed as the next command's answer.
  - The instrument protocol is stateful: only one channel can be "selected" at a time
    (`channelSelect()` tracks `_selChannel` and issues `INST OUT<n>` only when it changes) before
    per-channel commands like `VOLT`/`CURR`/`OUTP:CHAN` are valid. Any new channel-scoped *query*
    must go through `sendChannelCmdAndParseReply`, and any channel-scoped *write* must call
    `channelSelect()` first **and honour its return value** (as
    `setChannelVoltage`/`setChannelCurrent`/`setChannelOutEnable` do), to preserve this invariant.
    `channelSelect()` caches `_selChannel` only on success and resets it to `NoChannel` on
    failure — caching a select that never reached the instrument would silently route every
    later command for that channel to whatever output is actually selected.
  - `deviceConnect()` deletes and re-news the `QTcpSocket` on every connect, so it re-runs
    `createSocketConnections()` and resets `_selChannel = NoChannel` — the selected-channel cache
    must not survive a reconnect. On a failed connect it aborts the socket, because the 4 s wait
    is shorter than the OS connect timeout and a late success would arm the app behind an error
    dialog. `socketConnected()` treats `*IDN?` as the handshake and emits `deviceConnectionFailed()`
    instead of `deviceConnected()` if the instrument does not answer.
  - Link loss is declared in exactly one place, `reportLinkLost()`: it stops the poll, drops
    `_selChannel`, and emits `deviceConnectionError()` **once per session** (suppressed while
    connecting and after `abortPendingIo()`). Afterwards `sendCmdLine()` fails fast until the
    next `deviceConnect()`. It is reached two ways: a real socket error, or a command running out
    its **full** deadline — the latter is how a powered-off supply is detected, since that
    produces no socket error until the OS gives up minutes later.
  - `socketError()` deliberately ignores `SocketTimeoutError`. Every timed-out `waitFor*` emits
    it, and the waits run in 100 ms slices, so treating it as fatal declares the link lost
    whenever a reply takes longer than one slice. Don't route timeouts through `socketError()`.
  - A single-shot `QTimer` (`_periodicUpdateTmr`) re-arms itself at the end of
    `onPeriodicTimer()` to poll live voltage/current every `PERIODIC_UPDATE_INTERVAL_MS` (500ms)
    while `_periodicUpdateEnable` is set — deliberately not a repeating timer, so a slow/blocked
    socket round-trip can't cause overlapping polls.
  - `HMCChannel` is a `Q_ENUM` (underlying type `int`) with `NoChannel = 0` and channels numbered
    1-3. `hmcChannels` is the canonical iteration order (`Channel1, Channel2, Channel3`). Array
    storage maps channel → index via `chToArrayIndex()` (`x - 1`), which asserts
    `isValidChannel()`; every public slot that takes a channel rejects invalid values at the top,
    because `NoChannel` would otherwise index `-1`. New channel-taking slots need the same guard.
  - Per-channel limits are read from the instrument at connect (`VOLT? MAX` / `CURR? MAX` in
    `updateChannelLimits()`) and published via `channelLimitsChanged`. `FALLBACK_MAX_VOLTAGE` /
    `FALLBACK_MAX_CURRENT` in `hmcappglobal.h` are used only if those queries fail — don't
    hardcode limits elsewhere.
  - Cleanup is intentionally asymmetric: `cleanup()` (invoked via `Qt::BlockingQueuedConnection`
    from `MainWindow::closeEvent`) moves the object back to the GUI thread so the destructor
    (which calls `_thread.quit()`/`_thread.wait()`) runs safely from the thread that owns it.

- **`MainWindow`** (`mainwindow.h/.cpp`) — top-level window. Owns the `HMCSupplyCtrl` instance
  by value and wires GUI actions to it through signals/slots. Manages connect/disconnect
  lifecycle, drives `setPeriodicUpdateEnable`, and enables/disables channel widgets and the
  master-out button based on connection state. Host address is persisted via `QSettings` under
  key `SKEY_HOSTADDR` (`hmcappglobal.h`), defaulting to `DEFAULT_HOST` (`192.168.12.100`);
  the settings location comes from the organization/application names set in `main.cpp`.
  - `registerMetaTypes()` registers `QHostAddress` and `HMCSupplyCtrl::HMCChannel`. **Any new
    custom type carried by a cross-thread signal must be registered there**, or the queued
    connection silently fails at runtime.

- **`HMCChannelWidget`** (`hmcchannelwidget.h/.cpp`) — one instance per physical channel,
  configured post-construction via `setupWidget(hmcCtrl, channel, name)` rather than through the
  constructor (so it can be placed in the `.ui` file as a plain widget and wired up afterward). It
  connects directly to the shared `HMCSupplyCtrl` instance's per-channel signals, filtering on its
  own `_channel` to ignore updates for other channels.
  - Voltage/current entry goes through `ValueSetDialog`, invoked from
    `btnSetVoltageClicked`/`btnSetCurrentClicked`, with per-widget static preset lists
    (`_voltagePresets`, `_currentPresets`). The dialog gets `setRange(0, _maxVoltage/_maxCurrent)`
    from the device-reported limits, and the result is clamped to the same range before emitting.
  - The dialog is seeded from the widget's own `_targetVoltage`/`_targetCurrent`, cached on the
    GUI thread from `channelTargetVoltageChanged`/`channelTargetCurrentChanged`. `HMCSupplyCtrl`
    has **no public getters**, and apart from `abortPendingIo()` GUI code never calls into it
    directly. Keep it that way: mirror state into the widget via a signal rather than adding a
    getter.
  - Displayed power is computed client-side as the product of the last measured `_voltage` and
    `_current`; it is not queried from the instrument. Current auto-switches between A and mA
    display (with the `lbCurrUnit` label) below 1.0 A.

- **`ValueSetDialog`** / **`HostAddrConfigDialog`** — small reusable modal dialogs (numeric
  value entry with unit label and presets; host address entry) with no dependency on
  `HMCSupplyCtrl`, driven purely through their public getter/setter API and `QDialog::exec()`.
  Picking a `ValueSetDialog` preset closes the dialog immediately (it calls `QDialog::accept()`
  directly, bypassing validation). Typed input is validated in the overridden `accept()` — it
  refuses unparseable or out-of-range text rather than returning a stale value — and a C-locale
  `QDoubleValidator` from `setRange()` blocks bad input as it's typed. Call `setUnitString()`
  before `setPresets()`/`setRange()` — both embed the unit string.

### Threading model recap

GUI thread (`MainWindow`, `HMCChannelWidget`, dialogs) <-> queued signals/slots <-> worker thread
(`HMCSupplyCtrl`, blocking SCPI I/O). When adding new device functionality, add a slot to
`HMCSupplyCtrl` plus a matching signal on the caller side (mirroring the existing
`setChannelVoltage`/`channelTargetVoltageChanged` pattern) rather than exposing new synchronous
getters that would be called across threads.

### UI files

`.ui` files (`mainwindow.ui`, `hmcchannelwidget.ui`, `valuesetdialog.ui`,
`hostaddrconfigdialog.ui`) are edited with Qt Designer/Creator; `CMAKE_AUTOUIC` generates
`ui_*.h` headers at build time (and they are gitignored) — don't hand-edit generated `ui_*.h`
files.

### Dark theme

`main.cpp` sets a hardcoded dark `QPalette` on the Fusion style application-wide plus a tooltip
stylesheet; there is no light-theme path, so new widgets should rely on palette roles rather than
hardcoded colors. The exception is the three `QLCDNumber` readouts, which
`HMCChannelWidget::setupWidget` styles with explicit per-readout colors (green voltage, red
current, orange power) — that colour coding is deliberate, so keep it if you touch those.
