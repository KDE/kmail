# KMail

State-of-the-art, feature-rich email client with support for many protocols.

KMail is a Qt/KDE application for managing email, calendars, and messaging workflows in a modern desktop environment. This repository contains the upstream-style KMail sources used for building the application from source.

## Overview

KMail is designed for productivity and flexibility, with support for:

- IMAP, POP3, and local mail storage
- Multiple account handling
- Rich message composition and formatting
- Folder and filtering workflows
- KDE integration and desktop environment features
- Email processing extensions and agents

## Project structure

- `src/` – main application source code
- `agents/` – mail processing agents such as send later, archive, follow-up reminders, and filters
- `ktnef/` – TNEF support utilities
- `kmail-refresh-settings/` – settings refresh utility
- `doc/` – documentation
- `po/` – translations
- `CMakeLists.txt` – top-level build configuration
- `CMakePresets.json` – common build presets

## Requirements

Building KMail requires a modern KDE and Qt development environment, including:

- CMake 3.29+
- Ninja build system
- Qt 6.9+
- KDE Frameworks 6.29+
- KDE PIM dependencies such as Akonadi, KPim libraries, and related components

This repository follows the typical KDE/Qt build flow and uses CMake presets for common configurations.

## Build

Clone the repository and configure the project:

```bash
git clone https://github.com/xabbas/kmail.git
cd kmail
cmake --preset dev
```

Build the project:

```bash
cmake --build --preset dev
```

Useful presets are already defined in `CMakePresets.json`, including:

- `dev` – debug build
- `dev-mold` – debug build with mold linker enabled when available
- `release` – release build
- `asan` – sanitizers enabled
- `coverage` – coverage build
- `unity` – unity build mode
- `ftime-trace` – build tracing for performance analysis

## Run

After building, run the application from the build directory or install it to your KDE environment as appropriate for your system.

## Development notes

This project is a C++ application with KDE/Qt integrations and is organized around desktop email workflows. Contributions typically involve UI work, mail processing logic, agents, translation updates, and build-system maintenance.

## License

This project uses KDE/REUSE licensing conventions. See the `LICENSES/` directory and `REUSE.toml` for details.

## Related

- Project homepage: https://invent.kde.org/pim/kmail
- Repository: https://github.com/xabbas/kmail
