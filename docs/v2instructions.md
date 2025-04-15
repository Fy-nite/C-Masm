# MicroASM Instruction Set Documentation

This document details all instructions available in the MicroASM language.

### notes

Instructions are case-insensitive (MOV = mov)
Arguments are case-sensitive (R1 ≠ r1)
No commas between arguments (use mov RAX RBX not mov RAX, RBX)
Currently lacks memory addressing with offsets (no mov RBX [RBX + 5] style syntax)

## Registers

- RAX (index 0)
- RBX (index 1)
- RCX (index 2)
- RDX (index 3)
- RSI (index 4)
- RDI (index 5)
- RBP (index 6)   ; Stack Base Pointer
- RSP (index 7)   ; Stack Pointer
- R0 (index 8) through R15 (index 23) (General Purpose Registers)
- **Note:** There is no dedicated RFLAGS register; flags are handled internally by the interpreter.

Registers are mapped as follows:
- RAX, RBX, ..., RSP: indices 0-7
- R0-R15: indices 8-23

RBP (index 6) and RSP (index 7) are used for stack frame management.

## Basic Instructions

### MOV (Move)

```
MOV dest, src
```

Copies a value from the source to the destination register.
Example: `MOV R1 R2` - Copies value from R2 to R1

### ADD (Addition)

```
ADD dest src
```

Adds the source value to the destination register and stores the result in the destination.
Example: `ADD R1 R2` - R1 = R1 + R2

### SUB (Subtraction)

```
SUB dest src
```

Subtracts the source value from the destination register and stores the result in the destination.
Example: `SUB R1 R2` - R1 = R1 - R2

### MUL (Multiplication)

```
MUL dest src
```

Multiplies the destination register by the source value and stores the result in the destination.
Example: `MUL R1 R2` - R1 = R1 * R2

### DIV (Division)

```
DIV dest src
```

Divides the destination register by the source value and stores the result in the destination.
Example: `DIV R1 R2` - R1 = R1 / R2

### INC (Increment)

```
INC dest
```

Increments the value in the destination register by 1.
Example: `INC R1` - R1 = R1 + 1

## Flow Control

### JMP (Jump)

```
JMP label
```

Unconditionally jumps to the specified label.
Example: `JMP #loop` - Jumps to label 'loop'

### CMP (Compare)

```
CMP dest, src
```

Compares two values and sets internal flags for conditional jumps.
Example: `CMP R1 R2` - Compares R1 with R2

### JE (Jump if Equal)

```
JE label_true label_false
```

Jumps to label_true if the previous comparison was equal, otherwise jumps to label_false.
Example: `JE #equal #not_equal`

### JL (Jump if Less)

```
JL label
```

Jumps to the specified label if the previous comparison result was "less than".
Example: `JL #less`

### CALL (Call Function)

```
CALL label
```

Calls a function at the specified label.
Example: `CALL #function`
or calls external code if not marked with a #
Example: `CALL $function`

Note: Unlike JMP, CALL saves the current instruction pointer (RIP) on the stack before jumping to the label. This allows the function to return to the point where it was called using the RET instruction.

## Stack Operations

### PUSH

```
PUSH src
```

Pushes a value onto the stack.
Example: `PUSH R1`

### POP

```
POP dest
```

Pops a value from the stack into the destination register.
Example: `POP R1`

## I/O Operations

### OUT

```
OUT port value
```

Outputs a value or string (prefixed with $) to 1. stdout, 2. stderr with a newline.
Example: `OUT 2 R1` or `OUT 1 $500`

### COUT

```
COUT port value
```

Outputs a single character value to stdout following the ASCII table and OUT rules.
Example: `COUT 1 R1` or `COUT 2 $65`

### OUTSTR (Output String)

```
OUTSTR port address length
```

Outputs a string from memory starting at the specified address with the given length to the specified port.
Example: `OUTSTR 1 R1 R2` - Outputs a string from address in R1 with length in R2 to stdout.

### OUTCHAR (Output Character)

```
OUTCHAR port address
```

Outputs a single character from memory at the specified address to the specified port.
Example: `OUTCHAR 1 R1` - Outputs a character from the address in R1 to stdout.

## Program Control

### HLT (Halt)

