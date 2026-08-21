# Repository Guidelines

## Project Structure & Module Organization

This is a Qt 6 desktop attendance tracker built with C++23 and CMake. Source files are grouped by feature under `Core/`:
`Application/` owns startup and the main window, `Attendance/` contains data types and calculations, `Calendar/`
provides the calendar UI, `Settings/` contains configuration dialogs, and `Widgets/` holds reusable controls. In each
feature directory, public `*.h` files sit at the top level and implementations live in `src/`. Images and Qt resource
declarations are under `resources/`; add new assets to `resources/resources.qrc`. Generated output belongs in `build/`
and `bin/<Debug|Release>/` and must remain untracked.

## Language and Dependency Baseline

The supported project baseline is C++23, Qt 6.11.1, CMake 4.3 or newer, and MSVC 18 (Visual Studio 2026) targeting
x64. Treat these versions as repository requirements, not machine-specific suggestions. All C++ targets must continue
to require `cxx_std_23` with compiler extensions disabled. The required Qt components are Core, Widgets, and Network;
use an MSVC-compatible Qt build when compiling with MSVC. Workstations may use different Qt installation paths, but
must provide the path locally through `Qt6_DIR`. Any baseline version change must update this section, the root
`CMakeLists.txt`, and the build commands together.

## Build, Test, and Development Commands

The Windows build expects CMake 4.3+, Visual Studio 2026, and Qt 6.11.1. Select the Visual Studio/MSVC toolchain in
the IDE and configure `Qt6_DIR` locally to point to that workstation's `lib/cmake/Qt6` directory. Do not commit
machine-specific Qt paths.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 `
  -DQt6_DIR=D:/path/to/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6
cmake --build build --config Debug
.\bin\Debug\AttendanceApp.exe
ctest --test-dir build -C Debug --output-on-failure
cmake --build build --config Release
```

The first command configures a local multi-configuration build tree, the second builds Debug, and the third launches the
application. In CLion, place the same `-DQt6_DIR=...` value in the CMake profile's options instead. CTest runs the
regression executables. Debug and Release application builds invoke `windeployqt` when available, placing the required
Qt runtime files next to `AttendanceApp.exe`; this project intentionally has no CMake install step.

## Coding Style & Naming Conventions

Use the repository `.clang-format`: WebKit-derived style, four spaces, no tabs, a 120-column limit, and left-aligned
pointers. Format touched files with `clang-format -i Core/Application/src/main.cpp`. Follow existing Qt/C++ conventions:
PascalCase classes (`WorkTimeCalculator`), camelCase functions and locals (`calculateWorkTimeResult`), `m_` prefixes for
members, and matching `.h`/`.cpp` filenames. Include project headers from the `Core/` root, for example
`#include "Attendance/WorkTimeCalculator.h"`. Keep `Q_OBJECT` classes in headers so CMake AUTOMOC can process them.

## Testing Guidelines

CTest runs the executable-based regression tests under `tests/`; no external test framework or coverage gate is
configured. Every change must at least build cleanly in Debug, pass CTest, and receive a focused manual check. For UI or
persistence changes, verify launch, single-instance activation, date editing/deletion, global time settings, and restart
persistence. Calculation changes should exercise invalid ranges, break overlap, late arrival, early departure, and
overtime; add a CTest-backed unit target for new automated coverage.

## Commit & Pull Request Guidelines

Recent commits use bracketed types such as `[feat]`, `[fix]`, `[refactor]`, `[style]`, and `[update]`; follow that
pattern with a concise imperative summary. Use `[test]`, `[docs]`, or `[build]` when those types describe the change
more precisely.

### Commit Granularity

- Give each commit one reviewable intent that can be described by one imperative sentence. If the summary needs "and"
  to join independently useful changes, split the commit. File count and line count are not reasons by themselves to
  split an otherwise atomic change.
- Keep every commit buildable. It must pass the focused tests for its scope and must not rely on a later commit to add a
  missing source file, CMake registration, Qt resource entry, migration, or required test fixture.
- Commit implementation and its direct regression tests together. In particular, attendance calculation changes belong
  with their `tests/` cases, new Qt classes belong with their `Core/CMakeLists.txt` registration, and new assets belong
  with the corresponding `resources/resources.qrc` entry.
- Keep persistence changes atomic: a `QSettings` key or record-format change, backward-compatible read or migration
  logic, and restart-persistence coverage belong in the same commit.
- Separate behavior-preserving refactors from behavior changes when the refactor is independently reviewable. Land the
  refactor first, with tests still passing, then add the behavior. Do not manufacture a separate refactor commit for
  trivial edits that only support one fix.
- Isolate mechanical churn from semantic changes. Repository-wide formatting, line-ending normalization, bulk renames,
  and generated-file refreshes each require a separate `[style]` or `[build]` commit so reviewers can ignore mechanical
  diffs safely.
- Keep unrelated feature areas separate. Calendar UI, work-time calculations, settings persistence, and build or
  dependency changes should not share a commit unless they jointly implement one indivisible user-visible behavior.
- Update version baselines atomically: changes to the required C++, Qt, CMake, or MSVC versions must include the root
  `CMakeLists.txt`, this document, and affected build commands in one `[build]` or `[update]` commit.
- Before committing, stage explicit files or hunks, inspect `git diff --cached`, and confirm no generated output,
  machine-specific paths, user `QSettings` data, or unrelated working-tree changes are included. Do not leave `WIP` or
  `fixup!` commits in the final branch history.

Examples of appropriate separation are `[fix] unify light and dark theme colors` for the theme implementation, widget
integration, CMake registration, and theme tests; followed by `[style] enforce LF line endings` for `.gitattributes` and
the mechanical normalization only.

Pull requests should explain behavior and validation, link related issues, and include before/after screenshots for
visible UI changes. Keep generated build files, IDE metadata, and user-specific `QSettings` data out of commits.
