# Repository Guidelines

## Project Structure & Module Organization

This is a Qt 6 desktop attendance tracker built with C++20 and CMake. Source files are grouped by feature under `Core/`: `Application/` owns startup and the main window, `Attendance/` contains data types and calculations, `Calendar/` provides the calendar UI, `Settings/` contains configuration dialogs, and `Widgets/` holds reusable controls. In each feature directory, public `*.h` files sit at the top level and implementations live in `src/`. Images and Qt resource declarations are under `resources/`; add new assets to `resources/resources.qrc`. Generated output belongs in `build/` and `bin/<Debug|Release>/` and must remain untracked.

## Build, Test, and Development Commands

The current Windows configuration expects CMake 4.0+, Qt 6.11.1, and either Visual Studio 2022 or MinGW in the paths declared by the root `CMakeLists.txt`.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
.\bin\Debug\AttendanceApp.exe
cmake --build build --config Release
cmake --install build --config Release --prefix build/package
```

The first command configures the project, the second builds it, and the third launches the debug executable. The final two commands build and stage a release; on Windows, the build also invokes `windeployqt` when available.

## Coding Style & Naming Conventions

Use the repository `.clang-format`: WebKit-derived style, four spaces, no tabs, a 120-column limit, and left-aligned pointers. Format touched files with `clang-format -i Core/Application/src/main.cpp`. Follow existing Qt/C++ conventions: PascalCase classes (`WorkTimeCalculator`), camelCase functions and locals (`calculateWorkTimeResult`), `m_` prefixes for members, and matching `.h`/`.cpp` filenames. Include project headers from the `Core/` root, for example `#include "Attendance/WorkTimeCalculator.h"`. Keep `Q_OBJECT` classes in headers so CMake AUTOMOC can process them.

## Testing Guidelines

No automated test framework or coverage gate is configured. Every change must at least build cleanly in Debug and receive a focused manual check. For UI or persistence changes, verify launch, single-instance activation, date editing/deletion, global time settings, and restart persistence. Calculation changes should exercise invalid ranges, break overlap, late arrival, early departure, and overtime; add a CTest-backed unit target when introducing automated tests.

## Commit & Pull Request Guidelines

Recent commits use bracketed types such as `[feat]`, `[fix]`, `[refactor]`, `[style]`, and `[update]`; follow that pattern with a concise imperative summary. Pull requests should explain behavior and validation, link related issues, and include before/after screenshots for visible UI changes. Keep generated build files, IDE metadata, and user-specific `QSettings` data out of commits.
