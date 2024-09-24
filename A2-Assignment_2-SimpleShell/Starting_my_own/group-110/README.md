# Simple-Shell: A Unix Shell in C from Scratch

## Description
SimpleShell is a basic Unix-like shell implemented in C that allows users to execute various commands, including built-in commands like `cd` and `exit`. It supports command execution with arguments, pipes, background processes, and command history. The shell continues running until the user terminates it using `Ctrl+C`.

## Features
- **Command Execution:** Execute Unix commands along with arguments.
- **Command History:** Maintains a history of executed commands and their details.
- **Pipes:** Supports piping between commands using the `|` operator.
- **Background Processes:** Allows execution of commands in the background using `&`.
- **Built-in Commands as well as a custom command:** `show_history`: Display the command history.
- **Signal Handling:** Handles `SIGINT` (Ctrl+C) gracefully by displaying the command history and process details.

## Usage
1. **Start the Shell:**
   Run the `SimpleShell` by compiling the code and executing the binary. 
   You can directly use the Makefile if you do not want to compile it manually.

2. **Basic Commands:**
- To run standard Unix commands like `ls`, `cat`, etc.:
  ```
  Vemy@simple-shell: $ ls
  ```
- To execute multiple commands using pipes:
  ```
  Vemy@simple-shell: $ cat file.txt | grep 'text' | wc -l
  ```
- To run commands in the background:
  ```
  Vemy@simple-shell: $ ./long_running_task &
  ```

3. **Built-in Commands:**
- `cd <directory>`: Change the shell's working directory.
- `exit`: Exit the shell.
- `show_history`: Display the history of executed commands.

## Compilation and Execution
1. Make sure you have the required dependencies:
- C compiler (`gcc`)
- GNU Make
- `readline` library

2. Run the shell: ./shell

## Limitations
- Commands like `jobs`, `fg`, and `bg` are not supported due to the lack of job control implementation.
- Commands that manipulate environment variables (`export`, `set`, `unset`) are not supported as they require modifying the parent shell's environment.
- Redirection (`>`, `<`, `>>`) is currently not implemented.

## Authors
- Group ID: Group-110
- Member: Yash Verma (2023610)


