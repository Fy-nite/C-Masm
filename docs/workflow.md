# MicroASM Compiler and Interpreter Workflow

This document explains how the MicroASM compiler (`microasm_compiler.cpp`) and interpreter (`microasm_interpreter.cpp`) work together, transforming human-readable assembly code into an executable format and running it.

## 1. Compilation Process (`microasm_compiler`)

The compiler's primary role is to translate the symbolic `.masm` source code into a compact, machine-understandable binary format (`.bin`). This involves several steps:

1.  **Parsing (`parse` and `parseLine`):**
    *   The source file is read line by line.
    *   Each line is processed:
        *   Leading/trailing whitespace is implicitly handled by stream extraction.
        *   Lines starting with `;` (comments) or empty lines are ignored.
        *   The first word on the line is treated as a potential instruction mnemonic or directive. It's converted to uppercase to ensure case-insensitivity (e.g., `mov` becomes `MOV`).
        *   Subsequent words on the line are treated as operands for the instruction/directive.

2.  **Label Handling (`LBL` Directive):**
    *   If the first word is `LBL`, the next word is taken as the label name.
    *   The compiler maintains a `labelMap` (e.g., `std::unordered_map<std::string, int>`).
    *   It stores an entry in `labelMap` where the key is the label name prefixed with `#` (e.g., `#loop_start`) and the value is the `currentAddress`.
    *   `currentAddress` tracks the byte offset within the *code segment* where the next instruction *would* be placed. This ensures jumps point to the correct location in the final binary's code section.

3.  **Data Handling (`DB` Directive):**
    *   If the first word is `DB`, the next word is expected to be a data label (e.g., `$1`, `$myString`). The word after that is expected to be a double-quoted string literal.
    *   The compiler maintains a `dataLabels` map (e.g., `std::unordered_map<std::string, int>`) and a `dataSegment` buffer (e.g., `std::vector<char>`).
    *   The current `dataAddress` (tracking the size of `dataSegment`) is stored in `dataLabels` with the data label as the key. This records the starting offset of this data within the data segment.
    *   The string literal is processed:
        *   Quotes are removed.
        *   Escape sequences (like `\n`, `\t`, `\\`, `\"`) are converted into their corresponding single byte values.
        *   The resulting bytes are appended to the `dataSegment` buffer.
        *   A null terminator (`\0`) is appended to the `dataSegment` buffer.
    *   The `dataAddress` counter is incremented by the total number of bytes added (processed string length + 1 for null terminator).

4.  **Instruction & Operand Processing:**
    *   If the first word is a recognized instruction mnemonic (not `LBL` or `DB`):
        *   The mnemonic is looked up in an `opcodeMap` to get its numerical `Opcode` value (e.g., `MOV` -> `0x01`).
        *   The remaining words on the line are stored as operand strings associated with this instruction.
        *   An internal `Instruction` structure (holding the opcode and operand strings) is added to a list (`instructions`).
        *   The `currentAddress` (code offset) is incremented. The size added is 1 byte for the opcode, plus `N * (1 + 4)` bytes for the operands, where `N` is the number of operands (1 byte for `OperandType`, 4 bytes for `value`). This calculation anticipates the final binary size.

5.  **Operand Resolution (`resolveOperand`):**
    *   This helper function is crucial both during parsing (for address calculation, though less critical) and especially during binary generation. It takes an operand string and determines its type and numerical value for the bytecode.
    *   **Labels (`#label_name`):** Looks up `#label_name` in `labelMap`. Returns type `LABEL_ADDRESS` and the stored code offset value. Throws an error if the label is not found.
    *   **Data Labels (`$data_label`):** Looks up `$data_label` in `dataLabels`. Returns type `DATA_ADDRESS` and the stored data offset value. Throws an error if not found.
    *   **Registers (`RAX`, `R0`, etc.):** Converts the name to uppercase. Looks up the name in a `regMap`. Returns type `REGISTER` and the register's index (0-23). Throws an error if not a valid register.
    *   **Immediates (`123`, `$500`):** Attempts to parse the string (after removing a leading `$` if present) as an integer. Returns type `IMMEDIATE` and the integer value. Throws an error if parsing fails or the value is out of the 32-bit signed range.

