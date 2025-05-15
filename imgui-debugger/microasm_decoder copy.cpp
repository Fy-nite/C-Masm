#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <map>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <cstdint>
#include <cctype>
#include "operand_types.h"
#include <raylib.h>

// Logging utility
enum class LogLevel { INFO, WARNING, ERROR };

std::vector<std::string> decoderOutputLines;
std::vector<std::string> dataSegmentLines; // Declare here

// Helper to add a line to the output buffer
void addOutputLine(const std::string& line) {
    decoderOutputLines.push_back(line);
}

// Replace log* functions to also add to ImGui output
void log(LogLevel level, const std::string& message) {
    std::string prefix;
    switch (level) {
        case LogLevel::INFO:    prefix = "[INFO] "; break;
        case LogLevel::WARNING: prefix = "[WARNING] "; break;
        case LogLevel::ERROR:   prefix = "[ERROR] "; break;
    }
    addOutputLine(prefix + message);
    // Optionally still print to stderr for errors
    if (level == LogLevel::ERROR) std::cerr << prefix << message << std::endl;
}

void logInfo(const std::string& message) {
    log(LogLevel::INFO, message);
}

void logWarning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void logError(const std::string& message) {
    log(LogLevel::ERROR, message);
}

// Placeholder for the machine state (registers, memory, etc.)
// In a real implementation, this would be a class or struct
struct RegisterMachineState {
    std::map<std::string, int> registers; // Example register map
    std::vector<unsigned char> memory;    // Example memory

};

// Define the type for MNI functions
// It takes the machine state and a vector of string arguments (registers/immediates)
using MniFunctionType = std::function<void(RegisterMachineState&, const std::vector<std::string>&)>;

extern std::map<std::string, MniFunctionType> mniRegistry;

