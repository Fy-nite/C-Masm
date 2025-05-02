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

enum MathOperatorTokenType {
    Operator, Register, Immediate
};

enum MathOperatorOperators {
    op_ADD,
    op_SUB,
    op_MUL,
    op_DIV,
    op_BDIV, // backward div
    op_LSR,
    op_LSL,
    op_AND,
    op_OR,
    op_XOR
};


struct MathOperatorToken {
    MathOperatorTokenType type;
    int val;
};
struct binary_search {
    char value;
    bool end;
    struct MathOperatorToken final_token;
    std::unordered_map<char, struct binary_search> further;
};


std::unordered_map<char, struct binary_search> token_search = {
    {'R', {'R', false, "R", {
        {'A', {'A', false, "RA", {
            {'X', {'X', true, "RAX", {
          
      }}},
  
    }}},
    {'B', {'B', false, "RB", {
            {'X', {'X', true, "RBX", {
          
      }}},
      {'P', {'P', true, "RBP", {
          
      }}},
  
    }}},
    {'C', {'C', false, "RC", {
            {'X', {'X', true, "RCX", {
          
      }}},
  
    }}},
    {'D', {'D', false, "RD", {
            {'X', {'X', true, "RDX", {
          
      }}},
      {'I', {'I', true, "RDI", {
          
      }}},
  
    }}},
    {'S', {'S', false, "RS", {
            {'I', {'I', true, "RSI", {
          
      }}},
      {'P', {'P', true, "RSP", {
          
      }}},
  
    }}},
    {'0', {'0', true, "R0", {
        
    }}},
    {'1', {'1', false, "R1", {
            {'0', {'0', true, "R10", {
          
      }}},
      {'1', {'1', true, "R11", {
          
      }}},
      {'2', {'2', true, "R12", {
          
      }}},
      {'3', {'3', true, "R13", {
          
      }}},
      {'4', {'4', true, "R14", {
          
      }}},
      {'5', {'5', true, "R15", {
          
      }}},
  
    }}},
    {'2', {'2', true, "R2", {
        
    }}},
    {'3', {'3', true, "R3", {
        
    }}},
    {'4', {'4', true, "R4", {
        
    }}},
    {'5', {'5', true, "R5", {
        
    }}},
    {'6', {'6', true, "R6", {
        
    }}},
    {'7', {'7', true, "R7", {
        
    }}},
    {'8', {'8', true, "R8", {
        
    }}},
    {'9', {'9', true, "R9", {
        
    }}},
  
  }}},
  {'+', {'+', true, "+", {
      
  }}},
  {'-', {'-', true, "-", {
      
  }}},
  {'*', {'*', true, "*", {
      
  }}},
  {'/', {'/', true, "/", {
      
  }}},
  {'>', {'>', false, ">", {
        {'>', {'>', true, ">>", {
        
    }}},
  
  }}},
  {'<', {'<', false, "<", {
        {'<', {'<', true, "<<", {
        
    }}},
  
  }}},
  {'|', {'|', true, "|", {
      
  }}},
  {'&', {'&', true, "&", {
      
  }}},
  {'^', {'^', true, "^", {
      
  }}},
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
    bool write_dbg_data = true;

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
    void setFlags(bool debug=false, bool write_dbg=false);
    void parse(const std::string& source);
    void compile(const std::string& outputFile);
};

// Declare the standalone main function for the compiler
int microasm_compiler_main(int argc, char* argv[]);

#endif // MICROASM_COMPILER_H