6.  **Binary File Generation (`compile` function):**
    *   This function orchestrates writing the final `.bin` file.
    *   **Calculate Sizes:** The actual code size is recalculated by iterating through the `instructions` list and summing the sizes (1 for opcode + N*(1+4) for operands), *skipping* any `DB` pseudo-instructions encountered during the initial parse. The data size is simply the final size of the `dataSegment` buffer.
    *   **Create Header:** A `BinaryHeader` struct is populated with the magic number (`0x4D53414D`), version, calculated `codeSize`, calculated `dataSize`, and the `entryPoint` (currently hardcoded to 0, meaning execution starts at the beginning of the code segment).
    *   **Write Header:** The `BinaryHeader` struct is written as raw bytes to the beginning of the output `.bin` file.
    *   **Write Code Segment:** The compiler iterates through the `instructions` list again:
        *   Pseudo-instructions like `DB` are skipped.
        *   For each actual instruction:
            *   The `Opcode` byte is written.
            *   For each operand string associated with the instruction:
                *   `resolveOperand` is called to get the final `OperandType` and `value`.
                *   The `OperandType` byte is written.
                *   The `value` (as a 4-byte integer) is written.
    *   **Write Data Segment:** The entire contents of the `dataSegment` buffer (containing all processed strings and null terminators from `DB` directives) are written to the file immediately following the code segment.

## 2. Binary Format (`.bin`)

The compiler produces a `.bin` file with a specific layout, essential for the interpreter to understand:

```
+-----------------------+ <-- File Start
|     BinaryHeader      | (Fixed size, e.g., 16 bytes)
| - magic: uint32_t     | (e.g., 0x4D53414D for "MASM")
| - version: uint16_t   | (e.g., 1)
| - reserved: uint16_t  | (Padding/Future use, currently 0)
| - codeSize: uint32_t  | (Size of the Code Segment in bytes)
| - dataSize: uint32_t  | (Size of the Data Segment in bytes)
| - entryPoint: uint32_t| (Offset within Code Segment to start execution)
+-----------------------+ <-- Header End / Code Segment Start
|      Code Segment     | (Variable size: header.codeSize bytes)
| - Opcode (1 byte)     | (e.g., 0x01 for MOV)
| - OperandType (1 byte)| (e.g., 0x01 for REGISTER)
| - OperandValue (4 B)  | (e.g., 0 for RAX, or an immediate value, or an offset)
| - OperandType (1 byte)| (If instruction has >1 operand)
| - OperandValue (4 B)  | (...)
| - Opcode (1 byte)     | (Next instruction)
| - ...                 |
+-----------------------+ <-- Code Segment End / Data Segment Start
|      Data Segment     | (Variable size: header.dataSize bytes)
| - Raw bytes from DB   | (e.g., 'H','e','l','l','o','\0', 'W','o','r',...)
| - ...                 |
+-----------------------+ <-- Data Segment End / File End
```

## 3. Interpretation Process (`microasm_interpreter`)

The interpreter executes the instructions contained within the `.bin` file.

1.  **Loading (`load` function):**
    *   The specified `.bin` file is opened in binary mode.
    *   **Read Header:** The first `sizeof(BinaryHeader)` bytes are read from the file into a `BinaryHeader` struct.
    *   **Validate Header:** The `magic` number is checked against `0x4D53414D`. If it doesn't match, it's not a valid file, and an error is thrown. Version checks could also be performed here.
    *   **Allocate RAM:** A `std::vector<char>` named `ram` is created with a specified size (e.g., 65536 bytes for 64KB). This simulates the computer's main memory.
    *   **Determine Data Segment Base:** A variable `dataSegmentBase` is calculated. This is the starting address *within the simulated RAM* where the data segment will be loaded (e.g., `ram.size() / 2`). This base address is crucial for resolving `DATA_ADDRESS` operands later.
    *   **Load Code Segment:** `header.codeSize` bytes are read from the file (immediately following the header) into the `bytecode_raw` buffer (`std::vector<uint8_t>`). This buffer now contains only the executable instructions and their operands.
    *   **Load Data Segment:** `header.dataSize` bytes are read from the file (immediately following the code segment) directly into the `ram` vector, starting at the `dataSegmentBase` offset.
    *   **Set Instruction Pointer:** The interpreter's instruction pointer (`ip`, an integer index into `bytecode_raw`) is initialized to the value specified by `header.entryPoint`.

