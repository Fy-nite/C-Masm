#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <unordered_map>
#include <set>
#include <sstream>
#include <cstdint>
#include <cctype>

// --- Shared enums/types (should match interpreter/compiler) ---
enum OperandType {
    NONE = 0x00,
    REGISTER = 0x01,
    IMMEDIATE = 0x02,
    LABEL_ADDRESS = 0x03,
    DATA_ADDRESS = 0x04,
    REGISTER_AS_ADDRESS = 0x05
};

enum Opcode {
    MOV = 0x01, ADD, SUB, MUL, DIV, INC,
    JMP, CMP, JE, JL, CALL, RET,
    PUSH, POP,
    OUT, COUT, OUTSTR, OUTCHAR,
    HLT, ARGC, GETARG,
    DB, LBL,
    AND, OR, XOR, NOT, SHL, SHR,
    MOVADDR, MOVTO,
    JNE, JG, JLE, JGE,
    ENTER, LEAVE,
    COPY, FILL, CMP_MEM,
    MNI,
    IN
};

struct BinaryHeader {
    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t reserved = 0;
    uint32_t codeSize = 0;
    uint32_t dataSize = 0;
    uint32_t entryPoint = 0;
};

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
    {MNI, "MNI"}, {IN, "IN"}
};

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
        case REGISTER:
            if (registerIndexToString.count(value)) return registerIndexToString.at(value);
            return "R?" + std::to_string(value);
        case REGISTER_AS_ADDRESS:
            if (registerIndexToString.count(value)) return "$" + registerIndexToString.at(value);
            return "$R?" + std::to_string(value);
        case IMMEDIATE:
            return std::to_string(value);
        case LABEL_ADDRESS:
            return "#" + std::to_string(value);
        case DATA_ADDRESS:
            return "$" + std::to_string(value);
        case NONE:
            return "[NONE]";
        default:
            return "[UNKNOWN_TYPE]";
    }
}

std::string escapeString(const std::string& s) {
    std::ostringstream ss;
    for (char c : s) {
        if (c == '\n') ss << "\\n";
        else if (c == '\t') ss << "\\t";
        else if (c == '\\') ss << "\\\\";
        else if (c == '"') ss << "\\\"";
        else if (std::isprint(static_cast<unsigned char>(c))) ss << c;
        else ss << "\\x" << std::setw(2) << std::setfill('0') << std::hex << (int)(unsigned char)c;
    }
    return ss.str();
}

std::string readString(const std::vector<uint8_t>& vec, size_t& offset) {
    std::string str;
    while (offset < vec.size()) {
        char c = static_cast<char>(vec[offset++]);
        if (c == '\0') break;
        str += c;
    }
    return str;
}

void printHexBytes(const std::vector<uint8_t>& vec, size_t start, size_t end) {
    for (size_t i = start; i < end && i < vec.size(); ++i)
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)vec[i] << " ";
    std::cout << std::dec;
}

int getOperandCount(Opcode opcode) {
    switch (opcode) {
        case MOV: case ADD: case SUB: case MUL: case DIV: case CMP: case AND: case OR: case XOR: case SHL: case SHR: case MOVADDR: case MOVTO:
        case GETARG: case COPY: case FILL: case CMP_MEM: case OUT: case COUT: case OUTCHAR:
            return 2;
        case OUTSTR:
            return 3;
        case INC: case JMP: case JE: case JL: case CALL: case PUSH: case POP: case JNE: case JG: case JLE: case JGE: case ENTER: case ARGC: case IN:
            return 1;
        case RET: case LEAVE: case HLT:
            return 0;
        case MNI:
            return -1; // special
        default:
            return -2; // unknown
    }
}

// ANSI color codes
#define CLR_RESET   "\033[0m"
#define CLR_OPCODE  "\033[1;36m"
#define CLR_OFFSET  "\033[1;33m"
#define CLR_OPERAND "\033[1;32m"
#define CLR_HEX     "\033[1;35m"
#define CLR_COMMENT "\033[1;90m"
#define CLR_ERROR   "\033[1;31m"

