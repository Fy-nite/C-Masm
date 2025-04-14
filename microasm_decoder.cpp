#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <iomanip> // For std::hex, std::setw, std::setfill
#include <unordered_map>
#include "operand_types.h" // Include the shared operand type definitions
#include <sstream>
#include <map>
#include <functional> // Required for std::function
#include <set>     // For storing referenced data offsets
#include <cctype>  // For isprint

// Placeholder for the machine state (registers, memory, etc.)
// In a real implementation, this would be a class or struct
struct RegisterMachineState {
    std::map<std::string, int> registers; // Example register map
    std::vector<unsigned char> memory;    // Example memory

};

// Define the type for MNI functions
// It takes the machine state and a vector of string arguments (registers/immediates)
using MniFunctionType = std::function<void(RegisterMachineState&, const std::vector<std::string>&)>;

// Global registry for MNI functions
std::map<std::string, MniFunctionType> mniRegistry;

// Function to register MNI functions
void registerMNI(const std::string& module, const std::string& name, MniFunctionType func) {
    std::string fullName = module + "." + name;
    if (mniRegistry.find(fullName) == mniRegistry.end()) {
        mniRegistry[fullName] = func;
     
        std::cout << "Registered MNI function: " << fullName << std::endl;
    } else {

        std::cerr << "Warning: MNI function " << fullName << " already registered." << std::endl;
    }
}

// Example MNI function implementation (replace with actual logic)
void exampleMathSin(RegisterMachineState& machine, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        std::cerr << "Error: Math.sin requires 2 arguments (src, dest)." << std::endl;
        return;
    }
    // Basic example: assumes args are register names
    // Real implementation needs robust parsing (registers, immediates, memory addresses)
    // and type checking/conversion.
    // machine.registers[args[1]] = static_cast<int>(sin(machine.registers[args[0]]));
    std::cout << "Called Math.sin with args: " << args[0] << ", " << args[1] << std::endl;
}

// Function to initialize MNI functions (call this during startup)
void initializeMNIFunctions() {
    registerMNI("Math", "sin", exampleMathSin);
    // Register other MNI functions here...
    // registerMNI("Memory", "allocate", ...);
    // registerMNI("IO", "write", ...);
}

// Define the same binary header structure as the compiler/interpreter
struct BinaryHeader {
    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t reserved = 0;
    uint32_t codeSize = 0;
    uint32_t dataSize = 0;
    uint32_t entryPoint = 0;
};

// Use the same Opcode enum as the compiler/interpreter - Added MNI
enum Opcode {
    // Basic
    MOV = 0x01, ADD, SUB, MUL, DIV, INC,
    // Flow Control
    JMP, CMP, JE, JL, CALL, RET,
    // Stack
    PUSH, POP,
    // I/O
    OUT, COUT, OUTSTR, OUTCHAR,
    // Program Control
    HLT, ARGC, GETARG,
    // Data Definition (Not present in code segment)
    DB,
    // Labels (Not present in code segment)
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
    COPY, FILL, CMP_MEM,
    MNI // Added MNI Opcode
};

// Helper map to get mnemonic string from Opcode - Added MNI
const std::unordered_map<Opcode, std::string> opcodeToString = {
    {MOV, "MOV"}, {ADD, "ADD"}, {SUB, "SUB"}, {MUL, "MUL"}, {DIV, "DIV"}, {INC, "INC"},
    {JMP, "JMP"}, {CMP, "CMP"}, {JE, "JE"}, {JL, "JL"}, {CALL, "CALL"}, {RET, "RET"},
    {PUSH, "PUSH"}, {POP, "POP"},
    {OUT, "OUT"}, {COUT, "COUT"}, {OUTSTR, "OUTSTR"}, {OUTCHAR, "OUTCHAR"},
    {HLT, "HLT"}, {ARGC, "ARGC"}, {GETARG, "GETARG"},
    {AND, "AND"}, {OR, "OR"}, {XOR, "XOR"}, {NOT, "NOT"}, {SHL, "SHL"}, {SHR, "SHR"},
    {MOVADDR, "MOVADDR"}, {MOVTO, "MOVTO"},
    {JNE, "JNE"}, {JG, "JG"}, {JLE, "JLE"}, {JGE, "JGE"},
    {ENTER, "ENTER"}, {LEAVE, "LEAVE"},
    {COPY, "COPY"}, {FILL, "FILL"}, {CMP_MEM, "CMP_MEM"},
    {MNI, "MNI"} // Added MNI mapping
};

