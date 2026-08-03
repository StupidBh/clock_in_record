# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Commands

```bash
# Configure (Ninja single-config generator)
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -B build -S .

# Build
cmake --build build

# Run the built app
./bin/RelWithDebInfo/AttendanceApp.exe

# Format C++ sources/headers using the checked-in .clang-format
clang-format -i Core/*.cpp Core/*.h Core/Cal/*.cpp Core/Cal/*.h Core/Utils/*.cpp Core/Utils/*.h Core/Types/*.h
```

There are currently no tests and no linter target. If new `.cpp`/`.h` files are added, re-run the CMake configure command because `Core/CMakeLists.txt` uses `file(GLOB_RECURSE ...)`.

The root `CMakeLists.txt` currently sets `CMAKE_PREFIX_PATH` to `D:/Qt6/6.11.1/mingw_64`, but the project is written to find either Qt 6 or Qt 5 (`Core`, `Widgets`, `Network`). Build outputs go to `bin/<CONFIG>/` at the repository root. On Windows, `Core/CMakeLists.txt` adds the app icon resource and runs `windeployqt` after build when available.

## Architecture

This is a Windows desktop attendance tracker built with Qt Widgets. Users select calendar dates, enter arrival/departure times, choose whether the day counts toward average overtime, and view monthly overtime statistics.

**Application entry / single instance** (`Core/main.cpp`): Creates the `QApplication`, sets app metadata (`MyCompany` / `AttendanceApp`) used by `QSettings`, applies the global font/icon, and enforces a single running instance with `QLocalServer`/`QLocalSocket` using server name `AttendanceApp-SingleInstance`. A second launch sends `activate`; the existing `AttendanceMainWindow` calls `raiseAndActivate()`.

**Data storage** (`QSettings`): Data is stored in the platform settings backend under organization/app name `MyCompany/AttendanceApp` (Windows registry on Windows). There are two logical tiers:

- Per-date record keys under `YYYY-MM-DD/`: `arrival`, `departure`, `needAverageCal`. These are written by `TimeSettingDialog::saveRecord()`.
- Global time defaults under `settings/`: `workStart`, `workEnd`, `lunchStart`, `lunchEnd`, `dinnerStart`, `dinnerEnd`. These are edited in the main window's right panel and read by `loadGlobalTimeDefaults()` in `WorkTimeCalculator.h`.

Data is loaded lazily from `QSettings`; there is no central in-memory model. The main exception is `CustomCalendarWidget::m_data`, a view cache for painted arrival/departure text populated while monthly statistics are recalculated. `AttendanceMainWindow::deleteAttendanceRecord()` also removes legacy per-date work/break keys for backward compatibility.

**Domain types and calculation** (`Core/Types/AttendanceTypes.h`, `Core/Cal/WorkTimeCalculator.*`): `AttendanceRecord` and `WorkTimeResult` are plain structs. `AttendanceRecord` defaults to 09:00–18:00 work time, 12:30–13:30 lunch, and 18:00–18:30 dinner. `WorkTimeCalculator::calculateWorkTimeResult()` is a stateless utility: it validates arrival/departure order, computes late/early-leave minutes, clips lunch/dinner breaks to the actual and standard work ranges, and returns overtime as `actualWorkMinutes - standardWorkMinutes` (can be negative).

**Main window** (`Core/AttendanceMainWindow.*`): Owns the main UI and orchestration. The left side is `CustomCalendarWidget` plus usage text; the right side is monthly stats plus a collapsed-by-default `CollapsibleGroupBox` for global work/break settings inside a `QSplitter`. Date clicks open `TimeSettingDialog`. Month changes, record saves/deletes, and global settings changes refresh calendar painting and monthly statistics. Monthly total overtime sums only positive daily overtime; `needAverageCal` controls the denominator for average overtime. The target average overtime is hardcoded at 2.5 hours/day.

**Calendar view** (`Core/Utils/CustomCalendarWidget.*`): Subclasses `QCalendarWidget`. It paints selected/today states and overlays arrival/departure times from `m_data`. Right-click uses the internal `QTableView` to map the click position to a date, then shows a delete menu only for dates with an `arrival` setting.

**Time-setting dialog** (`Core/Utils/TimeSettingDialog.*`): Modal dialog for one date. It edits only arrival, departure, and `计入平均加班日`; standard work/break times come from global settings captured when the dialog is constructed. Arrival/departure changes trigger live recalculation through `QTimeEdit::timeChanged`. New records default `needAverageCal` to false on weekends and true on weekdays.

**Collapsible settings widget** (`Core/Utils/CollapsibleGroupBox.*`): Small reusable widget consisting of a checkable button and content widget; used for the global settings panel.

## Project-specific notes

- Keep UI strings consistent with the existing Chinese-language interface.
- `.clang-format` is WebKit-based with 4-space indentation, 120-column limit, LF endings, unsorted includes, and left-aligned pointers/references.
- `resources/resources.qrc` contains the Qt resource collection; the app icon is `resources/Icons/logo.ico` and is referenced as `:/Icons/logo.ico`.
