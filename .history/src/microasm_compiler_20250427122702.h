#ifndef MICROASM_COMPILER_H
#define MICROASM_COMPILER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include "common_defs.h"   // Include common definitions (Opcode, BinaryHeader)
#include "operand_types.h" // Include operand types

#define VERSION 2

// Define Instruction struct here
struct Instruction {
    Opcode opcode;
    std::vector<std::string> operands;
    std::string mniFunctionName; // Store MNI function name if opcode is MNI
};

// Define ResolvedOperand struct here
struct ResolvedOperand {
    OperandType type = OperandType::NONE;
    int value = 0;
};

// Helper function declaration (if it needs to be public, otherwise keep static in .cpp)
// std::vector<std::string> readFileLines(const std::string& filePath);

class Compiler {
    std::unordered_map<std::string, int> labelMap;
    std::vector<Instruction> instructions; // Now knows what Instruction is
    std::vector<char> dataSegment;
    int currentAddress = 0;
    int dataAddress = 0;
    bool debugMode = false;

    // Include directive handling
    std::set<std::string> includedFiles;
    std::string currentFilePath;
    std::string currentFileDir;
    std::string stdLibRoot = "./stdlib";

    // Private methods
    int calculateInstructionSize(const Instruction& instr); // Now knows what Instruction is
    std::string trim(const std::string& str);
    std::string resolveIncludePath(const std::string& includePath);
    void parseFile(const std::string& filePath);
    Opcode getOpcode(const std::string& mnemonic);
    ResolvedOperand resolveOperand(const std::string& operand, Opcode contextOpcode = (Opcode)0); // Now knows what ResolvedOperand is
    void parseLine(const std::string& line, int lineNumber); // Updated to accept lineNumber

public:
    void setDebugMode(bool enabled);
    void parse(const std::string& source, const std::string& debug_file);
    void compile(const std::string& outputFile);
};

// Declare the standalone main function for the compiler
int microasm_compiler_main(int argc, char* argv[]);

#endif // MICROASM_COMPILER_H
