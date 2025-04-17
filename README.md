# MASM Icc

The Ultimate Runtime for Micro-Assembly

## Overview

MicroASM is a custom assembly language designed for simplicity and extensibility. This toolchain provides:

1.  **`microasm_compiler`**: Compiles MicroASM source code (`.masm`) into a custom binary format (`.bin`).
2.  **`microasm_interpreter`**: Executes the compiled MicroASM binary files.

While the toolchain can be used standalone, its primary purpose is to serve as a core library for building custom MicroASM runtimes in other environments (e.g., embedding MicroASM scripting into a larger application), as described in [usage.md](usage.md).

## MicroASM Language

MicroASM features a set of basic instructions for arithmetic, logic, flow control, stack manipulation, and I/O. It uses a simple syntax with registers (RAX, RBX, R0-R15, etc.) and supports labels for jumps and calls. Data can be defined using the `DB` directive.

Key features include:
*   Case-insensitive instructions (`MOV` is the same as `mov`).
*   Case-sensitive arguments (`R1` is different from `r1`).
*   No commas between arguments (`MOV R1 R2`).
*   Labels for jumps/calls are prefixed with `#` (e.g., `JMP #loop`).
*   Data labels/addresses are often prefixed with `$` (e.g., `DB $1000 "string"`, `MOV R1 $1000`).

For a detailed description of the instruction set, see [docs/v2instructions.md](docs/v2instructions.md).

## MNI (Micro-Assembly Native Interface)

A key feature of this implementation is the **MNI (Micro-Assembly Native Interface)**. MNI allows MicroASM code to call native functions implemented in the host language (in this case, C++). This enables extending the capabilities of MicroASM significantly by providing access to system resources, complex libraries, and host application features.

MNI functions are called using the `MNI` instruction followed by a module and function name (e.g., `Module.Function`), along with arguments (which can be registers, immediate values, or data addresses).

Example:
```nasm
; Allocate memory using a native function
MNI Memory.allocate 100 R1 ; Allocate 100 bytes, store address in R1

; Call a native string operation
DB $1000 "Hello"
MOV R1 1000
MNI StringOperations.length R1 R2 ; Get length of string at address in R1, store in R2

; Write to console
MNI IO.write 1 R2 ; Write the value in R2 (the length) to stdout
```

In this C++ implementation, MNI functions are made available to the interpreter by calling `registerMNI("ModuleName", "FunctionName", cpp_lambda_or_function_pointer)` within the C++ code during initialization. This allows seamless integration between the assembly code and the native C++ environment.

See [docs/mni-instructions.md](docs/mni-instructions.md) and the MNI sections in [docs/v2instructions.md](docs/v2instructions.md) for more details and available functions.

## Building

The project uses CMake to generate build files.

1.  Ensure you have CMake and a compatible C++ compiler (like MSVC, GCC, or Clang supporting C++17) installed.
2.  Create a build directory (e.g., `build`).
3.  Navigate into the build directory and run CMake: `cmake ..`
4.  Build the project using your chosen build system (e.g., `make` or open the generated solution file in Visual Studio).

This will typically produce `microasm_compiler` and `microasm_interpreter` executables (or potentially a combined `masm` executable depending on the CMake configuration).

## Usage

**Compiler:**
```bash
./microasm_compiler <input_file.masm> <output_file.bin>
```

**Interpreter:**
```bash
./microasm_interpreter <input_file.bin> [arg1] [arg2] ...
```
Command-line arguments passed after the `.bin` file can be accessed within the MicroASM code using the `ARGC` and `GETARG` instructions.

## Documentation

Further details about the language, MNI, and the compilation/interpretation workflow can be found in the `docs` directory:

*   [docs/v2instructions.md](docs/v2instructions.md): Core instruction set reference.
*   [docs/mni-instructions.md](docs/mni-instructions.md): MNI usage and examples.
*   [docs/workflow.md](docs/workflow.md): Compiler and interpreter workflow details.
*   [docs/extended-mni.md](docs/extended-mni.md): Information on extended MNI functions (if implemented).
*   [usage.md](usage.md): High-level description of the toolchain's purpose.

