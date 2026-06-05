# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -B build -S .
cmake --build build
```

Qt 5.15.2 path is hardcoded in root `CMakeLists.txt` as `C:/Qt/5.15.2/msvc2019_64`. MSVC 2019 64-bit, C++20, with `/utf-8` flag.

There are no tests. There is no linter/formatter configured beyond `.clang-format` (WebKit-based, customized).

`src/CMakeLists.txt` uses `file(GLOB_RECURSE ...)` to collect sources — new `.cpp`/`.h` files are auto-discovered, but you must re-run cmake after adding files.

## Architecture

A Windows desktop attendance tracker built with Qt Widgets. The user clicks a calendar date, fills in arrival/departure times, and sees monthly overtime statistics.

**Single-instance enforcement** (`src/main.cpp`): Uses `QLocalServer`/`QLocalSocket` (server name `AttendanceApp-SingleInstance`). A second launch sends `"activate"` to the existing instance and exits. The existing instance calls `raiseAndActivate()` to bring its window to the foreground.

**Data layer** (`QSettings` → Windows registry: `HKEY_CURRENT_USER\Software\MyCompany\AttendanceApp`). Two tiers of keys:

- **Per-date** (3 keys): `YYYY-MM-DD/arrival`, `YYYY-MM-DD/departure`, `YYYY-MM-DD/needAverageCal`. These are the only keys written by `TimeSettingDialog::saveRecord()`. The `deleteAttendanceRecord()` method deletes 9 keys as a backward-compatibility cleanup (standard work/break times were historically stored per-date).
- **Global defaults** (6 keys under `settings/` prefix): `settings/workStart`, `settings/workEnd`, `settings/lunchStart`, `settings/lunchEnd`, `settings/dinnerStart`, `settings/dinnerEnd`. These are managed by the right-panel `CollapsibleGroupBox` in the main window and used as defaults for all dates.

Loading is lazy — data is read from QSettings on demand, never cached in memory (except `CustomCalendarWidget::m_data`, which is a view-layer display cache populated by `updateMonthlyStatistics()`).

**Calculation engine** (`src/Cal/WorkTimeCalculator`): Pure stateless utility. A single static method takes an `AttendanceRecord` (all times for a day) and returns a `WorkTimeResult` (late minutes, early-leave minutes, actual work minutes, standard work minutes, overtime minutes, total break minutes). Overtime can be negative (deficit). Break time only counts the portion of lunch/dinner breaks that overlap with the actual work period.

**Main window** (`src/AttendanceMainWindow`): Left panel = custom calendar + instructions; right panel = monthly stats label + collapsible global settings, inside a `QSplitter` (2:1 ratio, right max 350px). Clicking a date opens `TimeSettingDialog`. After save, month change, or global settings change, it recalculates all days in the visible month for the stats panel. Stats tracked but not displayed: total late minutes, total early-leave minutes. Clicking outside the calendar resets the selection (by selecting a date one year in the future and paging back to the current month).

**Custom calendar** (`src/Utils/CustomCalendarWidget`): Subclasses `QCalendarWidget`. Paints cells with green background for dates that have records (lighter shade when `needAverageCal` is false). Selected date gets a blue rounded rect; today gets a blue outline with white fill. Arrival/departure times are drawn in small blue text on the cell. Right-click on a date with data shows a delete context menu. Uses `QTableView` (found via `findChild`) for position-to-date mapping.

**Time setting dialog** (`src/Utils/TimeSettingDialog`): Modal dialog for a single date. Shows only arrival time, departure time, and a "计入平均加班日" (include in average overtime) checkbox. Standard work hours and break times are read from global QSettings — they are not editable per-date in this dialog. Live calculation runs on every `QTimeEdit::timeChanged` signal (no "Calculate" button). The result label has a fixed height (130px) to prevent dialog resizing.

**Collapsible group box** (`src/Utils/CollapsibleGroupBox`): Generic reusable widget (toggle button + animated content area), used in the main window's right panel to fold the global settings section (collapsed by default).

## Key patterns

- `AttendanceTypes.h` defines the two plain structs (`AttendanceRecord`, `WorkTimeResult`) used everywhere. No methods, just data. Default constructor fills in typical 9-to-6 times with a 12:30–13:30 lunch break.
- `TimeSettingDialog` does live calculation on every `QTimeEdit::timeChanged` signal — there is no "Calculate" button.
- The `needAverageCal` flag excludes non-standard days (holidays, make-up workdays) from the monthly average overtime calculation. For new records, it defaults to `false` on weekends (Saturday=6, Sunday=7) and `true` on weekdays.
- The statistics panel uses a hardcoded target of **2.5 hours/day** average overtime. If the monthly average is below this, it shows "缺加班时间" (deficit); if above, "余加班时间" (surplus).
- Only positive overtime minutes are summed for the monthly total (deficits on individual days are ignored in the sum).
- Global settings changes trigger immediate save to QSettings and recalculation of monthly statistics.
