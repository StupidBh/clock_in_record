# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -B build -S .
cmake --build build
```

Qt 5.15.2 path is hardcoded in root `CMakeLists.txt` as `C:/Qt/5.15.2/msvc2019_64`. MSVC 2019 64-bit, C++20, with `/utf-8` flag.

There are no tests. There is no linter/formatter configured beyond `.clang-format` (WebKit-based, customized).

## Architecture

A Windows desktop attendance tracker built with Qt Widgets. The user clicks a calendar date, fills in arrival/departure times and break periods, and sees monthly overtime statistics.

**Data layer**: All attendance records persist to `QSettings` (Windows registry: `HKEY_CURRENT_USER\Software\MyCompany\AttendanceApp`). Each date stores 9 string/bool keys (`YYYY-MM-DD/arrival`, `YYYY-MM-DD/departure`, etc.). Loading is lazy — data is read from QSettings on demand, never cached in memory.

**Calculation engine** (`src/Cal/WorkTimeCalculator`): Pure stateless utility. A single static method takes an `AttendanceRecord` (all times for a day) and returns a `WorkTimeResult` (late minutes, early-leave minutes, actual work minutes, standard work minutes, overtime minutes, total break minutes). Overtime can be negative (deficit). Break time only counts the portion of lunch/dinner breaks that overlap with the actual work period.

**Main window** (`src/AttendanceMainWindow`): Left panel = custom calendar + instructions; right panel = monthly stats label, inside a `QSplitter` (2:1 ratio, right max 350px). Clicking a date opens `TimeSettingDialog`. After save or month change, it recalculates all days in the visible month for the stats panel. Stats tracked but not displayed: total late minutes, total early-leave minutes.

**Custom calendar** (`src/Utils/CustomCalendarWidget`): Subclasses `QCalendarWidget`. Paints cells with green background for dates that have records (lighter shade when `needAverageCal` is false). Selected date gets a blue rounded rect; today gets a blue outline. Arrival/departure times are drawn in small blue text on the cell. Right-click on a date with data shows a delete context menu.

**Auto-updater** (`src/UpdateChecker/GitHubUpdater`): Fetches `https://api.github.com/repos/wozouri/clock_in_record/releases/latest`, compares `QVersionNumber`, downloads the `.exe` asset to the user's Downloads folder. The call to `updateCheck()` is commented out in the constructor — the feature is shipped but disabled.

## Key patterns

- `AttendanceTypes.h` defines the two plain structs (`AttendanceRecord`, `WorkTimeResult`) used everywhere. No methods, just data.
- `TimeSettingDialog` does live calculation on every `QTimeEdit::timeChanged` signal — there is no "Calculate" button.
- The `needAverageCal` flag exists so non-standard days (holidays, make-up workdays) can be excluded from the monthly average overtime calculation but still recorded.
- The `CollapsibleGroupBox` widget is a generic reusable component (toggle button + animated content area), used in `TimeSettingDialog` to fold the detailed settings section.
