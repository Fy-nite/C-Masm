#include <iostream>
#include <fstream>
#include <vector>
#include <stack> // Use std::stack for the stack
#include <stdexcept>
#include <string> // For OUTSTR etc.
#include <vector> // For memory, bytecode
#include <cmath> // For potential future math ops
#include <limits> // For integer limits
#include <cstring> // Required for memcpy, memset, memcmp
#include <cstdint> // Required for uint32_t
#include "operand_types.h" // Include the new operand type definitions

// Define the same binary header structure as the compiler
struct BinaryHeader {
    uint32_t magic = 0x4D53414D; // "MASM" in ASCII (little-endian)
    uint16_t version = 1;
    uint16_t reserved = 0; // Padding/Reserved for future use
    uint32_t codeSize = 0;
    uint32_t dataSize = 0;
    uint32_t entryPoint = 0; // Offset within the code segment
};

// Use the same Opcode enum as the compiler - MOVI removed
enum Opcode {
    // Basic
    MOV = 0x01, ADD, SUB, MUL, DIV, INC, // MOVI removed
    // Flow Control
    JMP, CMP, JE, JL, CALL, RET,
    // Stack
    PUSH, POP,
    // I/O
    OUT, COUT, OUTSTR, OUTCHAR,
    // Program Control
    HLT, ARGC, GETARG,
    // Data Definition
    DB,
    // Labels
    LBL,
    // Bitwise
    AND, OR, XOR, NOT, SHL, SHR,
    // Memory Addressing
    MOVADDR, MOVTO,
    // Additional Flow Control
    JNE, JG, JLE, JGE,
    // Stack Frame
    ENTER, LEAVE,
    // String/Memory Ops
    COPY, FILL, CMP_MEM
};

// Structure to hold operand info read from bytecode
struct BytecodeOperand {
    OperandType type;
    int value;
};

class Interpreter {
    // Registers: RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP (0-7) + R0-R15 (8-23)
    std::vector<int> registers;
    std::vector<char> ram; // Main memory (bytes)
    std::vector<uint8_t> bytecode_raw; // Store raw bytes of the CODE segment only
    int dataSegmentBase; // Base address in RAM where the data segment is loaded
    int ip = 0; // Instruction pointer (index into bytecode_raw vector)
    int sp; // Stack Pointer (index into RAM, managed by RSP register)
    int bp; // Base Pointer (index into RAM, managed by RBP register)

    // Flags for CMP result
    bool zeroFlag = false;
    bool signFlag = false; // True if result is negative

    // Command line arguments storage
    std::vector<std::string> cmdArgs;

    // Helper to read next operand from bytecode
    BytecodeOperand nextRawOperand() {
        if (ip + 1 + sizeof(int) > bytecode_raw.size()) { // Check size for type byte + value int
             throw std::runtime_error("Unexpected end of bytecode reading typed operand (IP: " + std::to_string(ip) + ", CodeSize: " + std::to_string(bytecode_raw.size()) + ")");
        }
        BytecodeOperand operand;
        operand.type = static_cast<OperandType>(bytecode_raw[ip++]);
        operand.value = *reinterpret_cast<const int*>(&bytecode_raw[ip]);
        ip += sizeof(int);
        return operand;
    }

    // Helper to get the actual value based on operand type
    int getValue(const BytecodeOperand& operand) {
        switch (operand.type) {
            case OperandType::REGISTER:
                if (operand.value < 0 || operand.value >= registers.size()) {
                    throw std::runtime_error("Invalid register index encountered: " + std::to_string(operand.value));
                }
                return registers[operand.value];
            case OperandType::IMMEDIATE:
            case OperandType::LABEL_ADDRESS: // Addresses are immediate values in bytecode
                return operand.value;
            case OperandType::DATA_ADDRESS:
                // Return the absolute RAM address (Base + Offset)
                // dataSegmentBase is now correctly set during loading
                if (dataSegmentBase + operand.value < 0 || dataSegmentBase + operand.value >= ram.size()) {
                     throw std::runtime_error("Data address out of RAM bounds: Base=" + std::to_string(dataSegmentBase) + ", Offset=" + std::to_string(operand.value));
                }
                return dataSegmentBase + operand.value;
            default:
                throw std::runtime_error("Cannot get value for unknown or invalid operand type: " + std::to_string(static_cast<int>(operand.type)));
        }
    }

