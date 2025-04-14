#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm> // Required for std::transform and potentially find_if_not
#include <cctype>    // Required for std::isspace
#include <climits>   // Required for INT_MIN, INT_MAX
#include <cstdint> // Required for uint32_t
#include "operand_types.h" // Include the new operand type definitions

// Define a simple binary header structure
struct BinaryHeader {
    uint32_t magic = 0x4D53414D; // "MASM" in ASCII (little-endian)
    uint16_t version = 1;
    uint16_t reserved = 0; // Padding/Reserved for future use
    uint32_t codeSize = 0;
    uint32_t dataSize = 0;
    uint32_t entryPoint = 0; // Offset within the code segment
};

// Updated Opcode Enum - Removed MOVI
enum Opcode {
    // Basic
    MOV = 0x01, ADD, SUB, MUL, DIV, INC, // MOVI removed
    // Flow Control
    JMP, CMP, JE, JL, CALL, RET,
    // Stack
    PUSH, POP,
    // I/O (Simplified: Assume operands are registers or immediates)
    OUT, COUT, OUTSTR, OUTCHAR,
    // Program Control
    HLT, ARGC, GETARG,
    // Data Definition (Placeholder - requires more complex handling)
    DB,
    // Labels
    LBL, // Pseudo-instruction for parsing
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

struct Instruction {
    Opcode opcode;
    std::vector<std::string> operands;
};

// Structure to hold resolved operand info
struct ResolvedOperand {
    OperandType type = OperandType::NONE;
    int value = 0;
};

class Compiler {
    std::unordered_map<std::string, int> labelMap;
    std::vector<Instruction> instructions;
    std::vector<char> dataSegment; // Basic data segment
    std::unordered_map<std::string, int> dataLabels; // Map data labels ($1) to offsets
    int currentAddress = 0;
    int dataAddress = 0; // Track data segment size/address

public:
    void parse(const std::string& source) {
        std::istringstream stream(source);
        std::string line;
        while (std::getline(stream, line)) {
            parseLine(line);
        }
    }

    void parseLine(const std::string& line) {
        std::istringstream stream(line);
        std::string token;
        stream >> token;

        if (token.empty() || token[0] == ';') return; // Skip comments and empty lines

        // Convert token to uppercase for case-insensitivity
        std::string upperToken = token;
        std::transform(upperToken.begin(), upperToken.end(), upperToken.begin(), ::toupper);


        if (upperToken == "LBL") {
            std::string label;
            stream >> label;
            if (label.empty()) throw std::runtime_error("Label name missing");
            labelMap["#" + label] = currentAddress; // Store labels with # prefix
        } else if (upperToken == "DB") {
             // Simplified DB handling: Assume address is a label like $1, $2 etc.
             // and string follows. Compiler needs to manage this data.
             // For now, we'll just note it requires more work.
             // Example: DB $1 "Hello"
             std::string dataLabel, dataValue;
             stream >> dataLabel; // e.g., $1
             std::getline(stream >> std::ws, dataValue); // Read rest of line as string

             // Trim trailing whitespace from dataValue
             dataValue.erase(std::find_if_not(dataValue.rbegin(), dataValue.rend(), [](unsigned char ch) {
                 return std::isspace(ch);
             }).base(), dataValue.end());

             // Now check for quotes on the potentially trimmed string
             if (dataValue.length() >= 2 && dataValue.front() == '"' && dataValue.back() == '"') {
                 dataValue = dataValue.substr(1, dataValue.length() - 2); // Remove quotes
             } else {
                 // If it still fails, throw the error
                 throw std::runtime_error("DB requires a quoted string (check quotes and content): [" + dataValue + "]");
             }
             dataLabels[dataLabel] = dataAddress;
             // Handle escape sequences like \n (basic implementation)
             std::string processedValue;
             for (size_t i = 0; i < dataValue.length(); ++i) {
                 if (dataValue[i] == '\\' && i + 1 < dataValue.length()) {
                     switch (dataValue[i+1]) {
                         case 'n': processedValue += '\n'; i++; break;
                         case 't': processedValue += '\t'; i++; break;
                         case '\\': processedValue += '\\'; i++; break;
                         case '"': processedValue += '"'; i++; break;
                         // Add other escapes if needed
                         default: processedValue += dataValue[i]; break; // Keep backslash if not a known escape
                     }
                 } else {
                     processedValue += dataValue[i];
                 }
             }

             for(char c : processedValue) {
                 dataSegment.push_back(c);
             }
             dataSegment.push_back('\0'); // Null-terminate for convenience
             dataAddress += processedValue.length() + 1;

        } else {
            Instruction instr;
            try {
                 instr.opcode = getOpcode(upperToken);
            } catch (const std::out_of_range& oor) {
                 throw std::runtime_error("Unknown instruction: " + token);
            }
            std::string operand;
            while (stream >> operand) {
                instr.operands.push_back(operand);
            }

            // *** Refinement for MOV: Determine if source is immediate ***
            // This simple compiler doesn't explicitly track types in bytecode.
            // The interpreter will have to infer based on the resolved value,
            // or we'd need a more complex bytecode format.
            // For now, resolveOperand handles immediate detection.

            instructions.push_back(instr);
            // Increment address: 1 byte opcode + (1 byte type + 4 bytes value) per operand
            currentAddress += 1 + instr.operands.size() * (1 + sizeof(int));
        }
    }

