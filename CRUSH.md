# Shortwave Radio Development Guide

## Project Overview
- **Language**: C-style C++ for Win32 API compatibility
- **Target Platform**: Windows XP
- **Primary Goal**: Vintage Shortwave Radio Tuner Application

## Build & Development Commands
- **Build**: `nix build`
  - Compiles Shortwave Radio application
- **Build Installer**: `nix build .#installer` or `build-installer`
  - Creates Windows installer (ShortwaveRadioInstaller.exe)
- **Dev Setup**: `setup-dev`
  - Generates `compile_commands.json`
- **Deploy**: `deploy-to-xp`
  - Copies executable and DLLs to XP VM
- **Debugging**: Use Visual Studio or WinDbg
- **Testing**: Manual testing on Windows XP

## Important: Nix Build System
- **CRITICAL**: Nix only includes files tracked in git
- **Always run**: `git add .` after adding new files/libraries
- **BASS Integration**: Files in `libs/` directory must be committed to git
- **Build fails?** Check if new files are added to git with `git status`

## Code Style Guidelines

### Formatting
- Use tabs for indentation
- Opening braces on same line
- Max line length: 80 characters
- Avoid trailing whitespace

### Naming Conventions
- Functions: `PascalCase` (e.g., `StartBassStreaming`)
- Constants/Macros: `UPPER_SNAKE_CASE` (e.g., `ID_ABOUT`)
- Global Variables: `g_` prefix (e.g., `g_radio`)
- Avoid abbreviations

### Types & Memory
- Prefer standard C types: `int`, `char*`
- Use Win32 types: `HWND`, `LPARAM`
- Avoid STL
- Static allocation preferred
- No dynamic memory allocation
- No exceptions

### Error Handling
- Always check Win32 API return values
- Validate pointers before use
- Use `NULL` checks
- Log errors to console/file
- Graceful failure modes

### Imports & Headers
1. `<windows.h>`
2. Standard C headers
3. Project-specific headers
- Use include guards
- Minimize header dependencies

### Documentation
- Comment complex Win32 logic
- Document function parameters/returns

## Best Practices
- Prioritize Win32 API compatibility
- Minimize external dependencies
- Focus on performance and low resource usage
- Test thoroughly on target Windows XP environment

## Radio Features
- Vintage shortwave radio interface
- Internet radio streaming capability
- Realistic tuning and signal simulation
- Keyboard and mouse controls
- Debug console for troubleshooting