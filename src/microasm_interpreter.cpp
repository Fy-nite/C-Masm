#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>
#include <limits>
#include <cstring>
#include <cstdint>
#include <map>
#include <functional>
#include <iomanip>
#include <sstream> // Add this header for std::stringstream

// Include own header FIRST
#include "microasm_interpreter.h"

// Global registry for MNI functions (define only in ONE .cpp file)
std::map<std::string, MniFunctionType> mniRegistry;

// Define the MNI registration function
void registerMNI(const std::string& module, const std::string& name, MniFunctionType func) {
    std::string fullName = module + "." + name;
    if (mniRegistry.find(fullName) == mniRegistry.end()) {
        mniRegistry[fullName] = func;
       // Optional: Keep debug print here if desired
       // std::cout << "Registered MNI function: " << fullName << std::endl;
    } else {
       std::cerr << "Warning: MNI function " << fullName << " already registered." << std::endl;
    }
}

// --- Interpreter Method Definitions ---

Interpreter::Interpreter(int ramSize, const std::vector<std::string>& args, bool debug)
    : registers(24, 0),
      ram(ramSize, 0),
      cmdArgs(args),
      debugMode(debug)
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

    // Initialize MNI functions
    initializeMNIFunctions();

    if (debugMode) std::cout << "[Debug][Interpreter] Debug mode enabled. RAM Size: " << ramSize << "\n";
}

BytecodeOperand Interpreter::nextRawOperand() {
    if (ip + 1 + sizeof(int) > bytecode_raw.size()) { // Check size for type byte + value int
         throw std::runtime_error("Unexpected end of bytecode reading typed operand (IP: " + std::to_string(ip) + ", CodeSize: " + std::to_string(bytecode_raw.size()) + ")");
    }
    BytecodeOperand operand;
    operand.type = static_cast<OperandType>(bytecode_raw[ip++]);
    // Handle NONE type immediately if needed (though it shouldn't be read here usually)
    if (operand.type == OperandType::NONE) {
         operand.value = 0; // No value associated
         // ip adjustment might depend on how NONE is encoded (e.g., if it has a dummy value)
         // Assuming it's just the type byte based on compiler code for MNI marker
    } else {
         if (ip + sizeof(int) > bytecode_raw.size()) {
             throw std::runtime_error("Unexpected end of bytecode reading operand value (IP: " + std::to_string(ip) + ")");
         }
         operand.value = *reinterpret_cast<const int*>(&bytecode_raw[ip]);
         ip += sizeof(int);
    }
    return operand;
}

