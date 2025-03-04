# JBash

JBash (Jeremiah Brenio awesome SHell) is a custom Unix shell implementation in C.

## Overview

JBash is a lightweight command-line interpreter that provides basic shell functionality. It's designed as an educational project to demonstrate fundamental concepts of operating systems, process management, and terminal handling.

## Features

- Command execution through fork/exec system calls
- Built-in commands:
  - `cd` - Change directory
  - `exit` - Exit the shell
- Interactive terminal interface:
  - Character-by-character input processing
  - Cursor movement with left/right arrow keys
  - Basic line editing capabilities
  - Signal handling (Ctrl+C)
- Dynamic memory allocation for command parsing

## Implementation Details
- JBash implements:
  - Raw terminal mode for interactive input
  - Command parsing with support for quoted strings
  - Process creation and management
  - Memory-safe operations with dynamic buffer sizing

## Building

To compile JBash, simply run:

```bash
make
```
This will produce the executable JBash which you can run with:
```bash
./JBash
```
