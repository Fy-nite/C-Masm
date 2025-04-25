# MicroASM Bytecode Format & Manual Binary Construction

## 1. Binary File Layout

A MicroASM `.bin` file consists of:
- **Header** (24 bytes)
- **Code Segment** (bytecode instructions)
- **Data Segment** (raw bytes, e.g. strings)

### Header Structure (24 bytes)
| Offset | Size | Field        | Description                |
|--------|------|--------------|----------------------------|
| 0      | 4    | magic        | 0x4D53414D ("MASM")        |
| 4      | 2    | version      | 1                          |
| 6      | 2    | reserved     | 0                          |
| 8      | 4    | codeSize     | Code segment size (bytes)  |
| 12     | 4    | dataSize     | Data segment size (bytes)  |
| 16     | 4    | entryPoint   | Code offset to start exec  |
| 20     | 4    | (unused)     | 0 (padding)                |

All integer fields are **little-endian**.

---

## 2. Instruction Encoding

Each instruction is encoded as:
- **1 byte**: Opcode
- For each operand:
  - **1 byte**: Operand type
  - **1-4 bytes**: Operand value (int32, little-endian)

### Operand Types
| Value | Name                  | Description                        | Size           |
|-------|-----------------------|------------------------------------|----------------|
| 0x00  | NONE                  | (Special, e.g. MNI end marker)     |0 byte          |
| 0x01  | REGISTER              | Register index (see below)         |1 byte          |
| 0x02  | IMMEDIATE             | 32-bit integer                     |1-4 bytes       |
| 0x03  | LABEL_ADDRESS         | Code offset                        |1-4 bytes       |
| 0x04  | DATA_ADDRESS          | Data segment offset                |1-4 bytes       |
| 0x05  | REGISTER_AS_ADDRESS   | Register index (address in reg)    |1 byte          |

Operand types also encode the size of the operand. Operands have the format of 0xST where S is the size of the operand in bytes and T is the operand type.

### Register Indices
| Name | Index |
|------|-------|
| RAX  | 0     |
| RBX  | 1     |
| RCX  | 2     |
| RDX  | 3     |
| RSI  | 4     |
| RDI  | 5     |
| RBP  | 6     |
| RSP  | 7     |
| R0   | 8     |
| ...  | ...   |
| R15  | 23    |

---

## 3. Opcode Table

| Name      | Value | Operand Count | Description                        |
|-----------|-------|--------------|------------------------------------|
| MOV       | 0x01  | 2            | MOV dest, src                      |
| ADD       | 0x02  | 2            | ADD dest, src                      |
| SUB       | 0x03  | 2            | SUB dest, src                      |
| MUL       | 0x04  | 2            | MUL dest, src                      |
| DIV       | 0x05  | 2            | DIV dest, src                      |
| INC       | 0x06  | 1            | INC reg                            |
| JMP       | 0x07  | 1            | JMP addr                           |
| CMP       | 0x08  | 2            | CMP a, b                           |
| JE        | 0x09  | 1            | JE addr                            |
| JL        | 0x0A  | 1            | JL addr                            |
| CALL      | 0x0B  | 1            | CALL addr                          |
| RET       | 0x0C  | 0            | RET                                |
| PUSH      | 0x0D  | 1            | PUSH src                           |
| POP       | 0x0E  | 1            | POP dest                           |
| OUT       | 0x0F  | 2            | OUT port, value                    |
| COUT      | 0x10  | 2            | COUT port, value                   |
| OUTSTR    | 0x11  | 3            | OUTSTR port, addr, len             |
| OUTCHAR   | 0x12  | 2            | OUTCHAR port, addr                 |
| HLT       | 0x13  | 0            | HLT                                |
| ARGC      | 0x14  | 1            | ARGC dest                          |
| GETARG    | 0x15  | 2            | GETARG dest, index                 |
| ...       | ...   | ...          | ...                                |
| MNI       | 0x32  | variable     | MNI function call                  |
| IN        | 0x33  | 1            | IN addr                            |

---

## 4. Example: Manual `.bin` Construction

Suppose you want a program that reads a line from stdin and prints it:

```asm
lbl main
    in $100
    out 1 $100
hlt
```

### Step 1: Header (24 bytes)
- magic: 0x4D53414D
- version: 1
- reserved: 0
- codeSize: 18
- dataSize: 0
- entryPoint: 0

### Step 2: Code Segment (18 bytes)
| Offset | Bytes (hex)         | Meaning                          |
|--------|---------------------|----------------------------------|
| 0      | 33                  | IN opcode (0x33)                 |
| 1      | 14                  | DATA_ADDRESS (0x04) 1 byte       |
| 2      | 64                  | $100 = 0x64                      |
| 3      | 0F                  | OUT opcode (0x0F)                |
| 4      | 12                  | IMMEDIATE (0x02)    1 byte       |
| 5      | 01                  | port 1                           |
| 6      | 14                  | DATA_ADDRESS (0x04) 1 byte       |
| 7      | 64                  | $100 = 0x64                      |
| 8      | 13                  | HLT opcode (0x13)                |

### Step 3: Data Segment
- (empty in this example)

### Step 4: Combine
Concatenate header + code segment + data segment.  
Write as binary (little-endian for all multi-byte fields).

---

## 5. Tips for Manual Construction

- Use a hex editor to create/edit `.bin` files.
- All integers (header fields, operand values) are little-endian.
- Strings in the data segment should be null-terminated if used as C-strings.
- For MNI calls, after the opcode (0x32), write the null-terminated function name, then each operand (type + value), then a NONE (0x00) + 0x00000000 as end marker.

---

## 6. Example: OUTSTR

To print 5 bytes from address $200 to stdout:

| Bytes (hex)         | Meaning                |
|---------------------|------------------------|
| 11                  | OUTSTR opcode (0x11)   |
| 02                  | IMMEDIATE (port) 1 byte|
| 01                  | 1                      |
| 04                  | DATA_ADDRESS     1 byte|
| C8                  | $200 = 0xC8            |
| 02                  | IMMEDIATE (len)  1 byte|
| 05                  | 5                      |

---

## 7. Reference

- See `src/microasm_compiler.cpp` and `src/microasm_interpreter.cpp` for details.
- For more instructions, see the [full instruction set documentation](v2instructions.md).