int Interpreter::getValue(const BytecodeOperand& operand) {
    switch (operand.type) {
        case OperandType::REGISTER:
        case OperandType::REGISTER_AS_ADDRESS: // For getValue, treat same as REGISTER (return content)
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

int Interpreter::getRegisterIndex(const BytecodeOperand& operand) {
    if (operand.type != OperandType::REGISTER) {
        throw std::runtime_error("Expected register operand, got type " + std::to_string(static_cast<int>(operand.type)));
    }
    if (operand.value < 0 || operand.value >= registers.size()) {
        throw std::runtime_error("Invalid register index encountered: " + std::to_string(operand.value));
    }
    return operand.value;
}

int Interpreter::readRamInt(int address) {
    if (address < 0 || address + sizeof(int) > ram.size()) {
        throw std::runtime_error("Memory read out of bounds at address: " + std::to_string(address));
    }
    return *reinterpret_cast<int*>(&ram[address]);
}

void Interpreter::writeRamInt(int address, int value) {
    if (address < 0 || address + sizeof(int) > ram.size()) {
        throw std::runtime_error("Memory write out of bounds at address: " + std::to_string(address));
    }
    *reinterpret_cast<int*>(&ram[address]) = value;
}

char Interpreter::readRamChar(int address) {
    if (address < 0 || address >= ram.size()) {
        throw std::runtime_error("Memory read out of bounds at address: " + std::to_string(address));
    }
    return ram[address];
}

void Interpreter::writeRamChar(int address, char value) {
    if (address < 0 || address >= ram.size()) {
        throw std::runtime_error("Memory write out of bounds at address: " + std::to_string(address));
    }
    ram[address] = value;
}

std::string Interpreter::readRamString(int address) {
    std::string str = "";
    int currentAddr = address;
    while(true) {
        char c = readRamChar(currentAddr++);
        if (c == '\0') break;
        str += c;
    }
    return str;
}

void Interpreter::pushStack(int value) {
    registers[7] -= sizeof(int); // Decrement RSP (stack grows down)
    sp = registers[7];
    writeRamInt(sp, value);
}

int Interpreter::popStack() {
    int value = readRamInt(sp);
    registers[7] += sizeof(int); // Increment RSP
    sp = registers[7];
    return value;
}

std::string Interpreter::readBytecodeString() {
    std::string str = "";
    while (ip < bytecode_raw.size()) {
        char c = static_cast<char>(bytecode_raw[ip++]);
        if (c == '\0') {
            break;
        }
        str += c;
    }
    // Consider adding a check if ip reached end without null terminator
    return str;
}

void Interpreter::initializeMNIFunctions() {
    // Example: Math.sin R1 R2 (R1=input reg, R2=output reg)
    registerMNI("Math", "sin", [](Interpreter& machine, const std::vector<BytecodeOperand>& args) {
        if (args.size() != 2) throw std::runtime_error("Math.sin requires 2 arguments (srcReg, destReg)");
        int srcReg = machine.getRegisterIndex(args[0]);
        int destReg = machine.getRegisterIndex(args[1]);
        // Note: Using double for calculation, storing as int
        machine.registers[destReg] = static_cast<int>(std::sin(static_cast<double>(machine.registers[srcReg])));
    });

    // Example: IO.write 1 R1 (Port=1/2, R1=address of null-terminated string in RAM)
    registerMNI("IO", "write", [](Interpreter& machine, const std::vector<BytecodeOperand>& args) {
        if (args.size() != 2) throw std::runtime_error("IO.write requires 2 arguments (port, addressReg/Imm)");
        int port = machine.getValue(args[0]); // Port can be immediate or register
        int address = machine.getValue(args[1]); // Address MUST resolve to a RAM location
        if (args[1].type != OperandType::REGISTER && args[1].type != OperandType::DATA_ADDRESS && args[1].type != OperandType::IMMEDIATE) {
             throw std::runtime_error("IO.write address argument must be register or data address");
        }

        std::ostream& out_stream = (port == 2) ? std::cerr : std::cout;
         if (port != 1 && port != 2) throw std::runtime_error("Invalid port for IO.write: " + std::to_string(port));

        out_stream << machine.readRamString(address); // Read null-terminated string from RAM
    });


    // Memory.allocate R1 R2 (R1=size, R2=destReg for address) - Needs heap management!
    // StringOperations.concat R1 R2 R3 (R1=addr1, R2=addr2, R3=destAddr) - Needs memory allocation!
}

std::string Interpreter::formatOperandDebug(const BytecodeOperand& op) {
    std::stringstream ss;
    ss << "T:0x" << std::hex << static_cast<int>(op.type) << std::dec;
    ss << " V:" << op.value;
    switch(op.type) {
        case OperandType::REGISTER: ss << "(R" << op.value << ")"; break;
        case OperandType::REGISTER_AS_ADDRESS: ss << "($R" << op.value << ")"; break;
        case OperandType::IMMEDIATE: ss << "(Imm)"; break;
        case OperandType::LABEL_ADDRESS: ss << "(LblAddr)"; break;
        case OperandType::DATA_ADDRESS: ss << "(DataAddr 0x" << std::hex << (dataSegmentBase + op.value) << std::dec << ")"; break;
        default: ss << "(?)"; break;
    }
    return ss.str();
}

void Interpreter::load(const std::string& bytecodeFile) {
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
    if (header.version != 1) {
        throw std::runtime_error("Unsupported bytecode version: " + std::to_string(header.version) + " (Supported version: 1)");
    }

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

    if (debugMode) {
         std::cout << "[Debug][Interpreter] Loading bytecode from: " << bytecodeFile << "\n";
         std::cout << "[Debug][Interpreter]   Header - Magic: 0x" << std::hex << header.magic
                   << ", Version: " << std::dec << header.version
                   << ", CodeSize: " << header.codeSize
                   << ", DataSize: " << header.dataSize
                   << ", EntryPoint: 0x" << std::hex << header.entryPoint << std::dec << "\n";
         std::cout << "[Debug][Interpreter]   Data Segment loaded at RAM base: 0x" << std::hex << dataSegmentBase << std::dec << "\n";
         std::cout << "[Debug][Interpreter]   IP set to entry point: 0x" << std::hex << ip << std::dec << "\n";
    }
}

void Interpreter::setArguments(const std::vector<std::string>& args) {
    cmdArgs = args;
    if (debugMode) {
        std::cout << "[Debug][Interpreter] Arguments set/updated. Count: " << cmdArgs.size() << "\n";
    }
}

void Interpreter::setDebugMode(bool enabled) {
    debugMode = enabled;
     if (debugMode) {
        std::cout << "[Debug][Interpreter] Debug mode explicitly set to: " << (enabled ? "ON" : "OFF") << "\n";
    }
}

void Interpreter::execute() {
    if (debugMode) std::cout << "[Debug][Interpreter] Starting execution...\n";
    // Reset flags before execution? Or assume they persist? Assume reset for now.
    zeroFlag = false;
    signFlag = false;
    // IP should be set by load() or jump instructions

    // Ensure SP and BP reflect register values at start
    sp = registers[7];
    bp = registers[6];

    while (ip < bytecode_raw.size()) {
        int currentIp = ip;
        if (debugMode) std::cout << "[Debug][Interpreter] IP: 0x" << std::hex << std::setw(4) << std::setfill('0') << currentIp << std::dec << ": ";

        Opcode opcode = static_cast<Opcode>(bytecode_raw[ip++]);

        if (debugMode) {
             std::cout << "Opcode 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(opcode) << std::dec;
             // You might want a helper function to convert Opcode enum to string here
             std::cout << " (";
             switch(opcode) { // Basic opcode names for debug
                case MOV: std::cout << "MOV"; break; case ADD: std::cout << "ADD"; break;
                case SUB: std::cout << "SUB"; break; case MUL: std::cout << "MUL"; break;
                case DIV: std::cout << "DIV"; break; case INC: std::cout << "INC"; break;
                case JMP: std::cout << "JMP"; break; case CMP: std::cout << "CMP"; break;
                case JE: std::cout << "JE"; break; case JL: std::cout << "JL"; break;
                case CALL: std::cout << "CALL"; break; case RET: std::cout << "RET"; break;
                case PUSH: std::cout << "PUSH"; break; case POP: std::cout << "POP"; break;
                case OUT: std::cout << "OUT"; break; case COUT: std::cout << "COUT"; break;
                case OUTSTR: std::cout << "OUTSTR"; break; case OUTCHAR: std::cout << "OUTCHAR"; break;
                case HLT: std::cout << "HLT"; break; case ARGC: std::cout << "ARGC"; break;
                case GETARG: std::cout << "GETARG"; break; case DB: std::cout << "DB"; break;
                case LBL: std::cout << "LBL"; break; case AND: std::cout << "AND"; break;
                case OR: std::cout << "OR"; break; case XOR: std::cout << "XOR"; break;
                case NOT: std::cout << "NOT"; break; case SHL: std::cout << "SHL"; break;
                case SHR: std::cout << "SHR"; break; case MOVADDR: std::cout << "MOVADDR"; break;
                case MOVTO: std::cout << "MOVTO"; break; case JNE: std::cout << "JNE"; break;
                case JG: std::cout << "JG"; break; case JLE: std::cout << "JLE"; break;
                case JGE: std::cout << "JGE"; break; case ENTER: std::cout << "ENTER"; break;
                case LEAVE: std::cout << "LEAVE"; break; case COPY: std::cout << "COPY"; break;
                case FILL: std::cout << "FILL"; break; case CMP_MEM: std::cout << "CMP_MEM"; break;
                case MNI: std::cout << "MNI"; break;
                case IN: std::cout << "IN"; break;
                default: std::cout << "???"; break;
             }
             std::cout << ")\n";
        }

        // Store pre-execution register state for comparison if needed
        std::vector<int> regsBefore;
        if (debugMode) regsBefore = registers;


        try {
            switch (opcode) {
                // Basic Arithmetic
                case MOV: {
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_src = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Src ): " << formatOperandDebug(op_src) << "\n";
                    int dest_reg = getRegisterIndex(op_dest); // Destination must be register
                    registers[dest_reg] = getValue(op_src);   // Get value from source (reg or imm)
                    break;
                }
                case ADD: {
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_src = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Src ): " << formatOperandDebug(op_src) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    registers[dest_reg] += getValue(op_src); // Add value (reg or imm)
                    break;
                }
                case SUB: {
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_src = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Src ): " << formatOperandDebug(op_src) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    registers[dest_reg] -= getValue(op_src);
                    break;
                }
                case MUL: {
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_src = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Src ): " << formatOperandDebug(op_src) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    registers[dest_reg] *= getValue(op_src);
                    break;
                }
                case DIV: {
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_src = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Src ): " << formatOperandDebug(op_src) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    int src_val = getValue(op_src);
                    if (src_val == 0) throw std::runtime_error("Division by zero");
                    registers[dest_reg] /= src_val;
                    break;
                }
                case INC: {
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    registers[dest_reg]++;
                    break;
                }

                // Flow Control
                case JMP: {
                    BytecodeOperand op_target = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Target): " << formatOperandDebug(op_target) << "\n";
                    // Target should be immediate label address from compiler
                    if (op_target.type != OperandType::LABEL_ADDRESS && op_target.type != OperandType::IMMEDIATE) {
                         throw std::runtime_error("JMP requires immediate/label address operand");
                    }
                    ip = op_target.value; // Jump to absolute address
                    if(debugMode) std::cout << "[Debug][Interpreter]     Jumping to 0x" << std::hex << ip << std::dec << "\n";
                    break;
                }
                case CMP: {
                    BytecodeOperand op1 = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1: " << formatOperandDebug(op1) << "\n";
                    BytecodeOperand op2 = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2: " << formatOperandDebug(op2) << "\n";
                    int val1 = getValue(op1);
                    int val2 = getValue(op2);
                    zeroFlag = (val1 == val2);
                    signFlag = (val1 < val2);
                     if(debugMode) std::cout << "[Debug][Interpreter]     Compare(" << val1 << ", " << val2 << ") -> ZF=" << zeroFlag << ", SF=" << signFlag << "\n";
                    break;
                }
                // Conditional Jumps (JE, JNE, JL, JG, JLE, JGE)
                case JE: case JNE: case JL: case JG: case JLE: case JGE: {
                    BytecodeOperand op_target = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Target): " << formatOperandDebug(op_target) << "\n";
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
                        if(debugMode) std::cout << "[Debug][Interpreter]     Condition met. Jumping to 0x" << std::hex << ip << std::dec << "\n";
                    } else {
                         if(debugMode) std::cout << "[Debug][Interpreter]     Condition not met. Continuing.\n";
                    }
                    break;
                }

                case CALL: {
                    BytecodeOperand op_target = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Target): " << formatOperandDebug(op_target) << "\n";
                     if (op_target.type != OperandType::LABEL_ADDRESS && op_target.type != OperandType::IMMEDIATE) {
                         throw std::runtime_error("CALL requires immediate/label address operand");
                    }
                    pushStack(ip); // Push return address (address AFTER call instruction + operands)
                    if(debugMode) std::cout << "[Debug][Interpreter]     Pushing return address 0x" << std::hex << ip << std::dec << ". Calling 0x" << std::hex << op_target.value << std::dec << "\n";
                    ip = op_target.value;   // Jump to function
                    break;
                }
                case RET: {
                    if (registers[7] >= ram.size()) throw std::runtime_error("Stack underflow on RET");
                    int retAddr = popStack();
                    if(debugMode) std::cout << "[Debug][Interpreter]     Popped return address 0x" << std::hex << retAddr << std::dec << ". Returning.\n";
                    ip = retAddr; // Pop return address and jump
                    break;
                }

                // Stack Operations
                case PUSH: {
                    BytecodeOperand op_src = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Src): " << formatOperandDebug(op_src) << "\n";
                    int val = getValue(op_src);
                    pushStack(val); // Push value (reg or imm)
                    if(debugMode) std::cout << "[Debug][Interpreter]     Pushed value " << val << ". New SP: 0x" << std::hex << sp << std::dec << "\n";
                    break;
                }
                case POP: {
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    int val = popStack();
                    registers[dest_reg] = val;
                    if(debugMode) std::cout << "[Debug][Interpreter]     Popped value " << val << " into R" << dest_reg << ". New SP: 0x" << std::hex << sp << std::dec << "\n";
                    break;
                }
            
                // I/O Operations
                case OUT: {
                    BytecodeOperand op_port = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Port): " << formatOperandDebug(op_port) << "\n";
                    BytecodeOperand op_val = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Val ): " << formatOperandDebug(op_val) << "\n";
                    int port = getValue(op_port);
                    std::ostream& out_stream = (port == 2) ? std::cerr : std::cout;
                    if (port != 1 && port != 2) throw std::runtime_error("Invalid port for OUT: " + std::to_string(port));
                    switch (op_val.type) {
                        case OperandType::DATA_ADDRESS: {
                            // Get the absolute RAM address (Base + Offset)
                            int address = dataSegmentBase + op_val.value;
                            if (address < 0 || address >= ram.size()) {
                                 throw std::runtime_error("OUT: Data address out of RAM bounds: Base=" + std::to_string(dataSegmentBase) + ", Offset=" + std::to_string(op_val.value));
                            }
                            out_stream << readRamString(address); // Read and print the string from RAM
                            break;
                        }
                        case OperandType::REGISTER_AS_ADDRESS: {
                            int reg_index = op_val.value;
                            if (reg_index < 0 || reg_index >= registers.size()) {
                                throw std::runtime_error("OUT: Invalid register index for REGISTER_AS_ADDRESS: " + std::to_string(reg_index));
                            }
                            int address = registers[reg_index]; // Get address from register
                             if (address < 0 || address >= ram.size()) {
                                 throw std::runtime_error("OUT: Address in register R" + std::to_string(reg_index) + " (" + std::to_string(address) + ") is out of RAM bounds");
                             }
                            out_stream << readRamString(address); // Read and print the string from RAM
                            break;
                        }
                        case OperandType::REGISTER: {
                            int reg_val = registers[getRegisterIndex(op_val)];
                            out_stream << reg_val; // Print the integer value in the register
                            break;
                        }
                        case OperandType::IMMEDIATE: {
                            out_stream << op_val.value; // Print the immediate integer value
                            break;
                        }
                        default:
                            throw std::runtime_error("Unsupported operand type for OUT value: " + std::to_string(static_cast<int>(op_val.type)));
                    }
                    break;
                }
                 case COUT: {
                    BytecodeOperand op_port = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Port): " << formatOperandDebug(op_port) << "\n";
                    BytecodeOperand op_val = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Val ): " << formatOperandDebug(op_val) << "\n";
                    int port = getValue(op_port);
                    std::ostream& out_stream = (port == 2) ? std::cerr : std::cout;
                    if (port != 1 && port != 2) throw std::runtime_error("Invalid port for COUT: " + std::to_string(port));
                    out_stream << static_cast<char>(getValue(op_val)); // Output char value
                    break;
                }
                case OUTSTR: { // Prints string from RAM address IN REGISTER
                    BytecodeOperand op_port = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Port): " << formatOperandDebug(op_port) << "\n";
                    BytecodeOperand op_addr = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Addr): " << formatOperandDebug(op_addr) << "\n";
                    BytecodeOperand op_len = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op3(Len ): " << formatOperandDebug(op_len) << "\n";
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
                    BytecodeOperand op_port = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Port): " << formatOperandDebug(op_port) << "\n";
                    BytecodeOperand op_addr = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Addr): " << formatOperandDebug(op_addr) << "\n";
                    int port = getValue(op_port);
                    std::ostream& out_stream = (port == 2) ? std::cerr : std::cout;
                    if (port != 1 && port != 2) throw std::runtime_error("Invalid port for OUTCHAR: " + std::to_string(port));

                    int addr = getValue(op_addr);
                    out_stream << readRamChar(addr); // Read single char
                    // No automatic newline for OUTCHAR
                    break;
                }
                case IN: {
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    // Destination can be REGISTER_AS_ADDRESS or DATA_ADDRESS
                    int destAddr;
                    if (op_dest.type == OperandType::REGISTER_AS_ADDRESS) {
                        int reg_index = op_dest.value;
                        if (reg_index < 0 || reg_index >= registers.size()) {
                            throw std::runtime_error("IN: Invalid register index for REGISTER_AS_ADDRESS: " + std::to_string(reg_index));
                        }
                        destAddr = registers[reg_index];
                    } else if (op_dest.type == OperandType::DATA_ADDRESS) {
                        destAddr = dataSegmentBase + op_dest.value;
                    } else {
                         throw std::runtime_error("IN requires a destination operand of type REGISTER_AS_ADDRESS ($R<n>) or DATA_ADDRESS ($<label>)");
                    }

                    if (destAddr < 0 || destAddr >= ram.size()) { // Basic check, might need more space
                         throw std::runtime_error("IN: Destination address " + std::to_string(destAddr) + " is out of RAM bounds");
                    }

                    std::string input;
                    std::getline(std::cin, input);

                    // Check if there's enough space in RAM
                    if (destAddr + input.size() + 1 > ram.size()) { // +1 for null terminator
                        throw std::runtime_error("IN: Not enough RAM space at address " + std::to_string(destAddr) + " for input string of size " + std::to_string(input.size()));
                    }

                    // Write input and null terminator to memory
                    std::copy(input.begin(), input.end(), ram.begin() + destAddr);
                    ram[destAddr + input.size()] = '\0';
                    break;
                }

                // Program Control
                case HLT: { if(debugMode) std::cout << "[Debug][Interpreter] HLT encountered.\n";  return; } // Halt execution
                case ARGC: {
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    registers[getRegisterIndex(op_dest)] = cmdArgs.size(); // Use cmdArgs member
                    break;
                }
                case GETARG: {
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_index = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Index): " << formatOperandDebug(op_index) << "\n";
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
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_src = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Src ): " << formatOperandDebug(op_src) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    registers[dest_reg] &= getValue(op_src);
                    break; 
                }
                case OR: { 
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_src = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Src ): " << formatOperandDebug(op_src) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    registers[dest_reg] |= getValue(op_src);
                    break; 
                }
                case XOR: { 
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_src = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Src ): " << formatOperandDebug(op_src) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    registers[dest_reg] ^= getValue(op_src);
                    break; 
                }
                case NOT: { 
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    registers[dest_reg] = ~registers[dest_reg];
                    break; 
                }
                case SHL: { 
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_count = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Count): " << formatOperandDebug(op_count) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    registers[dest_reg] <<= getValue(op_count);
                    break; 
                }
                case SHR: { 
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_count = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Count): " << formatOperandDebug(op_count) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    registers[dest_reg] >>= getValue(op_count);
                    break; 
                } // Arithmetic right shift

                // Memory Addressing
                case MOVADDR: { // MOVADDR dest_reg src_addr_reg offset_reg
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_src_addr = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(SrcAddr): " << formatOperandDebug(op_src_addr) << "\n";
                    BytecodeOperand op_offset = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op3(Offset): " << formatOperandDebug(op_offset) << "\n";
                    int dest_reg = getRegisterIndex(op_dest);
                    int address = getValue(op_src_addr) + getValue(op_offset);
                    registers[dest_reg] = readRamInt(address);
                    break;
                }
                case MOVTO: { // MOVTO dest_addr_reg offset_reg src_reg
                    BytecodeOperand op_dest_addr = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(DestAddr): " << formatOperandDebug(op_dest_addr) << "\n";
                    BytecodeOperand op_offset = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Offset): " << formatOperandDebug(op_offset) << "\n";
                    BytecodeOperand op_src = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op3(Src): " << formatOperandDebug(op_src) << "\n";
                    int address = getValue(op_dest_addr) + getValue(op_offset);
                    writeRamInt(address, getValue(op_src));
                    break;
                }

                // Stack Frame Management
                case ENTER: { // ENTER framesize (immediate)
                    BytecodeOperand op_frameSize = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(FrameSize): " << formatOperandDebug(op_frameSize) << "\n";
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
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_src = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Src ): " << formatOperandDebug(op_src) << "\n";
                    BytecodeOperand op_len = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op3(Len ): " << formatOperandDebug(op_len) << "\n";
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
                    BytecodeOperand op_dest = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Dest): " << formatOperandDebug(op_dest) << "\n";
                    BytecodeOperand op_val = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Val ): " << formatOperandDebug(op_val) << "\n";
                    BytecodeOperand op_len = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op3(Len ): " << formatOperandDebug(op_len) << "\n";
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
                    BytecodeOperand op_addr1 = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op1(Addr1): " << formatOperandDebug(op_addr1) << "\n";
                    BytecodeOperand op_addr2 = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op2(Addr2): " << formatOperandDebug(op_addr2) << "\n";
                    BytecodeOperand op_len = nextRawOperand(); if(debugMode) std::cout << "[Debug][Interpreter]   Op3(Len ): " << formatOperandDebug(op_len) << "\n";
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

                case MNI: {
                    std::string functionName = readBytecodeString(); // Read Module.Function\0
                    if(debugMode) std::cout << "[Debug][Interpreter]   MNI Func: " << functionName << "\n";
                    std::vector<BytecodeOperand> mniArgs;
                    while(true) {
                        BytecodeOperand arg = nextRawOperand();
                        if (arg.type == OperandType::NONE) {
                            if(debugMode) std::cout << "[Debug][Interpreter]     End MNI Args\n";
                            break; // End of arguments marker
                        }
                        if(debugMode) std::cout << "[Debug][Interpreter]     MNI Arg : " << formatOperandDebug(arg) << "\n";
                        mniArgs.push_back(arg);
                    }

                    if (mniRegistry.count(functionName)) {
                        mniRegistry[functionName](*this, mniArgs); // Call the registered function
                    } else {
                        throw std::runtime_error("Unregistered MNI function called: " + functionName);
                    }
                    break;
                }

                default:
                    throw std::runtime_error("Unimplemented or unknown opcode encountered during execution: 0x" + std::to_string(opcode));
            }

            // Optional: Print changed registers
            if (debugMode) {
                for(size_t i = 0; i < registers.size(); ++i) {
                    if (registers[i] != regsBefore[i]) {
                         std::cout << "[Debug][Interpreter]     Reg Change: R" << i << " = " << registers[i] << " (was " << regsBefore[i] << ")\n";
                    }
                }
                // Print flag changes if CMP or relevant instruction executed
                // (Requires tracking if flags were potentially modified by the opcode)
            }

        } catch (const std::exception& e) {
             std::cerr << "\nRuntime Error at bytecode offset 0x" << std::hex << currentIp << std::dec << " (Opcode: 0x" << std::hex << static_cast<int>(opcode) << std::dec << "): " << e.what() << std::endl;
             // Dump registers on error
             std::cerr << "Register dump:\n";
             for(size_t i = 0; i < registers.size(); ++i) {
                 std::cerr << "  R" << std::setw(2) << std::setfill(' ') << i << ": " << std::setw(10) << registers[i] << " (0x" << std::hex << std::setw(8) << std::setfill('0') << registers[i] << std::dec << ")\n";
             }
             std::cerr << "  ZF=" << zeroFlag << ", SF=" << signFlag << "\n";
             throw; // Re-throw after logging context
        }
    }
     if (debugMode) std::cout << "[Debug][Interpreter] Execution finished (reached end of bytecode).\n";
}

