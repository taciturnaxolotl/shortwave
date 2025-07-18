# Shortwave Development Guide

## Project Overview
- **Language**: C-style C++ for Win32 API compatibility
- **Target Platform**: Windows XP
- **Primary Goal**: Simple Win32 Application

## Build & Development Commands
- **Build**: `nix build`
  - Compiles Win32 application
- **Dev Setup**: `setup-dev`
  - Generates `compile_commands.json`
- **Deploy**: `deploy-to-xp`
  - Copies executable and DLLs to XP VM
- **Debugging**: Use Visual Studio or WinDbg
- **Testing**: Manual testing on Windows XP

## Code Style Guidelines

### Formatting
- Use tabs for indentation
- Opening braces on same line
- Max line length: 80 characters
- Avoid trailing whitespace

### Naming Conventions
- Functions: `PascalCase` (e.g., `QRCode_Init`)
- Constants/Macros: `UPPER_SNAKE_CASE` (e.g., `ID_ABOUT`)
- Global Variables: `g_` prefix (e.g., `g_qrCode`)
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
- Log errors to file/console
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