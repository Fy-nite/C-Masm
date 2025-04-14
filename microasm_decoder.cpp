#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <iomanip> // For std::hex, std::setw, std::setfill
#include <unordered_map>
#include "operand_types.h" // Include the shared operand type definitions

// Define the same binary header structure as the compiler/interpreter
struct BinaryHeader {
    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t reserved = 0;
    uint32_t codeSize = 0;
    uint32_t dataSize = 0;
    uint32_t entryPoint = 0;
};

// Use the same Opcode enum as the compiler/interpreter
// (Duplicated here for simplicity, could be shared in a header)
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
    COPY, FILL, CMP_MEM
};

// Helper map to get mnemonic string from Opcode
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
    {COPY, "COPY"}, {FILL, "FILL"}, {CMP_MEM, "CMP_MEM"}
};

// Helper map to get the number of operands for each Opcode
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
    {COPY, 3}, {FILL, 3}, {CMP_MEM, 3}
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

int decoder_main(int argc, char* argv[]) {
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

        // 2. Decode Code Segment
        std::cout << "--- Code Segment (Size: " << header.codeSize << ") ---" << std::endl;
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

            std::cout << std::setw(4) << std::setfill('0') << instructionStartIp << ": ";

            if (opcodeToString.count(opcode)) {
                std::cout << std::setw(8) << std::left << opcodeToString.at(opcode) << std::right;

                int numOperands = 0;
                if (opcodeOperandCount.count(opcode)) {
                    numOperands = opcodeOperandCount.at(opcode);
                } else {
                    std::cout << " !Unknown Operand Count!";
                }

                for (int i = 0; i < numOperands; ++i) {
                    if (ip + 1 + sizeof(int) > header.codeSize) {
                         throw std::runtime_error("Unexpected end of code segment while reading operand " + std::to_string(i+1) + " for opcode 0x" + std::to_string(static_cast<int>(opcode)) + " at offset " + std::to_string(instructionStartIp));
                    }
                    OperandType opType = static_cast<OperandType>(codeSegment[ip++]);
                    int opValue = *reinterpret_cast<const int*>(&codeSegment[ip]);
                    ip += sizeof(int);
                    std::cout << " " << formatOperand(opType, opValue);
                }
                std::cout << std::endl;

            } else {
                std::cout << "Unknown Opcode (0x" << std::hex << static_cast<int>(opcode) << std::dec << ")" << std::endl;
                // Attempt to recover or stop? For now, stop.
                throw std::runtime_error("Encountered unknown opcode during decoding.");
            }
        }
        std::cout << "--------------" << std::endl << std::endl;


        // 3. Decode Data Segment
        std::cout << "--- Data Segment (Size: " << header.dataSize << ") ---" << std::endl;
        std::vector<char> dataSegment(header.dataSize);
         if (header.dataSize > 0) {
            // Read directly from the input file stream 'in'
            if (!in.read(dataSegment.data(), header.dataSize)) {
                 throw std::runtime_error("Failed to read data segment.");
            }

            // Print data segment (e.g., hex dump)
            const int bytesPerRow = 16;
            for (uint32_t i = 0; i < header.dataSize; ++i) {
                if (i % bytesPerRow == 0) {
                    if (i > 0) std::cout << std::endl; // Newline for previous row
                    std::cout << std::setw(4) << std::setfill('0') << std::hex << i << std::dec << ": ";
                }
                std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(static_cast<unsigned char>(dataSegment[i])) << std::dec << " ";
            }
            std::cout << std::endl;
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
