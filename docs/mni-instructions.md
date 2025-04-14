# MNI (Module Native Interface) Documentation

MNI functions allow you to extend MicroASM with native modules written in Java/Kotlin. Here's how to use them in your assembly code.

## Basic Syntax

```
MNI ModuleName.functionName arg1 arg2 arg3...
```

arguments that are passed to the function are required to be a array of items. say a char* array in case of strings in C/C++.

## Available Modules

### StringOperations

#### String Comparison
```nasm
MNI StringOperations.cmp R1 R2    ; Compare strings at memory locations in R1 and R2
                                 ; Sets RFLAGS to 1 if equal, 0 if not
```

#### String Concatenation
```nasm
MNI StringOperations.concat R1 R2 R3    ; Concatenate strings at R1 and R2
                                       ; Result stored at memory location R3
```

#### String Length
```nasm
MNI StringOperations.length R1 R2    ; Get length of string at R1
                                    ; Result stored in register R2
```

#### String Split
```nasm
MNI StringOperations.split R1 R2 $5000    ; Split string at R1 using delimiter at R2
                                         ; Results stored starting at memory address $5000
```

#### String Replace
```nasm
MNI StringOperations.replace R1 R2 R3 R4    ; Replace in string at R1
                                           ; Replace R2 with R3
                                           ; Result stored at R4
```

### IO Operations

#### Write Output
```nasm
MNI IO.write 1 R1    ; Write string at R1 to stdout
MNI IO.write 2 R1    ; Write string at R1 to stderr
```

#### Flush Output
```nasm
MNI IO.flush 1       ; Flush stdout
MNI IO.flush 2       ; Flush stderr
```

### FileOperations

#### Read File
```nasm
; Example of reading a file
DB $1000 "example.txt"    ; Store filename at memory address $1000
MNI FileOperations.readFile R1 $2000    ; Read file named at R1
                                       ; Content stored starting at $2000
```

## Example Program

Here's a complete example that demonstrates several MNI functions:

```nasm
; Store two strings
DB $1000 "Hello, "
DB $2000 "World!"

; Concatenate strings
MOV R1 1000
MOV R2 2000
MOV R3 3000
MNI StringOperations.concat R1 R2 R3

; Print result
MOV R1 3000
MNI IO.write 1 R1
MNI IO.flush 1

; Get string length
MOV R1 3000
MNI StringOperations.len R1 R4
MOV R1 R4
OUT 1 R1    ; Print the length

HLT
```

## Notes

- Memory addresses prefixed with $ are used for data storage
- Register values are used for temporary storage and calculations
- Always ensure you have enough memory allocated for string operations
- Some operations modify the RFLAGS register which can be used for conditional jumps
- **C++ Implementation:** In the C++ interpreter/decoder, MNI functions are typically registered programmatically using a `registerMNI(module, name, function_pointer)` function during initialization, rather than being dynamically loaded from external files (like DLLs or JARs) by default. However, external loaders can utilize this registration mechanism.