// Function to register MNI functions
void registerMNI(const std::string& module, const std::string& name, MniFunctionType func) {
    std::string fullName = module + "." + name;
    if (mniRegistry.find(fullName) == mniRegistry.end()) {
        mniRegistry[fullName] = func;
     
        logInfo("Registered MNI function: " + fullName);
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
    std::cout << "Called Math.sin with args: " << args[0] << ", " << args[1] << std::endl;
}

// Function to initialize MNI functions (call this during startup)
void initializeMNIFunctions() {
    registerMNI("Math", "sin", exampleMathSin);

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
    OP_OUT, COUT, OUTSTR, OUTCHAR,
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
    {OP_OUT, "OUT"}, {COUT, "COUT"}, {OUTSTR, "OUTSTR"}, {OUTCHAR, "OUTCHAR"},
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
    {OP_OUT, 2}, {COUT, 2}, {OUTSTR, 3}, {OUTCHAR, 2},
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
        case OperandType::REGISTER_AS_ADDRESS:
            if (registerIndexToString.count(value)) {
                return "[$" + registerIndexToString.at(value) + "]";
            } else {
                return "[$R?" + std::to_string(value) + "]";
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

// Helper function to colorize a byte value as hex (REMOVE terminal escape codes)
std::string colorHexByte(uint8_t byte) {
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << std::hex << (int)byte;
    return ss.str();
}

// Helper function to format a sequence of bytes as hex with color
std::string formatBytesToHexColored(const std::vector<uint8_t>& vec, uint32_t start, uint32_t end) {
    std::stringstream ss;
    for (uint32_t i = start; i < end && i < vec.size(); ++i) {
        ss << colorHexByte(vec[i]) << " ";
    }
    return ss.str();
}

// Simulate execution to find referenced data offsets
void findReferencedDataOffsets(
    const std::vector<uint8_t>& codeSegment,
    uint32_t entryPoint,
    const std::unordered_map<Opcode, int>& opcodeOperandCount,
    std::set<uint32_t>& referencedDataOffsets,
    std::set<uint32_t>& visitedOffsets
) {
    uint32_t codeSize = codeSegment.size();
    std::vector<uint32_t> toVisit = {entryPoint};

    while (!toVisit.empty()) {
        uint32_t ip = toVisit.back();
        toVisit.pop_back();

        if (visitedOffsets.count(ip) || ip >= codeSize) continue;
        visitedOffsets.insert(ip);

        uint32_t instructionStartIp = ip;
        Opcode opcode = static_cast<Opcode>(codeSegment[ip++]);

        // Parse operands
        std::vector<std::pair<OperandType, int>> currentOperands;
        std::string mniFunctionName = "";
        uint32_t tempIp = ip;

        if (opcode == MNI) {
            mniFunctionName = readStringFromVector(codeSegment, tempIp);
            while (tempIp < codeSize) {
                if (tempIp + 1 + sizeof(int) > codeSize) break;
                OperandType opType = static_cast<OperandType>(codeSegment[tempIp]);
                int opValue = *reinterpret_cast<const int*>(&codeSegment[tempIp + 1]);
                if (opType == OperandType::NONE) {
                    tempIp += 1 + sizeof(int);
                    break;
                }
                currentOperands.push_back({opType, opValue});
                tempIp += 1 + sizeof(int);
                if (opType == OperandType::DATA_ADDRESS) {
                    referencedDataOffsets.insert(static_cast<uint32_t>(opValue));
                }
            }
        } else if (opcodeOperandCount.count(opcode)) {
            int numOperands = opcodeOperandCount.at(opcode);
            for (int i = 0; i < numOperands; ++i) {
                if (tempIp + 1 + sizeof(int) > codeSize) break;
                OperandType opType = static_cast<OperandType>(codeSegment[tempIp]);
                int opValue = *reinterpret_cast<const int*>(&codeSegment[tempIp + 1]);
                currentOperands.push_back({opType, opValue});
                tempIp += 1 + sizeof(int);
                if (opType == OperandType::DATA_ADDRESS) {
                    referencedDataOffsets.insert(static_cast<uint32_t>(opValue));
                }
            }
        }

        // Follow control flow: JMP, CALL, JE, JL, JNE, JG, JLE, JGE
        // (Assume first operand is LABEL_ADDRESS for these)
        switch (opcode) {
            case JMP: case CALL: case JE: case JL: case JNE: case JG: case JLE: case JGE:
                if (!currentOperands.empty() && currentOperands[0].first == OperandType::LABEL_ADDRESS) {
                    uint32_t target = static_cast<uint32_t>(currentOperands[0].second);
                    toVisit.push_back(target);
                }
                // For CALL, also continue after call (fall-through)
                if (opcode == CALL) toVisit.push_back(tempIp);
                break;
            case RET: case HLT:
                // End of flow
                break;
            default:
                // Fall-through to next instruction
                toVisit.push_back(tempIp);
                break;
        }
    }
}

// Helper function to render a scrollable pane with interactivity
int renderInteractivePane(const std::vector<std::string>& lines, const Rectangle& pane, int& scrollOffset, Font font, int fontSize, int selectedLine) {
    DrawRectangleRec(pane, Color{40, 44, 52, 255});
    int y = pane.y - scrollOffset;
    int lineIndex = 0;
    int clickedLine = -1;

    for (const auto& line : lines) {
        if (y > pane.y - fontSize && y < pane.y + pane.height) {
            // Highlight selected line
            if (lineIndex == selectedLine) {
                DrawRectangle(pane.x, y, pane.width, fontSize + 4, Color{60, 60, 80, 255});
            }

            // Draw line text
            DrawTextEx(font, line.c_str(), (Vector2){pane.x + 8, (float)y}, (float)fontSize, 1, RAYWHITE);

            // Detect mouse click
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mousePos = GetMousePosition();
                if (mousePos.x >= pane.x && mousePos.x <= pane.x + pane.width && mousePos.y >= y && mousePos.y <= y + fontSize + 4) {
                    clickedLine = lineIndex;
                }
            }
        }
        y += fontSize + 4;
        lineIndex++;
    }

    return clickedLine;
}

int main(int argc, char* argv[]) {
    // --- Raylib window initialization ---
    const int screenWidth = 1200;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Masm Decoder (raylib)");
    SetTargetFPS(60);

    // --- Font loading ---
    const int fontSize = 16; // Reduced font size
    const int headerFontSize = 24; // Reduced header font size
    Font font = LoadFontEx("resources/JetBrainsMono-Regular.ttf", fontSize, 0, 0);
    Font headerFont = LoadFontEx("resources/JetBrainsMono-Regular.ttf", headerFontSize, 0, 0);
    if (font.texture.id == 0) font = GetFontDefault();
    if (headerFont.texture.id == 0) headerFont = GetFontDefault();

    // --- Argument check ---
    if (argc < 2) {
        addOutputLine("Usage: microasm_decoder <inputfile>");
        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(Color{30, 34, 40, 255});
            DrawRectangleRounded((Rectangle){20, 20, screenWidth-40, 80}, 0.2f, 8, Color{40, 44, 52, 255});
            DrawTextEx(headerFont, "Masm Decoder Output", (Vector2){40, 36}, (float)headerFontSize, 2, RAYWHITE);
            DrawTextEx(font, "Usage: microasm_decoder <inputfile>", (Vector2){40, 80}, (float)fontSize, 1, RED);
            EndDrawing();
        }
        CloseWindow();
        UnloadFont(font);
        UnloadFont(headerFont);
        return 1;
    }

    std::string inputFile = argv[1];
    std::ifstream in(inputFile, std::ios::binary);
    if (!in) {
        addOutputLine("Error: Cannot open input file: " + inputFile);
        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(Color{30, 34, 40, 255});
            DrawRectangleRounded((Rectangle){20, 20, screenWidth-40, 80}, 0.2f, 8, Color{40, 44, 52, 255});
            DrawTextEx(headerFont, "Masm Decoder Output", (Vector2){40, 36}, (float)headerFontSize, 2, RAYWHITE);
            DrawTextEx(font, ("Error: Cannot open input file: " + inputFile).c_str(), (Vector2){40, 80}, (float)fontSize, 1, RED);
            EndDrawing();
        }
        CloseWindow();
        UnloadFont(font);
        UnloadFont(headerFont);
        return 1;
    }

    // Declare dataSegment to store the data segment
    std::vector<char> dataSegment;
    // dataSegmentLines is now declared globally

    try {
        addOutputLine("==========================================");

        // 1. Read and print header
        BinaryHeader header;
        if (!in.read(reinterpret_cast<char*>(&header), sizeof(header))) {
            throw std::runtime_error("Failed to read header.");
        }

        addOutputLine("--- Header ---");
        std::ostringstream oss;
        oss << "Magic:      0x" << std::hex << header.magic << std::dec
            << " ('" << static_cast<char>(header.magic & 0xFF)
            << static_cast<char>((header.magic >> 8) & 0xFF)
            << static_cast<char>((header.magic >> 16) & 0xFF)
            << static_cast<char>((header.magic >> 24) & 0xFF) << "')";
        addOutputLine(oss.str());
        oss.str("");
        oss << "Version:    " << header.version;
        addOutputLine(oss.str());
        oss.str("");
        oss << "Code Size:  " << header.codeSize << " bytes";
        addOutputLine(oss.str());
        oss.str("");
        oss << "Data Size:  " << header.dataSize << " bytes";
        addOutputLine(oss.str());
        oss.str("");
        oss << "Entry Point:" << header.entryPoint << " (offset)";
        addOutputLine(oss.str());
        addOutputLine("--------------");
        addOutputLine("");

        // Set to store data segment offsets referenced by the code
        std::set<uint32_t> referencedDataOffsets;

        // 2. Decode Code Segment
        oss.str("");
        oss << "--- Code Segment (Size: " << header.codeSize << ") ---";
        addOutputLine(oss.str());
        addOutputLine("Offset  | Bytes        | Disassembly");
        addOutputLine("--------|--------------|--------------------------------");
        std::vector<uint8_t> codeSegment(header.codeSize);
        if (header.codeSize > 0) {
            if (!in.read(reinterpret_cast<char*>(codeSegment.data()), header.codeSize)) {
                throw std::runtime_error("Failed to read code segment.");
            }
        }

        // --- NEW: Static analysis to find referenced data offsets ---
        std::set<uint32_t> visitedOffsets;
        findReferencedDataOffsets(
            codeSegment,
            header.entryPoint,
            opcodeOperandCount,
            referencedDataOffsets,
            visitedOffsets
        );
        // --- END NEW ---

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
            oss.str("");
            oss << std::setw(7) << std::setfill('0') << instructionStartIp << " | ";
            // Print Hex Bytes (colored, pad to 12 chars width)
            std::string hexBytes = formatBytesToHexColored(codeSegment, instructionStartIp, instructionEndIp);
            // Remove ANSI codes for width calculation
            std::string hexBytesNoColor;
            bool inEscape = false;
            for (char c : hexBytes) {
                if (c == '\033') inEscape = true;
                else if (inEscape && c == 'm') inEscape = false;
                else if (!inEscape) hexBytesNoColor += c;
            }
            int padLen = 12 - static_cast<int>(hexBytesNoColor.length());
            if (padLen < 0) padLen = 0;
            oss << hexBytes << std::string(padLen, ' ') << " | ";
            oss << std::setfill(' '); // Reset fill character

            // Print Disassembly
            if (opcode == MNI) {
                oss << std::setw(8) << std::left << "MNI" << std::right << " " << mniFunctionName;
                for (const auto& opPair : currentOperands) {
                    oss << " " << formatOperand(opPair.first, opPair.second);
                }
                addOutputLine(oss.str());
            } else if (opcodeToString.count(opcode)) {
                oss << std::setw(8) << std::left << opcodeToString.at(opcode) << std::right;
                for (const auto& opPair : currentOperands) {
                    oss << " " << formatOperand(opPair.first, opPair.second);
                }
                addOutputLine(oss.str());
            } else {
                 // Should have been caught earlier, but as a fallback
                 oss << "Unknown Opcode (0x" << std::hex << static_cast<int>(opcode) << std::dec << ")";
                 addOutputLine(oss.str());
            }

            ip = instructionEndIp; // Advance main IP to the end of the processed instruction

        }
        addOutputLine("--------|--------------|--------------------------------");
        addOutputLine("");

        // 3. Read Data Segment into memory first
        dataSegment.resize(header.dataSize); // Resize dataSegment to match the data size
        if (header.dataSize > 0) {
            if (!in.read(dataSegment.data(), header.dataSize)) {
                 throw std::runtime_error("Failed to read data segment.");
            }
        }

        // Now generate the hex view for the data segment pane
        oss.str("");
        oss << "--- Data Segment (Size: " << header.dataSize << ") ---";
        addOutputLine(oss.str()); // Add header to the main log

        // Clear the separate data source and repopulate it
        dataSegmentLines.clear(); // Clear previous content if any
        dataSegmentLines.push_back("Offset  | Hex Bytes (x10)                     | ASCII");
        dataSegmentLines.push_back("--------|-------------------------------------|----------------");

        const int bytesPerLine = 16; // Bytes per line in the hex view

        if (header.dataSize > 0) {
            for (uint32_t i = 0; i < header.dataSize; i += bytesPerLine) {
                oss.str("");
                // Offset column
                oss << std::setw(7) << std::setfill('0') << std::hex << i << std::dec << " | ";

                // Hex Bytes column
                std::string asciiChars = "";
                for (int j = 0; j < bytesPerLine; ++j) {
                    uint32_t currentOffset = i + j;
                    if (currentOffset < header.dataSize) {
                        uint8_t byte = static_cast<uint8_t>(dataSegment[currentOffset]);
                        oss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(byte) << " ";
                        // ASCII representation (replace non-printable with '.')
                        asciiChars += (std::isprint(byte) ? static_cast<char>(byte) : '.');
                    } else {
                        oss << "   "; // Padding if line extends beyond data size
                        asciiChars += " ";
                    }
                }
                // Ensure hex column has consistent width even if last line is short
                int hexPadding = (bytesPerLine * 3) - (oss.str().length() - 10); // 10 is approx length of "offset | "
                 if (hexPadding < 0) hexPadding = 0;
                 oss << std::string(hexPadding, ' ');


                oss << "| " << asciiChars;
                dataSegmentLines.push_back(oss.str());

                // Optional: Add comments for referenced data or strings (more complex)
                // Example: Check if 'i' is in referencedDataOffsets or start of a known string
                // if (referencedDataOffsets.count(i)) {
                //     dataSegmentLines.back() += " ; Referenced";
                // }
            }
        } else {
             dataSegmentLines.push_back("(Empty)");
        }
        // Add a separator line to the main log as well
        addOutputLine("--------------");
        addOutputLine("Data segment view generated separately."); // Inform user in main log
        addOutputLine("==========================================");


    } catch (const std::exception& e) {
        addOutputLine(std::string("Decoding Error: ") + e.what());
        // Ensure dataSegmentLines has at least the header if error occurs before population
        if (dataSegmentLines.empty()) { // Now accessible here
             dataSegmentLines.push_back("Offset  | Hex Bytes (x10)                     | ASCII");
             dataSegmentLines.push_back("--------|-------------------------------------|----------------");
             dataSegmentLines.push_back("Error during decoding.");
        }
        // No return here, let it proceed to show error in Raylib window
    }

    // --- Pane definitions (REMOVE from here) ---
    // Rectangle codePane, dataPane, detailsPane; // Declare here, define in loop

    int codeScrollOffset = 0;
    int dataScrollOffset = 0;
    int selectedCodeLine = -1;
    int selectedDataLine = -1;

    // --- Raylib main loop ---
    while (!WindowShouldClose()) {
        // --- Fullscreen Toggle ---
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        // --- Dynamic Pane Layout (calculate inside loop) ---
        int currentScreenWidth = GetScreenWidth();
        int currentScreenHeight = GetScreenHeight();

        // Adjust margins and gaps as needed
        float leftMargin = 32.0f;
        float rightMargin = 16.0f;
        float topMargin = 100.0f; // Space for header + pane titles
        float bottomMargin = 40.0f;
        float horizontalGap = 8.0f;

        float totalUsableWidth = currentScreenWidth - leftMargin - rightMargin - (2 * horizontalGap); // 2 gaps between 3 panes
        float paneHeight = currentScreenHeight - topMargin - bottomMargin;

        // Prevent negative widths/heights if window is too small
        if (totalUsableWidth < 0) totalUsableWidth = 0;
        if (paneHeight < 0) paneHeight = 0;

        // Adjust percentages as desired
        float codeWidth = totalUsableWidth * 0.45f;
        float dataWidth = totalUsableWidth * 0.35f;
        float detailsWidth = totalUsableWidth * 0.20f; // Remaining space

        Rectangle codePane = { leftMargin, topMargin, codeWidth, paneHeight };
        Rectangle dataPane = { codePane.x + codeWidth + horizontalGap, topMargin, dataWidth, paneHeight };
        Rectangle detailsPane = { dataPane.x + dataWidth + horizontalGap, topMargin, detailsWidth, paneHeight };


        // Handle scrolling for each pane
        Vector2 mousePos = GetMousePosition();
        int wheel = GetMouseWheelMove();
        if (CheckCollisionPointRec(mousePos, codePane)) {
            codeScrollOffset -= wheel * fontSize * 3;
            if (codeScrollOffset < 0) codeScrollOffset = 0;
        } else if (CheckCollisionPointRec(mousePos, dataPane)) {
            dataScrollOffset -= wheel * fontSize * 3;
            if (dataScrollOffset < 0) dataScrollOffset = 0;
        }

        BeginDrawing();
        ClearBackground(Color{30, 34, 40, 255});

        // Header
        DrawTextEx(headerFont, "Masm Decoder", (Vector2){leftMargin, 32}, (float)headerFontSize, 2, RAYWHITE);
        DrawTextEx(font, "[F11 Toggle Fullscreen]", (Vector2){static_cast<float>(currentScreenWidth - 200), 32}, (float)fontSize, 1, LIGHTGRAY);


        // Render Code Segment Pane
        DrawTextEx(font, "Code Segment", (Vector2){codePane.x + 8, codePane.y - 20}, (float)fontSize, 1, LIGHTGRAY);
        int clickedCodeLine = renderInteractivePane(decoderOutputLines, codePane, codeScrollOffset, font, fontSize, selectedCodeLine);
        if (clickedCodeLine != -1) selectedCodeLine = clickedCodeLine;

        // Render Data Segment Pane
        DrawTextEx(font, "Data Segment", (Vector2){dataPane.x + 8, dataPane.y - 20}, (float)fontSize, 1, LIGHTGRAY);
        int clickedDataLine = renderInteractivePane(dataSegmentLines, dataPane, dataScrollOffset, font, fontSize, selectedDataLine);
        if (clickedDataLine != -1) selectedDataLine = clickedDataLine;

        // Render Details Pane
        DrawRectangleRec(detailsPane, Color{40, 44, 52, 255});
        DrawTextEx(font, "Details", (Vector2){detailsPane.x + 8, detailsPane.y - 20}, (float)fontSize, 1, LIGHTGRAY);
        if (selectedCodeLine != -1 && selectedCodeLine < decoderOutputLines.size()) {
            DrawTextEx(font, ("Selected Code: " + decoderOutputLines[selectedCodeLine]).c_str(), (Vector2){detailsPane.x + 8, detailsPane.y + 8}, (float)fontSize, 1, RAYWHITE);
        } else if (selectedDataLine != -1 && selectedDataLine < dataSegmentLines.size()) {
            DrawTextEx(font, ("Selected Data: " + dataSegmentLines[selectedDataLine]).c_str(), (Vector2){detailsPane.x + 8, detailsPane.y + 8}, (float)fontSize, 1, RAYWHITE);
        } else {
            DrawTextEx(font, "No selection", (Vector2){detailsPane.x + 8, detailsPane.y + 8}, (float)fontSize, 1, LIGHTGRAY);
        }

        EndDrawing();
    }

    // --- Cleanup ---
    CloseWindow();
    UnloadFont(font);
    UnloadFont(headerFont);

    return 0;
}
