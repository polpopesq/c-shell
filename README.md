# c-shell

A minimal Unix-like shell written in C.  
It supports basic command execution, pipelines, redirections, and built-in commands.

This project is primarily an educational implementation focused on understanding
how shells parse, execute, and manage processes.

---

## Build & Run

The project uses **CMake**.

### Build
```bash
cmake -S . -B build
cmake --build build
```

### Run
```bash
./build/shell
```

## Features
### Command execution
- Executes external programs found in PATH
- Uses execvp() for program lookup
- Supports built-in commands (e.g. cd, exit, pwd, echo, history, type)

### Pipelines
- Supports pipelines using |
Example:
```sh
cat file.txt | grep foo | wc -l
```
- Proper pipe setup and process chaining
- Exit status follows the last command in the pipeline

### Redirections 
#### Input and output redirection:
- < (stdin)
- > / 1> (stdout truncate)
- >> / 1>> (stdout append)
- 2> / 2>> (stderr)
Redirections override pipe file descriptors when present

### Line editing & history
- Uses GNU Readline
- Line editing, history, and basic autocompletion support

## How it works (high-level)
### Initialization
- Scans PATH for executables
- Initializes GNU Readline
- Sets up command history handling

### Input loop
- Reads a line from the user
- Passes it to the parser
- Executes the resulting pipeline
- Cleans up allocated resources

### Parsing
- Tokenizes input while respecting quotes
- Splits commands on |
- Associates redirections with commands
- Builds an internal Pipeline structure

### Execution
- Executes a single command directly

For pipelines:
- Creates pipes
- Forks processes
- Sets up stdin/stdout via dup2
- Applies redirections
- Waits for all children

## Notes
- This is not a full POSIX shell
- No job control (fg, bg, signals, etc.)
- Intended for learning, not production use

## Dependencies
- CMake ≥ 3.13
- A C compiler with C23 support
- GNU Readline (libreadline-dev on Debian/Ubuntu)