    Opcode getOpcode(const std::string& mnemonic) {
         // Case-insensitive lookup
        std::string upperMnemonic = mnemonic;
        std::transform(upperMnemonic.begin(), upperMnemonic.end(), upperMnemonic.begin(), ::toupper);

        // Updated map - MOVI removed
        static std::unordered_map<std::string, Opcode> opcodeMap = {
            {"MOV", MOV}, {"ADD", ADD}, {"SUB", SUB}, {"MUL", MUL}, {"DIV", DIV}, {"INC", INC}, // MOVI removed
            {"JMP", JMP}, {"CMP", CMP}, {"JE", JE}, {"JL", JL}, {"CALL", CALL}, {"RET", RET},
            {"PUSH", PUSH}, {"POP", POP},
            {"OUT", OUT}, {"COUT", COUT}, {"OUTSTR", OUTSTR}, {"OUTCHAR", OUTCHAR},
            {"HLT", HLT}, {"ARGC", ARGC}, {"GETARG", GETARG},
            {"DB", DB}, // Keep DB for parsing, but compilation needs specific logic
            // LBL is handled during parsing, not compiled directly
            {"AND", AND}, {"OR", OR}, {"XOR", XOR}, {"NOT", NOT}, {"SHL", SHL}, {"SHR", SHR},
            {"MOVADDR", MOVADDR}, {"MOVTO", MOVTO},
            {"JNE", JNE}, {"JG", JG}, {"JLE", JLE}, {"JGE", JGE},
            {"ENTER", ENTER}, {"LEAVE", LEAVE},
            {"COPY", COPY}, {"FILL", FILL}, {"CMP_MEM", CMP_MEM}
        };
        // Use .at() for bounds checking which throws std::out_of_range if key not found
        return opcodeMap.at(upperMnemonic);
    }