// Helper map to get the number of operands for each Opcode - MNI needs special handling
const std::unordered_map<Opcode, int> opcodeOperandCount = {
    {MOV, 2}, {ADD, 2}, {SUB, 2}, {MUL, 2}, {DIV, 2}, {INC, 1},
    {JMP, 1}, {CMP, 2}, {JE, 1}, {JL, 1}, {CALL, 1}, {RET, 0},
    {PUSH, 1}, {POP, 1},
    {OUT, 2}, {COUT, 2}, {OUTSTR, 3}, {OUTCHAR, 2},
    {HLT, 0}, {ARGC, 1}, {GETARG, 2},
    {AND, 2}, {OR, 2}, {XOR, 2}, {NOT, 1}, {SHL, 2}, {SHR, 2},
    {MOVADDR, 3}, {MOVTO, 3},
    {JNE, 1}, {JG, 1}, {JLE, 1}, {JGE, 1},
    {ENTER, 1}, {LEAVE, 0},
    {COPY, 3}, {FILL, 3}, {CMP_MEM, 3},
    {MNI, -1} // Indicate variable/special handling for MNI
};

// Helper map for register names (index to name)
const std::unordered_map<int, std::string> registerIndexToString = {
    {0, "RAX"}, {1, "RBX"}, {2, "RCX"}, {3, "RDX"},
    {4, "RSI"}, {5, "RDI"}, {6, "RBP"}, {7, "RSP"},
    {8, "R0"}, {9, "R1"}, {10, "R2"}, {11, "R3"},
    {12, "R4"}, {13, "R5"}, {14, "R6"}, {15, "R7"},
    {16, "R8"}, {17, "R9"}, {18, "R10"}, {19, "R11"},
    {20, "R12"}, {21, "R13"}, {22, "R14"}, {23, "R15"}
};

// Helper function to read a null-terminated string from a byte vector at a given offset
std::string readStringFromVector(const std::vector<uint8_t>& vec, uint32_t& offset) {
    std::string str = "";
    while (offset < vec.size()) {
        char c = static_cast<char>(vec[offset++]);
        if (c == '\0') {
            break;
        }
        str += c;
    }
    // Optional: Check if null terminator was found before end of vector
    return str;
}

std::string formatOperand(OperandType type, int value) {
    switch (type) {
        case OperandType::REGISTER:
            if (registerIndexToString.count(value)) {
                return registerIndexToString.at(value);
            } else {
                return "R?" + std::to_string(value);
            }
        case OperandType::IMMEDIATE:
            return std::to_string(value);
        case OperandType::LABEL_ADDRESS:
            // In bytecode, it's just an address offset
            return "#" + std::to_string(value); // Represent as #offset
        case OperandType::DATA_ADDRESS:
            // In bytecode, it's an offset within the data segment
            return "$" + std::to_string(value); // Represent as $offset
        case OperandType::NONE:
            return "[NONE]";
        default:
            return "[UNKNOWN_TYPE]";
    }
}

// Helper function to format a sequence of bytes as hex
std::string formatBytesToHex(const std::vector<uint8_t>& vec, uint32_t start, uint32_t end) {
    std::stringstream ss;
    for (uint32_t i = start; i < end && i < vec.size(); ++i) {
        ss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(vec[i]) << " ";
    }
    return ss.str();
}