```
HLT
```
```

Gets the count of command line arguments into the destination register.
Example: `ARGC R1`

### GETARG

```
GETARG dest index
```

Gets the command line argument at the specified index into the destination register.
Example: `GETARG R1 R2`

in jmasm, the index is 0-based and comes after the launched program name, unlike how csharp wants -- --<argument> or -<argument> or <argument> to be the passing to the program

## Data Definition

### DB (Define Bytes)

```
DB address "string"
```

Defines a string constant at the specified address.
Example: `DB $1 "Hello, World!"` (using $ prefix)

## Labels

### LBL (Label)

```
LBL name
```

Defines a label that can be jumped to.
Example: `LBL loop`

## Preprocessor Directives

Preprocessor directives are processed before the main compilation phase. They are typically used for including files or defining constants.

### #include

```
#include "path/to/file.masm"
#include "library.module.function"
```

The `#include` directive instructs the compiler to insert the content of another file into the current file at the location of the directive. This is useful for organizing code into reusable modules or using standard libraries.

**Path Resolution:**

1.  **Local Files:** If the path is enclosed in quotes and contains directory separators (`/` or `\`), it's treated as a relative path from the directory of the current source file.
    *   Example: `#include "utils/math.mas"` will look for `math.mas` inside a `utils` subdirectory relative to the current file.

2.  **Standard Library Files:** If the path is enclosed in quotes and uses dot (`.`) notation without explicit directory separators, it's treated as a standard library include. The dots are replaced with the system's directory separator, and the file is searched for within a predefined standard library "root" directory. The compiler checks for both `.mas` and `.masm` extensions.
    *   Example: `#include "stdio.print"` will search for `root/stdio/print.mas` and `root/stdio/print.masm` within the standard library location.

**Supported Extensions:** The compiler typically looks for files with `.mas` or `.masm` extensions.

## Notes

- Registers are referenced as R0, R1, R2, etc.
- String constants can be defined using DB and referenced with a $ prefix
- Labels are referenced with a # prefix in jump instructions
- All numeric values are treated as 64-bit integers

# MNI

Micro assembly can interact with native java code through JMASM or python to pymasm to allow users to make native system calls ether through the JNI or using C in python through `python.h`

examples of MNI can be found inside [mni-functions](mni-instructions.md)

# Extended MicroASM Instruction Set Documentation

This document details additional instructions for the MicroASM language.

## Bitwise Operations

### AND (Bitwise AND)

```
AND dest src
```

Performs a bitwise AND operation between the source and destination, storing the result in the destination.
Example: `AND R1 R2` - R1 = R1 & R2

### OR (Bitwise OR)

```
OR dest src
```

Performs a bitwise OR operation between the source and destination, storing the result in the destination.
Example: `OR R1 R2` - R1 = R1 | R2

### XOR (Bitwise XOR)

```
XOR dest src
```

Performs a bitwise XOR operation between the source and destination, storing the result in the destination.
Example: `XOR R1 R2` - R1 = R1 ^ R2

### NOT (Bitwise NOT)

```
NOT dest
```

Inverts all bits in the destination register.
Example: `NOT R1` - R1 = ~R1

### SHL (Shift Left)

```
SHL dest count
```

Shifts the bits in the destination register left by the specified count.
Example: `SHL R1 R2` - R1 = R1 << R2

### SHR (Shift Right)

```
SHR dest count
```

Shifts the bits in the destination register right by the specified count.
Example: `SHR R1 R2` - R1 = R1 >> R2

## Memory Addressing Extensions

### MOVADDR (Move from Address with Offset)

```
MOVADDR dest src offset
```

Copies a value from the memory address calculated as src+offset to the destination register.
Example: `MOVADDR R1 R2 R3` - R1 = Memory[R2 + R3]

### MOVTO (Move to Address with Offset)

```
MOVTO dest offset src
```

Copies a value from the source register to the memory address calculated as dest+offset.
Example: `MOVTO R1 R2 R3` - Memory[R1 + R2] = R3

## Additional Flow Control

### JNE (Jump if Not Equal)

```
JNE label
```

Jumps to the specified label if the previous comparison result was "not equal".
Example: `JNE #not_equal`

### JG (Jump if Greater)

```
JG label
```

Jumps to the specified label if the previous comparison result was "greater than".
Example: `JG #greater`

### JLE (Jump if Less or Equal)

```
JLE label
```

Jumps to the specified label if the previous comparison result was "less than or equal".
Example: `JLE #less_or_equal`

### JGE (Jump if Greater or Equal)

```
JGE label
```

Jumps to the specified label if the previous comparison result was "greater than or equal".
Example: `JGE #greater_or_equal`

## Stack Frame Management

### ENTER (Create Stack Frame)

```
ENTER framesize
```

Creates a stack frame of the specified size (combines PUSH RBP, MOV RBP RSP, SUB RSP framesize).
Example: `ENTER 64` - Creates a 64-byte stack frame

### LEAVE (Destroy Stack Frame)

```
LEAVE
```

Destroys the current stack frame (combines MOV RSP RBP, POP RBP).
Example: `LEAVE`

## String/Memory Operations

### COPY (Memory Copy)

```
COPY dest src len
```

Copies len bytes from the source address to the destination address.
Example: `COPY R1 R2 R3` - Copies R3 bytes from address R2 to address R1

### FILL (Memory Fill)

```
FILL dest value len
```

Fills len bytes at the destination address with the specified value.
Example: `FILL R1 R2 R3` - Fills R3 bytes at address R1 with value R2

### CMP_MEM (Memory Compare)

```
CMP_MEM dest src len
```

Compares len bytes at the destination and source addresses and sets flags for conditional jumps.
Example: `CMP_MEM R1 R2 R3` - Compares R3 bytes at addresses R1 and R2

## Additional MNI Functions

### Math Operations

```nasm
MNI Math.sin R1 R2        ; Calculate sine of angle in R1, result in R2
MNI Math.cos R1 R2        ; Calculate cosine of angle in R1, result in R2
MNI Math.tan R1 R2        ; Calculate tangent of angle in R1, result in R2
MNI Math.sqrt R1 R2       ; Calculate square root of R1, result in R2
MNI Math.pow R1 R2 R3     ; Calculate R1^R2, result in R3
MNI Math.log R1 R2        ; Calculate natural logarithm of R1, result in R2
MNI Math.round R1 R2      ; Round R1 to nearest integer, result in R2
MNI Math.floor R1 R2      ; Floor R1, result in R2
MNI Math.ceil R1 R2       ; Ceiling R1, result in R2
MNI Math.random R1        ; Generate random number between 0 and 100, result in R1
```

### Memory Management

```nasm
MNI Memory.allocate R1 R2  ; Allocate R1 bytes, address stored in R2
MNI Memory.free R1         ; Free memory at address R1
MNI Memory.copy R1 R2 R3   ; Copy R3 bytes from address R1 to address R2
MNI Memory.set R1 R2 R3    ; Set R3 bytes at address R1 to value R2
MNI Memory.zeroFill R1 R2  ; Fill R2 bytes at address R1 with zeros
```

### Advanced String Operations

```nasm
MNI StringOperations.toUpper R1 R2    ; Convert string at R1 to uppercase, result at R2
MNI StringOperations.toLower R1 R2    ; Convert string at R1 to lowercase, result at R2
MNI StringOperations.trim R1 R2       ; Trim whitespace from string at R1, result at R2
MNI StringOperations.parseInt R1 R2   ; Parse string at R1 to integer, result in R2
MNI StringOperations.parseFloat R1 R2 ; Parse string at R1 to float, result in R2
MNI StringOperations.format R1 R2 R3  ; Format string at R1 with args at R2, result at R3
MNI StringOperations.contains R1 R2   ; Check if string at R1 contains string at R2, result in RFLAGS
MNI StringOperations.startsWith R1 R2 ; Check if string at R1 starts with string at R2, result in RFLAGS
MNI StringOperations.endsWith R1 R2   ; Check if string at R1 ends with string at R2, result in RFLAGS
```

### Data Structures

```nasm
MNI DataStructures.createList R1      ; Create list, handle stored in R1
MNI DataStructures.destroyList R1     ; Destroy list with handle R1
MNI DataStructures.addItem R1 R2      ; Add item from address R2 to list R1
MNI DataStructures.getItem R1 R2 R3   ; Get item at index R2 from list R1, store at address R3
MNI DataStructures.removeItem R1 R2   ; Remove item at index R2 from list R1
MNI DataStructures.getSize R1 R2      ; Get size of list R1, result in R2
MNI DataStructures.clear R1           ; Clear all items from list R1

MNI DataStructures.createMap R1       ; Create map, handle stored in R1
MNI DataStructures.destroyMap R1      ; Destroy map with handle R1
MNI DataStructures.putItem R1 R2 R3   ; Put item from address R3 with key at address R2 into map R1
MNI DataStructures.getMapItem R1 R2 R3 ; Get item with key at address R2 from map R1, store at address R3
MNI DataStructures.removeMapItem R1 R2 ; Remove item with key at address R2 from map R1
MNI DataStructures.hasKey R1 R2       ; Check if map R1 has key at address R2, result in RFLAGS
```

### Network Operations

```nasm
MNI Network.httpGet R1 R2            ; HTTP GET request to URL at address R1, response stored at address R2
MNI Network.httpPost R1 R2 R3        ; HTTP POST request to URL at R1 with data at R2, response at R3
MNI Network.openSocket R1 R2 R3      ; Open socket to host at address R1, port R2, handle stored in R3
MNI Network.closeSocket R1           ; Close socket with handle R1
MNI Network.sendData R1 R2 R3        ; Send R3 bytes of data at address R2 through socket R1
MNI Network.receiveData R1 R2 R3     ; Receive up to R3 bytes from socket R1, store at address R2
MNI Network.getHostByName R1 R2      ; Get IP address of hostname at address R1, store at address R2
```

### Debugging Operations

```nasm
MNI Debug.breakpoint                 ; Insert a breakpoint for debugger
MNI Debug.dumpRegisters R1           ; Dump all registers to address R1
MNI Debug.watchAddress R1            ; Set watch on memory address R1
MNI Debug.printStack R1              ; Print R1 elements from the stack
MNI Debug.traceOn                    ; Enable instruction tracing
MNI Debug.traceOff                   ; Disable instruction tracing
```

### File System Operations

```nasm
MNI FileSystem.open R1 R2 R3         ; Open file at path R1 with mode R2 (r,w,a), handle in R3
MNI FileSystem.close R1              ; Close file with handle R1
MNI FileSystem.read R1 R2 R3 R4      ; Read R3 bytes from file R1 into buffer R2, bytes read in R4
MNI FileSystem.write R1 R2 R3 R4     ; Write R3 bytes from buffer R2 to file R1, bytes written in R4
MNI FileSystem.seek R1 R2            ; Seek to position R2 in file R1
MNI FileSystem.tell R1 R2            ; Get current position in file R1, store in R2
MNI FileSystem.fileSize R1 R2        ; Get size of file with handle R1, store in R2
MNI FileSystem.delete R1             ; Delete file at path R1
MNI FileSystem.exists R1             ; Check if file at path R1 exists, result in RFLAGS
MNI FileSystem.mkdir R1              ; Create directory at path R1
MNI FileSystem.rmdir R1              ; Remove directory at path R1
```

### System Operations

```nasm
MNI System.exec R1 R2                ; Execute command at address R1, exit code in R2
MNI System.sleep R1                  ; Sleep for R1 milliseconds
MNI System.getEnv R1 R2              ; Get environment variable at address R1, store at address R2
MNI System.setEnv R1 R2              ; Set environment variable at address R1 to value at address R2
MNI System.getTime R1                ; Get current time in milliseconds since epoch, store in R1
MNI System.getDate R1                ; Get current date as string, store at address R1
```

## Example Program Using New Instructions

Here's a complete example that demonstrates several of the new instructions:

```nasm
; Allocate memory for strings
MNI Memory.allocate 100 R1    ; Allocate buffer for input string
MNI Memory.allocate 100 R2    ; Allocate buffer for upper case result
MNI Memory.allocate 200 R3    ; Allocate buffer for formatted output

; Store a test string
DB $1000 "Hello, World!"
MOV R4 1000
COPY R1 R4 13                 ; Copy the string to our allocated buffer

; Convert to uppercase
MNI StringOperations.toUpper R1 R2

; Format a string with the result
DB $2000 "Original: %s, Uppercase: %s"
MOV R4 2000
MNI StringOperations.format R4 R1 R3  ; Format with both strings

; Output the result
MNI IO.write 1 R3
MNI IO.flush 1

; Free allocated memory
MNI Memory.free R1
MNI Memory.free R2
MNI Memory.free R3

HLT
```

## Notes

- Register values often contain memory addresses for MNI functions that work with strings or complex data
- For MNI functions, check the documentation to understand whether a register should contain a direct value or a memory address
- All numeric values are treated as 64-bit integers unless specified otherwise
- String operations assume null-terminated strings
- Always free allocated memory to prevent memory leaks
- **MNI in C++:** The C++ implementation typically uses a `registerMNI` function call within the C++ code itself to make MNI functions available to the MicroASM interpreter, rather than relying solely on dynamic loading from external libraries at runtime. Loaders for external libraries (DLLs, JARs) can be built on top of this registration system.