    // Helper to get register index, throws if not a register operand
    int getRegisterIndex(const BytecodeOperand& operand) {
        if (operand.type != OperandType::REGISTER) {
            throw std::runtime_error("Expected register operand, got type " + std::to_string(static_cast<int>(operand.type)));
        }
        if (operand.value < 0 || operand.value >= registers.size()) {
            throw std::runtime_error("Invalid register index encountered: " + std::to_string(operand.value));
        }
        return operand.value;
    }

    // Memory access helpers (handle potential out-of-bounds)
    int readRamInt(int address) {
        if (address < 0 || address + sizeof(int) > ram.size()) {
            throw std::runtime_error("Memory read out of bounds at address: " + std::to_string(address));
        }
        return *reinterpret_cast<int*>(&ram[address]);
    }

    void writeRamInt(int address, int value) {
        if (address < 0 || address + sizeof(int) > ram.size()) {
            throw std::runtime_error("Memory write out of bounds at address: " + std::to_string(address));
        }
        *reinterpret_cast<int*>(&ram[address]) = value;
    }

     char readRamChar(int address) {
        if (address < 0 || address >= ram.size()) {
            throw std::runtime_error("Memory read out of bounds at address: " + std::to_string(address));
        }
        return ram[address];
    }

    void writeRamChar(int address, char value) {
        if (address < 0 || address >= ram.size()) {
            throw std::runtime_error("Memory write out of bounds at address: " + std::to_string(address));
        }
        ram[address] = value;
    }

    // Helper to read string from RAM (null-terminated)
    std::string readRamString(int address) {
        std::string str = "";
        int currentAddr = address;
        while(true) {
            char c = readRamChar(currentAddr++);
            if (c == '\0') break;
            str += c;
        }
        return str;
    }

    // Stack operations using RAM and RSP register
    void pushStack(int value) {
        registers[7] -= sizeof(int); // Decrement RSP (stack grows down)
        sp = registers[7];
        writeRamInt(sp, value);
    }

    int popStack() {
        int value = readRamInt(sp);
        registers[7] += sizeof(int); // Increment RSP
        sp = registers[7];
        return value;
    }


public:
    // Constructor: Initialize registers, RAM, stack pointer, data segment base
    Interpreter(int ramSize = 65536, const std::vector<std::string>& args = {}) // 64KB RAM default
        : registers(24, 0), // 24 general purpose registers
          ram(ramSize, 0),
          cmdArgs(args)
    {
        // Initialize Stack Pointer (RSP, index 7) to top of RAM
        registers[7] = ramSize;
        sp = registers[7];
        // Base Pointer (RBP, index 6) can be initialized similarly or left 0
        registers[6] = ramSize;
        bp = registers[6];
        // Set data segment base - let's put it in the middle for now
        // Ensure this doesn't collide with stack growing down!
        dataSegmentBase = ramSize / 2;
        if (dataSegmentBase < 0) dataSegmentBase = 0; // Handle tiny RAM sizes
    }