2.  **Execution Loop (`execute` function):**
    *   The interpreter enters a `while` loop that continues as long as `ip` points to a valid location within the `bytecode_raw` buffer (`ip < bytecode_raw.size()`).
    *   **Fetch Opcode:** The byte at `bytecode_raw[ip]` is read. This is the `Opcode` for the current instruction. `ip` is incremented by 1.
    *   **Decode & Fetch Operands (`nextRawOperand`):**
        *   Based on the fetched `Opcode`, the interpreter knows how many operands to expect (using a lookup table or switch statement).
        *   For each expected operand, `nextRawOperand` is called.
        *   `nextRawOperand` reads the `OperandType` byte from `bytecode_raw[ip]` (incrementing `ip`) and then reads the next 4 bytes as the operand's raw `value` (incrementing `ip` by 4). It returns a `BytecodeOperand` struct containing the type and value.
    *   **Resolve Operand Values (`getValue`):**
        *   Before the instruction logic uses an operand, `getValue` is often called on the `BytecodeOperand` struct obtained from `nextRawOperand`.
        *   If type is `REGISTER`: It returns the integer value currently stored in the `registers` vector at the index specified by `operand.value`.
        *   If type is `IMMEDIATE` or `LABEL_ADDRESS`: It returns `operand.value` directly, as this value represents a literal number or a code offset (which is used directly as the target for jumps/calls).
        *   If type is `DATA_ADDRESS`: It calculates and returns the absolute RAM address: `dataSegmentBase + operand.value`. This translates the data offset (from the bytecode) into a usable memory address within the simulated `ram`.
    *   **Execute Instruction:** A large `switch` statement based on the `Opcode` performs the required action:
        *   **Register Modification:** Instructions like `MOV`, `ADD`, `SUB`, `POP` read values (using `getValue` if necessary) and write results directly into the `registers` vector.
        *   **Memory Modification:** Instructions like `MOVTO`, `FILL`, `COPY` calculate absolute RAM addresses (using `getValue` for base addresses and offsets) and use helper functions (`writeRamInt`, `writeRamChar`, `memcpy`, `memset`) to modify the `ram` vector. `MOVADDR` reads from RAM using `readRamInt`.
        *   **Flow Control:** `JMP`, `CALL`, and conditional jumps (`JE`, `JNE`, etc.) modify the `ip` register directly. `CALL` also pushes the *next* instruction's address (`ip` *after* fetching operands) onto the stack before changing `ip`. `RET` pops an address from the stack into `ip`.
        *   **Flags:** `CMP` and `CMP_MEM` calculate results and update internal boolean flags (`zeroFlag`, `signFlag`). Conditional jumps read these flags.
        *   **Stack Pointer:** `PUSH`, `POP`, `CALL`, `RET`, `ENTER`, `LEAVE` modify the `RSP` register (index 7) and interact with RAM via `readRamInt`/`writeRamInt` at the `RSP` address (adjusting `RSP` before/after).
        *   **I/O:** `OUT`, `COUT`, etc., read values/addresses (using `getValue`), potentially read strings/chars from `ram` using helpers (`readRamString`, `readRamChar`), and print to `std::cout` or `std::cerr`.
    *   **Loop Continuation:** The loop fetches the next opcode unless `HLT` was executed (which terminates the loop/program) or an error occurred.

This detailed process ensures that the symbolic assembly code is correctly translated into executable bytecode, and the interpreter can accurately load and run that bytecode by managing registers, simulated RAM, and the instruction pointer according to the defined instruction set.