// Helper function to escape non-printable characters for string output
std::string escapeString(const std::string& s) {
    std::stringstream ss;
    for (char c : s) {
        if (c == '\n') {
            ss << "\\n";
        } else if (c == '\t') {
            ss << "\\t";
        } else if (c == '\\') {
            ss << "\\\\";
        } else if (c == '"') {
            ss << "\\\"";
        } else if (std::isprint(static_cast<unsigned char>(c))) {
            ss << c;
        } else {
            // Output non-printable chars as hex escapes
            ss << "\\x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return ss.str();
}

// Renamed decoder_main back to main
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: microasm_decoder <input.bin>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::ifstream in(inputFile, std::ios::binary);
    if (!in) {
        std::cerr << "Error: Cannot open input file: " << inputFile << std::endl;
        return 1;
    }

    try {
        // 1. Read and print header
        BinaryHeader header;
        if (!in.read(reinterpret_cast<char*>(&header), sizeof(header))) {
            throw std::runtime_error("Failed to read header.");
        }

        std::cout << "--- Header ---" << std::endl;
        std::cout << "Magic:      0x" << std::hex << header.magic << std::dec
                  << " ('" << static_cast<char>(header.magic & 0xFF)
                  << static_cast<char>((header.magic >> 8) & 0xFF)
                  << static_cast<char>((header.magic >> 16) & 0xFF)
                  << static_cast<char>((header.magic >> 24) & 0xFF) << "')" << std::endl;
        std::cout << "Version:    " << header.version << std::endl;
        std::cout << "Code Size:  " << header.codeSize << " bytes" << std::endl;
        std::cout << "Data Size:  " << header.dataSize << " bytes" << std::endl;
        std::cout << "Entry Point:" << header.entryPoint << " (offset)" << std::endl;
        std::cout << "--------------" << std::endl << std::endl;

        // Set to store data segment offsets referenced by the code
        std::set<uint32_t> referencedDataOffsets;

        // 2. Decode Code Segment
        std::cout << "--- Code Segment (Size: " << header.codeSize << ") ---" << std::endl;
        std::cout << "Offset  | Bytes        | Disassembly" << std::endl;
        std::cout << "--------|--------------|--------------------------------" << std::endl;
        std::vector<uint8_t> codeSegment(header.codeSize);
        if (header.codeSize > 0) {
            if (!in.read(reinterpret_cast<char*>(codeSegment.data()), header.codeSize)) {
                throw std::runtime_error("Failed to read code segment.");
            }
        }

        uint32_t ip = 0;
        while (ip < header.codeSize) {
            uint32_t instructionStartIp = ip;
            Opcode opcode = static_cast<Opcode>(codeSegment[ip++]);

            // Store operands temporarily to calculate instruction size *before* printing hex bytes
            std::vector<std::pair<OperandType, int>> currentOperands;
            std::string mniFunctionName = ""; // For MNI
            uint32_t tempIp = ip; // Use a temporary IP to parse operands without advancing the main ip

            try { // Inner try-catch for operand parsing to determine instruction size
                if (opcode == MNI) {
                    mniFunctionName = readStringFromVector(codeSegment, tempIp);
                    while (tempIp < header.codeSize) {
                        if (tempIp + 1 + sizeof(int) > header.codeSize) {
                             throw std::runtime_error("Unexpected end of code segment while reading MNI operand/marker");
                        }
                        OperandType opType = static_cast<OperandType>(codeSegment[tempIp]);
                        int opValue = *reinterpret_cast<const int*>(&codeSegment[tempIp + 1]);

                        if (opType == OperandType::NONE) {
                            tempIp += 1 + sizeof(int); // Consume marker
                            break; // End of MNI arguments
                        }
                        currentOperands.push_back({opType, opValue});
                        tempIp += 1 + sizeof(int); // Consume operand
                        if (opType == OperandType::DATA_ADDRESS) {
                            referencedDataOffsets.insert(static_cast<uint32_t>(opValue));
                        }
                    }
                } else if (opcodeOperandCount.count(opcode)) {
                    int numOperands = opcodeOperandCount.at(opcode);
                    for (int i = 0; i < numOperands; ++i) {
                        if (tempIp + 1 + sizeof(int) > header.codeSize) {
                            throw std::runtime_error("Unexpected end of code segment while reading operand " + std::to_string(i+1));
                        }
                        OperandType opType = static_cast<OperandType>(codeSegment[tempIp]);
                        int opValue = *reinterpret_cast<const int*>(&codeSegment[tempIp + 1]);
                        currentOperands.push_back({opType, opValue});
                        tempIp += 1 + sizeof(int); // Consume operand
                        if (opType == OperandType::DATA_ADDRESS) {
                            referencedDataOffsets.insert(static_cast<uint32_t>(opValue));
                        }
                    }
                } else if (!opcodeToString.count(opcode)) {
                     throw std::runtime_error("Unknown opcode encountered."); // Handle unknown opcode early
                }
                // If opcode is known but count is missing (shouldn't happen with current setup)
                // else { throw std::runtime_error("Missing operand count for known opcode."); }

            } catch (const std::exception& operandEx) {
                 // Can't determine full instruction size if operands are bad
                 std::cerr << "Error parsing operands at offset " << instructionStartIp << ": " << operandEx.what() << std::endl;
                 tempIp = ip; // Reset tempIp, print only opcode byte maybe?
                 // Or rethrow / break loop
                 throw;
            }

            uint32_t instructionEndIp = tempIp; // End of the current instruction bytes

            // Print Offset
            std::cout << std::setw(7) << std::setfill('0') << instructionStartIp << " | ";
            // Print Hex Bytes
            std::string hexBytes = formatBytesToHex(codeSegment, instructionStartIp, instructionEndIp);
            std::cout << std::setw(12) << std::left << hexBytes << " | ";
            std::cout << std::setfill(' '); // Reset fill character

            // Print Disassembly
            if (opcode == MNI) {
                std::cout << std::setw(8) << std::left << "MNI" << std::right << " " << mniFunctionName;
                for (const auto& opPair : currentOperands) {
                    std::cout << " " << formatOperand(opPair.first, opPair.second);
                }
                std::cout << std::endl;
            } else if (opcodeToString.count(opcode)) {
                std::cout << std::setw(8) << std::left << opcodeToString.at(opcode) << std::right;
                for (const auto& opPair : currentOperands) {
                    std::cout << " " << formatOperand(opPair.first, opPair.second);
                }
                 std::cout << std::endl;
            } else {
                 // Should have been caught earlier, but as a fallback
                 std::cout << "Unknown Opcode (0x" << std::hex << static_cast<int>(opcode) << std::dec << ")" << std::endl;
            }

            ip = instructionEndIp; // Advance main IP to the end of the processed instruction

        }
        std::cout << "--------|--------------|--------------------------------" << std::endl << std::endl;


        // 3. Decode Data Segment with DB Reconstruction
        std::cout << "--- Data Segment (Size: " << header.dataSize << ") ---" << std::endl;
        std::vector<char> dataSegment(header.dataSize);
        std::vector<bool> dataProcessed(header.dataSize, false); // Track processed bytes

         if (header.dataSize > 0) {
            // Read directly from the input file stream 'in'
            if (!in.read(dataSegment.data(), header.dataSize)) {
                 throw std::runtime_error("Failed to read data segment.");
            }

            for (uint32_t i = 0; i < header.dataSize; /* increment handled inside */ ) {
                if (dataProcessed[i]) {
                    i++;
                    continue;
                }

                bool isReferenced = referencedDataOffsets.count(i);
                std::string currentString;
                bool isString = false;
                uint32_t stringEnd = i;

                // If referenced, try to interpret as a null-terminated string
                if (isReferenced) {
                    uint32_t k = i;
                    bool possibleString = true;
                    while (k < header.dataSize) {
                        char c = dataSegment[k];
                        if (c == '\0') { // Found null terminator
                            stringEnd = k;
                            isString = possibleString;
                            break;
                        }
                        // Allow printable ASCII, newline, tab
                        if (!std::isprint(static_cast<unsigned char>(c)) && c != '\n' && c != '\t') {
                            possibleString = false;
                            // Don't break immediately, find the null terminator anyway if it exists soon
                        }
                        currentString += c;
                        k++;
                    }
                     // If loop finished without null terminator, it's not a string ending here
                    if (k == header.dataSize && dataSegment[k-1] != '\0') {
                        isString = false;
                    }
                }

                // Output based on findings
                if (isString) {
                    std::cout << "; Referenced Data (String)" << std::endl;
                    std::cout << "DB $" << i << " \"" << escapeString(currentString) << "\"" << std::endl;
                    // Mark bytes as processed
                    for (uint32_t p = i; p <= stringEnd; ++p) {
                        dataProcessed[p] = true;
                    }
                    i = stringEnd + 1; // Move past the null terminator
                } else {
                    // Output single byte as DB 0xHH
                    if (isReferenced) {
                         std::cout << "; Referenced Data (Byte)" << std::endl;
                    } else {
                         std::cout << "; Unreferenced Data (Byte)" << std::endl;
                    }
                    std::cout << "DB $" << i << " 0x"
                              << std::setw(2) << std::setfill('0') << std::hex
                              << static_cast<int>(static_cast<unsigned char>(dataSegment[i]))
                              << std::dec << std::endl;
                    dataProcessed[i] = true;
                    i++; // Move to the next byte
                }
            }
        } else {
             std::cout << "(Empty)" << std::endl;
        }
        std::cout << "--------------" << std::endl;


    } catch (const std::exception& e) {
        std::cerr << "Decoding Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