    // Load bytecode from file, read header, load segments
    void load(const std::string& bytecodeFile) {
        std::ifstream in(bytecodeFile, std::ios::binary);
        if (!in) throw std::runtime_error("Failed to open bytecode file: " + bytecodeFile);

        // 1. Read the header
        BinaryHeader header;
        if (!in.read(reinterpret_cast<char*>(&header), sizeof(header))) {
            throw std::runtime_error("Failed to read header from bytecode file: " + bytecodeFile);
        }

        // 2. Validate the header (basic magic number check)
        if (header.magic != 0x4D53414D) { // "MASM"
            throw std::runtime_error("Invalid magic number in bytecode file. Not a MASM binary.");
        }
        // Could add version checks here too

        // 3. Load Code Segment into bytecode_raw
        bytecode_raw.resize(header.codeSize);
        if (header.codeSize > 0) {
            if (!in.read(reinterpret_cast<char*>(bytecode_raw.data()), header.codeSize)) {
                throw std::runtime_error("Failed to read code segment (expected " + std::to_string(header.codeSize) + " bytes)");
            }
        }

        // 4. Load Data Segment into RAM at dataSegmentBase
        if (header.dataSize > 0) {
            if (dataSegmentBase + header.dataSize > ram.size()) {
                throw std::runtime_error("RAM size (" + std::to_string(ram.size()) + ") too small for data segment (size " + std::to_string(header.dataSize) + " at base " + std::to_string(dataSegmentBase) + ")");
            }
            if (!in.read(&ram[dataSegmentBase], header.dataSize)) {
                 throw std::runtime_error("Failed to read data segment (expected " + std::to_string(header.dataSize) + " bytes)");
            }
        }

        // Check if we read exactly the expected amount
        in.peek(); // Try to read one more byte
        if (!in.eof()) {
             std::cerr << "Warning: Extra data found in bytecode file after code and data segments." << std::endl;
        }

        // 5. Set Instruction Pointer to Entry Point
        if (header.entryPoint >= header.codeSize && header.codeSize > 0) { // Allow entryPoint 0 for empty code
             throw std::runtime_error("Entry point (" + std::to_string(header.entryPoint) + ") is outside the code segment (size " + std::to_string(header.codeSize) + ")");
        }
        ip = header.entryPoint;

        // Remove the old data loading hack
        // const char* hw = "Hello, World!\n";
        // ... (hack removed) ...
    }

