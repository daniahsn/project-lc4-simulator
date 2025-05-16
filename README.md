# LC4 Simulator – CIS 2400 Final Project

A command-line LC4 processor simulator written in C, replicating the functionality of PennSim. It loads `.OBJ` binary files into simulated memory, executes instructions cycle-by-cycle, and outputs trace logs in PennSim-compatible format.

## 📁 Files

- `trace.c` – Main driver: handles file input, simulation loop, and output.
- `loader.c` – Parses LC4 `.OBJ` binary files and loads memory.
- `LC4.c` – Core simulator logic: instruction decoding, state updates.
- `LC4.h` / `loader.h` – Provided headers 
- `Makefile` – Compiles to a `trace` executable.

## 🧪 Build Instructions

```bash
make all
```

## ▶️ Usage

```bash
./trace output.txt file1.obj [file2.obj ...]
```

- `output.txt`: Trace log (one line per instruction).
- `fileX.obj`: Compiled LC4 binary files.

## 📝 Trace Format

Each line in the trace contains:

```
PC INSN_BINARY REGWE DESTREG REGVAL NZPWE NZPVAL DATAWE MEMADDR MEMVAL
```

Example:

```
8200 1001111000000000 1 7 0002 1 1 0 0000 0000
```

## ⚠️ Error Handling

Execution halts if:
- A data section is executed as code.
- Code is accessed as data.
- User mode accesses OS memory.

## 🔍 Testing

- Generate `.obj` files via PennSim `as` command.
- Compare output to PennSim using:
  ```bash
  diff trace_output.txt pennsim_trace.txt
  ```

## 🧑‍💻 Acknowledgements

Developed for **CIS 2400: Computer Systems**, University of Pennsylvania.
"""
