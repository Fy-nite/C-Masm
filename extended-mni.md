# Extended MNI Functions for Interpreter

## InterpreterOps Module

### Memory Management
```nasm
MNI InterpreterOps.createVirtualMemory R1    ; R1 = size in bytes
                                            ; Returns handle in RFLAGS
MNI InterpreterOps.readVirtual R1 R2 R3     ; R1 = handle, R2 = address, R3 = destination
MNI InterpreterOps.writeVirtual R1 R2 R3    ; R1 = handle, R2 = address, R3 = value
```

### Instruction Parsing
```nasm
MNI InterpreterOps.parseInstruction R1 R2    ; R1 = instruction string
                                            ; R2 = result struct address
                                            ; Format: [opcode][arg1][arg2][arg3]
MNI InterpreterOps.isValidOpcode R1         ; R1 = opcode string
                                            ; Sets RFLAGS to 1 if valid
```

### Virtual Register Operations
```nasm
MNI InterpreterOps.createRegisterBank R1     ; Creates virtual register bank
                                            ; R1 = number of registers
                                            ; Returns handle in RFLAGS
MNI InterpreterOps.readRegister R1 R2 R3    ; R1 = bank handle, R2 = reg number
                                            ; Result in R3
MNI InterpreterOps.writeRegister R1 R2 R3   ; R1 = bank handle, R2 = reg number
                                            ; R3 = value
```

---
**Note on C++ Implementation:** In the C++ interpreter/decoder, MNI functions (including these extended ones if implemented) are typically registered programmatically using a `registerMNI` function during initialization.