    // Execute loaded bytecode
    void execute() {
        // ip is already set by load()
        // while (ip < bytecode_raw.size()) { // Condition remains the same

        while (ip < bytecode_raw.size()) {
            int currentIp = ip;
            Opcode opcode = static_cast<Opcode>(bytecode_raw[ip++]); // Read opcode

            try {
                switch (opcode) {
                    // Basic Arithmetic
                    case MOV: {
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_src = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest); // Destination must be register
                        registers[dest_reg] = getValue(op_src);   // Get value from source (reg or imm)
                        break;
                    }
                    case ADD: {
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_src = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        registers[dest_reg] += getValue(op_src); // Add value (reg or imm)
                        break;
                    }
                    case SUB: {
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_src = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        registers[dest_reg] -= getValue(op_src);
                        break;
                    }
                    case MUL: {
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_src = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        registers[dest_reg] *= getValue(op_src);
                        break;
                    }
                    case DIV: {
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_src = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        int src_val = getValue(op_src);
                        if (src_val == 0) throw std::runtime_error("Division by zero");
                        registers[dest_reg] /= src_val;
                        break;
                    }
                    case INC: {
                        BytecodeOperand op_dest = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        registers[dest_reg]++;
                        break;
                    }

                    // Flow Control
                    case JMP: {
                        BytecodeOperand op_target = nextRawOperand();
                        // Target should be immediate label address from compiler
                        if (op_target.type != OperandType::LABEL_ADDRESS && op_target.type != OperandType::IMMEDIATE) {
                             throw std::runtime_error("JMP requires immediate/label address operand");
                        }
                        ip = op_target.value; // Jump to absolute address
                        break;
                    }
                    case CMP: {
                        BytecodeOperand op1 = nextRawOperand();
                        BytecodeOperand op2 = nextRawOperand();
                        int val1 = getValue(op1);
                        int val2 = getValue(op2);
                        zeroFlag = (val1 == val2);
                        signFlag = (val1 < val2);
                        break;
                    }
                    // Conditional Jumps (JE, JNE, JL, JG, JLE, JGE)
                    case JE: case JNE: case JL: case JG: case JLE: case JGE: {
                        BytecodeOperand op_target = nextRawOperand();
                         if (op_target.type != OperandType::LABEL_ADDRESS && op_target.type != OperandType::IMMEDIATE) {
                             throw std::runtime_error("Conditional jump requires immediate/label address operand");
                        }
                        bool shouldJump = false;
                        switch(opcode) {
                            case JE: shouldJump = zeroFlag; break;
                            case JNE: shouldJump = !zeroFlag; break;
                            case JL: shouldJump = signFlag; break;
                            case JG: shouldJump = !zeroFlag && !signFlag; break;
                            case JLE: shouldJump = zeroFlag || signFlag; break;
                            case JGE: shouldJump = zeroFlag || !signFlag; break;
                            default: break; // Should not happen
                        }
                        if (shouldJump) {
                            ip = op_target.value;
                        }
                        break;
                    }

                    case CALL: {
                        BytecodeOperand op_target = nextRawOperand();
                         if (op_target.type != OperandType::LABEL_ADDRESS && op_target.type != OperandType::IMMEDIATE) {
                             throw std::runtime_error("CALL requires immediate/label address operand");
                        }
                        pushStack(ip); // Push return address (address AFTER call instruction + operands)
                        ip = op_target.value;   // Jump to function
                        break;
                    }
                    case RET: {
                        if (registers[7] >= ram.size()) throw std::runtime_error("Stack underflow on RET");
                        ip = popStack(); // Pop return address and jump
                        break;
                    }

                    // Stack Operations
                    case PUSH: {
                        BytecodeOperand op_src = nextRawOperand();
                        pushStack(getValue(op_src)); // Push value (reg or imm)
                        break;
                    }
                    case POP: {
                        BytecodeOperand op_dest = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        registers[dest_reg] = popStack();
                        break;
                    }

                    // I/O Operations
                    case OUT: {
                        BytecodeOperand op_port = nextRawOperand();
                        BytecodeOperand op_val = nextRawOperand();
                        int port = getValue(op_port); // Port usually immediate
                        std::ostream& out_stream = (port == 2) ? std::cerr : std::cout;
                        if (port != 1 && port != 2) throw std::runtime_error("Invalid port for OUT: " + std::to_string(port));

                        if (op_val.type == OperandType::DATA_ADDRESS) {
                            // It's an address (offset resolved by compiler, base added by getValue)
                            int address = getValue(op_val); // getValue now correctly calculates absolute RAM address
                            out_stream << readRamString(address); // Removed std::endl
                        } else {
                            // It's an immediate value or register value
                            out_stream << getValue(op_val); // Removed std::endl
                        }
                        break;
                    }
                     case COUT: {
                        BytecodeOperand op_port = nextRawOperand();
                        BytecodeOperand op_val = nextRawOperand(); // Value (reg or imm)
                        int port = getValue(op_port);
                        std::ostream& out_stream = (port == 2) ? std::cerr : std::cout;
                        if (port != 1 && port != 2) throw std::runtime_error("Invalid port for COUT: " + std::to_string(port));
                        out_stream << static_cast<char>(getValue(op_val)); // Output char value
                        break;
                    }
                    case OUTSTR: { // Prints string from RAM address IN REGISTER
                        BytecodeOperand op_port = nextRawOperand();
                        BytecodeOperand op_addr = nextRawOperand();
                        BytecodeOperand op_len = nextRawOperand(); // Length is from register
                        int port = getValue(op_port);
                        std::ostream& out_stream = (port == 2) ? std::cerr : std::cout;
                        if (port != 1 && port != 2) throw std::runtime_error("Invalid port for OUTSTR: " + std::to_string(port));

                        int addr = getValue(op_addr);
                        int len = getValue(op_len);
                        for(int i=0; i<len; ++i) {
                            out_stream << readRamChar(addr + i); // Read char by char
                        }
                        // No automatic newline for OUTSTR
                        break;
                    }
                    case OUTCHAR: { // Prints single char from RAM address IN REGISTER
                        BytecodeOperand op_port = nextRawOperand();
                        BytecodeOperand op_addr = nextRawOperand();
                        int port = getValue(op_port);
                        std::ostream& out_stream = (port == 2) ? std::cerr : std::cout;
                        if (port != 1 && port != 2) throw std::runtime_error("Invalid port for OUTCHAR: " + std::to_string(port));

                        int addr = getValue(op_addr);
                        out_stream << readRamChar(addr); // Read single char
                        // No automatic newline for OUTCHAR
                        break;
                    }

                    // Program Control
                    case HLT: { std::cout << "HLT encountered. Execution finished." << std::endl; return; } // Halt execution
                    case ARGC: { registers[nextRawOperand().value] = cmdArgs.size(); break; }
                    case GETARG: {
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_index = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        int index = getValue(op_index);
                        if (index < 0 || index >= cmdArgs.size()) {
                            throw std::runtime_error("GETARG index out of bounds: " + std::to_string(index));
                        }
                        // How to store string arg in int register? Store address.
                        // Need to copy string to RAM and store address.
                        // Simplified: Not implemented fully. Requires memory management.
                        registers[dest_reg] = 0; // Placeholder
                        std::cerr << "Warning: GETARG not fully implemented (string handling)." << std::endl;
                        break;
                    }

                    // Bitwise Operations
                    case AND: { 
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_src = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        registers[dest_reg] &= getValue(op_src);
                        break; 
                    }
                    case OR: { 
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_src = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        registers[dest_reg] |= getValue(op_src);
                        break; 
                    }
                    case XOR: { 
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_src = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        registers[dest_reg] ^= getValue(op_src);
                        break; 
                    }
                    case NOT: { 
                        BytecodeOperand op_dest = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        registers[dest_reg] = ~registers[dest_reg];
                        break; 
                    }
                    case SHL: { 
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_count = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        registers[dest_reg] <<= getValue(op_count);
                        break; 
                    }
                    case SHR: { 
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_count = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        registers[dest_reg] >>= getValue(op_count);
                        break; 
                    } // Arithmetic right shift

                    // Memory Addressing
                    case MOVADDR: { // MOVADDR dest_reg src_addr_reg offset_reg
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_src_addr = nextRawOperand();
                        BytecodeOperand op_offset = nextRawOperand();
                        int dest_reg = getRegisterIndex(op_dest);
                        int address = getValue(op_src_addr) + getValue(op_offset);
                        registers[dest_reg] = readRamInt(address);
                        break;
                    }
                    case MOVTO: { // MOVTO dest_addr_reg offset_reg src_reg
                        BytecodeOperand op_dest_addr = nextRawOperand();
                        BytecodeOperand op_offset = nextRawOperand();
                        BytecodeOperand op_src = nextRawOperand();
                        int address = getValue(op_dest_addr) + getValue(op_offset);
                        writeRamInt(address, getValue(op_src));
                        break;
                    }

                    // Stack Frame Management
                    case ENTER: { // ENTER framesize (immediate)
                        BytecodeOperand op_frameSize = nextRawOperand();
                        int frameSize = getValue(op_frameSize);
                        pushStack(registers[6]); // Push RBP
                        registers[6] = registers[7]; // MOV RBP, RSP
                        bp = registers[6];
                        registers[7] -= frameSize; // SUB RSP, framesize
                        sp = registers[7];
                        break;
                    }
                    case LEAVE: { // LEAVE
                        registers[7] = registers[6]; // MOV RSP, RBP
                        sp = registers[7];
                        registers[6] = popStack(); // POP RBP
                        bp = registers[6];
                        break;
                    }

                    // String/Memory Operations
                    case COPY: { // COPY dest_addr_reg src_addr_reg len_reg
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_src = nextRawOperand();
                        BytecodeOperand op_len = nextRawOperand();
                        int dest_addr = getValue(op_dest);
                        int src_addr = getValue(op_src);
                        int len = getValue(op_len);
                        if (len < 0) throw std::runtime_error("COPY length cannot be negative");
                        if (dest_addr < 0 || dest_addr + len > ram.size() || src_addr < 0 || src_addr + len > ram.size()) {
                             throw std::runtime_error("COPY memory access out of bounds");
                        }
                        // Use memcpy directly (it's in the global namespace via <cstring>)
                        memcpy(&ram[dest_addr], &ram[src_addr], len);
                        break;
                    }
                    case FILL: { // FILL dest_addr_reg value_reg len_reg
                        BytecodeOperand op_dest = nextRawOperand();
                        BytecodeOperand op_val = nextRawOperand();
                        BytecodeOperand op_len = nextRawOperand();
                        int dest_addr = getValue(op_dest);
                        int value = getValue(op_val) & 0xFF; // Use lower byte of value register
                        int len = getValue(op_len);
                         if (len < 0) throw std::runtime_error("FILL length cannot be negative");
                        if (dest_addr < 0 || dest_addr + len > ram.size()) {
                             throw std::runtime_error("FILL memory access out of bounds");
                        }
                        // Use memset directly
                        memset(&ram[dest_addr], value, len);
                        break;
                    }
                    case CMP_MEM: { // CMP_MEM addr1_reg addr2_reg len_reg
                        BytecodeOperand op_addr1 = nextRawOperand();
                        BytecodeOperand op_addr2 = nextRawOperand();
                        BytecodeOperand op_len = nextRawOperand();
                        int addr1 = getValue(op_addr1);
                        int addr2 = getValue(op_addr2);
                        int len = getValue(op_len);
                        if (len < 0) throw std::runtime_error("CMP_MEM length cannot be negative");
                         if (addr1 < 0 || addr1 + len > ram.size() || addr2 < 0 || addr2 + len > ram.size()) {
                             throw std::runtime_error("CMP_MEM memory access out of bounds");
                        }
                        // Use memcmp directly
                        int result = memcmp(&ram[addr1], &ram[addr2], len);
                        zeroFlag = (result == 0);
                        signFlag = (result < 0);
                        break;
                    }


                    default:
                        throw std::runtime_error("Unimplemented or unknown opcode encountered during execution: 0x" + std::to_string(opcode));
                }
            } catch (const std::exception& e) {
                 std::cerr << "Runtime Error at bytecode offset " << currentIp << " (Opcode: 0x" << std::hex << static_cast<int>(opcode) << std::dec << "): " << e.what() << std::endl;
                 // Optionally dump registers or stack here for debugging
                 throw; // Re-throw after logging context
            }
        }
    }
};

int microasm_interpreter_main(int argc, char* argv[]) {
    // Expects argv[0] = bytecode file, argv[1...] = program args
    if (argc < 1) { // Need at least the bytecode file name (argc >= 1)
        // Updated usage message
        std::cerr << "Interpreter Usage: <bytecode.bin> [args...]" << std::endl;
        return 1;
    }

    std::string bytecodeFile = argv[0]; // Bytecode file is now argv[0]
    std::vector<std::string> programArgs;
    // Program arguments start from argv[1]
    for (int i = 1; i < argc; ++i) {
        programArgs.push_back(argv[i]);
    }


    try {
        Interpreter interpreter(65536, programArgs); // 64KB RAM, pass args
        interpreter.load(bytecodeFile);
        interpreter.execute();

        // std::cout << "Execution finished successfully!" << std::endl; // HLT provides its own message
    } catch (const std::exception& e) {
        // Error already logged in execute() or load()
        // std::cerr << "Execution failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
