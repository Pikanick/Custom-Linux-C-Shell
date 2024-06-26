# Custom-Linux-C-Shell

## Overview
This project is a simple shell implementation in C that supports basic command execution, including background process handling and command history.

## Files
- `shell.c`: The main C source file containing the implementation of the shell.
- `Makefile`: The Makefile to compile the shell program.

## Features
- **Command Execution**: Executes commands with arguments.
- **Background Execution**: Supports running commands in the background.
- **Command History**: Maintains a history of the last 10 executed commands.
- **Built-in Functions**: Includes some built-in functions for handling history and basic shell functionalities.

## Requirements
- GCC Compiler
- Unix-like Operating System

## Compilation
To compile the shell, run:
\`\`\`bash
make
\`\`\`
This will generate an executable named `shell`.

## Usage
To start the shell, run:
\`\`\`bash
./shell
\`\`\`

### Commands
- **Basic Commands**: You can run any command available in the system's PATH.
- **Background Execution**: Append `&` at the end of the command to run it in the background.
- **History**: The shell maintains a history of the last 10 commands.

### Built-in Commands
- **history**: Displays the list of recently executed commands.

## Clean Up
To clean up the compiled files, run:
\`\`\`bash
make clean
\`\`\`

## Implementation Details
The shell implementation includes:
- Tokenizing input commands.
- Forking processes to handle command execution.
- Using `execvp` to execute commands.
- Managing background processes.
- Maintaining a simple history of commands.

## License
This project is licensed under the MIT License.