    void compile(const std::string& outputFile) {
        std::ofstream out(outputFile, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot open output file: " + outputFile);

        // Calculate actual code size
        uint32_t actualCodeSize = 0;
        for (const auto& instr : instructions) {
            if (instr.opcode == DB) continue; // Skip DB pseudo-instruction
            actualCodeSize += 1; // Opcode
            actualCodeSize += instr.operands.size() * (1 + sizeof(int)); // Type + Value per operand
        }

        // Prepare the header
        BinaryHeader header;
        header.codeSize = actualCodeSize;
        header.dataSize = dataSegment.size();
        header.entryPoint = 0; // Assuming execution starts at the beginning of code segment

        // Write the header
        out.write(reinterpret_cast<const char*>(&header), sizeof(header));

        // Write code segment
        for (const auto& instr : instructions) {
            if (instr.opcode == DB) continue; // Skip DB pseudo-instruction in code segment

            out.put(static_cast<char>(instr.opcode)); // Write Opcode (1 byte)
            for (const auto& operand : instr.operands) {
                ResolvedOperand resolved = resolveOperand(operand, instr.opcode); // Pass opcode for context if needed
                out.put(static_cast<char>(resolved.type)); // Write Operand Type (1 byte)
                out.write(reinterpret_cast<const char*>(&resolved.value), sizeof(resolved.value)); // Write Operand Value (4 bytes)
            }
        }

        // Write data segment
        if (!dataSegment.empty()) {
            out.write(dataSegment.data(), dataSegment.size());
        }
        // Note: Interpreter needs to read the header to know segment sizes and entry point.
    }

    // Modified resolveOperand to return ResolvedOperand struct
    ResolvedOperand resolveOperand(const std::string& operand, Opcode contextOpcode = (Opcode)0) { // Context opcode might be useful later
        ResolvedOperand result;
        if (operand.empty()) throw std::runtime_error("Empty operand encountered");

        if (operand[0] == '#') { // Label address (for jumps/calls)
            if (labelMap.count(operand)) {
                result.type = OperandType::LABEL_ADDRESS;
                result.value = labelMap.at(operand);
            } else {
                throw std::runtime_error("Undefined label: " + operand);
            }
        } else if (operand[0] == '$') { // Data Label (e.g., $1) or Immediate ($500)
             if (dataLabels.count(operand)) {
                 // Data reference
                 result.type = OperandType::DATA_ADDRESS;
                 // Value is the offset within the data segment
                 result.value = dataLabels.at(operand);
             } else {
                 // Try parsing as an immediate number (e.g., $500)
                 try {
                     long long val = std::stoll(operand.substr(1));
                     if (val < INT_MIN || val > INT_MAX) {
                         throw std::runtime_error("Immediate value ($) out of 32-bit range: " + operand);
                     }
                     result.type = OperandType::IMMEDIATE;
                     result.value = static_cast<int>(val);
                 } catch (...) { // Catch invalid_argument, out_of_range
                     throw std::runtime_error("Invalid immediate value or undefined data label starting with $: " + operand);
                 }
             }
        } else if (toupper(operand[0]) == 'R') { // Register
            std::string regName = operand;
            std::transform(regName.begin(), regName.end(), regName.begin(), ::toupper);
            // ... (existing regMap lookup code) ...
            static const std::unordered_map<std::string, int> regMap = {
                {"RAX", 0}, {"RBX", 1}, {"RCX", 2}, {"RDX", 3},
                {"RSI", 4}, {"RDI", 5}, {"RBP", 6}, {"RSP", 7},
                {"RIP", -1}, // Special case, usually not directly settable
                {"R0", 8}, {"R1", 9}, {"R2", 10}, {"R3", 11},
                {"R4", 12}, {"R5", 13}, {"R6", 14}, {"R7", 15},
                {"R8", 16}, {"R9", 17}, {"R10", 18}, {"R11", 19},
                {"R12", 20}, {"R13", 21}, {"R14", 22}, {"R15", 23}
            };
            if (regMap.count(regName)) {
                int regIndex = regMap.at(regName);
                if (regIndex == -1) throw std::runtime_error("Cannot directly use RIP as operand");
                result.type = OperandType::REGISTER;
                result.value = regIndex;
            } else {
                 try {
                     int regIndex = std::stoi(operand.substr(1));
                     if (regIndex >= 0 && regIndex <= 15) {
                         result.type = OperandType::REGISTER;
                         result.value = regIndex + 8; // Map R0-R15 to indices 8-23
                     } else {
                         throw std::runtime_error("Register index out of range (R0-R15): " + operand);
                     }
                 } catch (...) {
                     throw std::runtime_error("Unknown register: " + operand);
                 }
            }
        } else { // Immediate value (not starting with $ or # or R)
            try {
                long long val = std::stoll(operand);
                 if (val < INT_MIN || val > INT_MAX) {
                     throw std::runtime_error("Immediate value out of 32-bit range: " + operand);
                 }
                result.type = OperandType::IMMEDIATE;
                result.value = static_cast<int>(val);
            } catch (...) { // Catch invalid_argument, out_of_range
                throw std::runtime_error("Invalid immediate value or unknown operand: " + operand);
            }
        }
        return result;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: microasm_compiler <source.masm> <output.bin>" << std::endl;
        return 1;
    }

    std::string sourceFile = argv[1];
    std::string outputFile = argv[2];

    try {
        std::ifstream fileStream(sourceFile);
        if (!fileStream) {
            throw std::runtime_error("Could not open source file: " + sourceFile);
        }
        std::ostringstream buffer;
        buffer << fileStream.rdbuf();

        Compiler compiler;
        // First pass (or equivalent logic) to resolve labels and data
        compiler.parse(buffer.str());
        // Second pass (compilation)
        compiler.compile(outputFile);

        std::cout << "Compilation successful: " << sourceFile << " -> " << outputFile << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Compilation Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