int decoder_main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: masmd <file.bin>" << std::endl;
        return 1;
    }
    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::cerr << "Error: Cannot open input file: " << argv[1] << std::endl;
        return 1;
    }

    try {
        BinaryHeader header;
        if (!in.read(reinterpret_cast<char*>(&header), sizeof(header)))
            throw std::runtime_error("Failed to read header.");

        std::cout << "--- Header ---" << std::endl;
        std::cout << "Magic:      0x" << std::hex << header.magic << std::dec
                  << " ('" << (char)(header.magic & 0xFF)
                  << (char)((header.magic >> 8) & 0xFF)
                  << (char)((header.magic >> 16) & 0xFF)
                  << (char)((header.magic >> 24) & 0xFF) << "')" << std::endl;
        std::cout << "Version:    " << header.version << std::endl;
        std::cout << "Code Size:  " << header.codeSize << " bytes" << std::endl;
        std::cout << "Data Size:  " << header.dataSize << " bytes" << std::endl;
        std::cout << "Entry Point:" << header.entryPoint << " (offset)" << std::endl;
        std::cout << "--------------" << std::endl << std::endl;

        std::vector<uint8_t> code(header.codeSize);
        if (header.codeSize > 0 && !in.read(reinterpret_cast<char*>(code.data()), header.codeSize))
            throw std::runtime_error("Failed to read code segment.");

        std::set<uint32_t> referencedDataOffsets;
        std::cout << "--- Code Segment (Size: " << header.codeSize << ") ---" << std::endl;
        std::cout << "Offset  | Bytes        | Disassembly" << std::endl;
        std::cout << "--------|--------------|--------------------------------" << std::endl;

        size_t ip = 0;
        while (ip < code.size()) {
            size_t startIp = ip;
            uint8_t opcodeByte = code[ip++];
            Opcode opcode = static_cast<Opcode>(opcodeByte);

            std::ostringstream diss;
            diss << CLR_OPCODE << std::setw(8) << std::left
                 << (opcodeToString.count(opcode) ? opcodeToString.at(opcode) : ("0x" + std::to_string(opcodeByte)))
                 << CLR_RESET;

            std::vector<std::pair<OperandType, int>> operands;
            std::string mniFunc;
            size_t tempIp = ip;
            bool unknown = false;

            try {
                int opCount = getOperandCount(opcode);
                if (opcode == MNI) {
                    mniFunc = readString(code, tempIp);
                    while (tempIp + 1 + 4 <= code.size()) {
                        OperandType t = static_cast<OperandType>(code[tempIp++]);
                        int v = *reinterpret_cast<const int*>(&code[tempIp]);
                        tempIp += 4;
                        if (t == NONE) break;
                        operands.emplace_back(t, v);
                        if (t == DATA_ADDRESS) referencedDataOffsets.insert(v);
                    }
                } else if (opCount >= 0) {
                    for (int i = 0; i < opCount; ++i) {
                        if (tempIp + 1 + 4 > code.size())
                            throw std::runtime_error("Unexpected end of code segment while reading operand");
                        OperandType t = static_cast<OperandType>(code[tempIp++]);
                        int v = *reinterpret_cast<const int*>(&code[tempIp]);
                        tempIp += 4;
                        operands.emplace_back(t, v);
                        if (t == DATA_ADDRESS) referencedDataOffsets.insert(v);
                    }
                } else if (!opcodeToString.count(opcode)) {
                    throw std::runtime_error("Unknown opcode encountered.");
                }
            } catch (...) {
                unknown = true;
            }

            size_t endIp = tempIp;
            std::cout << CLR_OFFSET << std::setw(7) << std::setfill('0') << startIp << CLR_RESET << " | ";
            std::cout << CLR_HEX;
            printHexBytes(code, startIp, endIp);
            std::cout << CLR_RESET << std::setw(12 - 3 * (endIp - startIp)) << " | ";

            // Colorize disassembly
            if (opcode == MNI) {
                std::cout << CLR_OPCODE << "MNI" << CLR_RESET << " " << CLR_OPERAND << mniFunc << CLR_RESET;
                for (auto& op : operands)
                    std::cout << " " << CLR_OPERAND << formatOperand(op.first, op.second) << CLR_RESET;
                std::cout << std::endl;
            } else if (opcodeToString.count(opcode)) {
                std::cout << CLR_OPCODE << opcodeToString.at(opcode) << CLR_RESET;
                for (auto& op : operands)
                    std::cout << " " << CLR_OPERAND << formatOperand(op.first, op.second) << CLR_RESET;
                std::cout << std::endl;
            } else {
                std::cout << CLR_ERROR << "Unknown Opcode (0x" << std::hex << (int)opcodeByte << std::dec << ")" << CLR_RESET << std::endl;
                unknown = true;
            }
            if (unknown) break;
            ip = endIp;
        }
        std::cout << "--------|--------------|--------------------------------" << std::endl << std::endl;

        // Data segment (colorize comments)
        std::cout << "--- Data Segment (Size: " << header.dataSize << ") ---" << std::endl;
        std::vector<char> data(header.dataSize);
        std::vector<bool> processed(header.dataSize, false);
        if (header.dataSize > 0 && !in.read(data.data(), header.dataSize))
            throw std::runtime_error("Failed to read data segment.");

        for (uint32_t i = 0; i < header.dataSize;) {
            if (processed[i]) { ++i; continue; }
            bool isRef = referencedDataOffsets.count(i);
            std::string s;
            bool isStr = false;
            uint32_t end = i;
            if (isRef) {
                uint32_t k = i;
                bool possible = true;
                while (k < header.dataSize) {
                    char c = data[k];
                    if (c == '\0') { end = k; isStr = possible; break; }
                    if (!std::isprint((unsigned char)c) && c != '\n' && c != '\t') possible = false;
                    s += c; ++k;
                }
                if (k == header.dataSize && data[k-1] != '\0') isStr = false;
            }
            if (isStr) {
                std::cout << CLR_COMMENT << "; Referenced Data (String)" << CLR_RESET << std::endl;
                std::cout << "DB $" << i << " \"" << escapeString(s) << "\"" << std::endl;
                for (uint32_t p = i; p <= end; ++p) processed[p] = true;
                i = end + 1;
            } else {
                if (isRef) std::cout << CLR_COMMENT << "; Referenced Data (Byte)" << CLR_RESET << std::endl;
                else std::cout << CLR_COMMENT << "; Unreferenced Data (Byte)" << CLR_RESET << std::endl;
                std::cout << "DB $" << i << " 0x"
                          << std::setw(2) << std::setfill('0') << std::hex
                          << (int)(unsigned char)data[i]
                          << std::dec << std::endl;
                processed[i] = true;
                ++i;
            }
        }
        if (header.dataSize == 0) std::cout << "(Empty)" << std::endl;
        std::cout << "--------------" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << CLR_ERROR << "Decoding Error: " << e.what() << CLR_RESET << std::endl;
        return 1;
    }
    return 0;
}