void Interpreter::executeStep() {
    // TODO: Implement single-instruction execution logic.
    // For now, just call execute() as a placeholder.
    execute();
}

// --- Standalone Interpreter Main Function Definition ---

int microasm_interpreter_main(int argc, char* argv[]) {
    // --- Argument Parsing for Standalone Interpreter ---
    std::string bytecodeFile;
    bool enableDebug = false;
    std::vector<std::string> programArgs; // Args for the interpreted program

    // argv[0] here is the *first argument* after "-i", not the program name
    for (int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--debug") {
            enableDebug = true;
        } else if (bytecodeFile.empty()) {
            bytecodeFile = arg;
        } else {
            // Remaining args are for the program being interpreted
            programArgs.push_back(arg);
        }
    }

     if (bytecodeFile.empty()) {
        std::cerr << "Interpreter Usage: <bytecode.bin> [args...] [-d|--debug]" << std::endl;
        return 1;
    }
    // --- End Argument Parsing ---

    try {
        Interpreter interpreter(65536, programArgs, enableDebug); // Pass debug flag
        interpreter.load(bytecodeFile);
        interpreter.execute();

        // std::cout << "Execution finished successfully!" << std::endl; // HLT provides its own message
    } catch (const std::exception& e) {
        // Error already logged in execute() or load()
        std::cerr << "Execution failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
