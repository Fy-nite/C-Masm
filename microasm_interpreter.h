#ifndef MICROASM_INTERPRETER_H
#define MICROASM_INTERPRETER_H

#include <string>
#include <vector>
#include <stack>
#include <map>
#include <functional>
#include <cstdint> // Required for uint8_t
#include "common_defs.h"   // Include common definitions (Opcode, BinaryHeader)
#include "operand_types.h" // Include operand types

// Structure to hold operand info read from bytecode
struct BytecodeOperand {
    OperandType type;
    int value;
};

// Forward declaration
class Interpreter;

// Define the type for MNI functions
using MniFunctionType = std::function<void(Interpreter&, const std::vector<BytecodeOperand>&)>;

// Declare the global registry for MNI functions as extern
extern std::map<std::string, MniFunctionType> mniRegistry;

// Declare the function to register MNI functions
void registerMNI(const std::string& module, const std::string& name, MniFunctionType func);

class Interpreter {
public: // Public members needed by MNI or main
    std::vector<int> registers;
    std::vector<char> ram;

private: // Private members
    std::vector<uint8_t> bytecode_raw;
    int dataSegmentBase;
    int ip = 0;
    int sp;
    int bp;
    bool zeroFlag = false;
    bool signFlag = false;
    std::vector<std::string> cmdArgs;
    bool debugMode = false;

    // Private methods
    BytecodeOperand nextRawOperand();
    int getRegisterIndex(const BytecodeOperand& operand);
    void pushStack(int value);
    int popStack();
    std::string readBytecodeString();
    void initializeMNIFunctions();
    std::string formatOperandDebug(const BytecodeOperand& op);

public: // Public methods including memory access for MNI
    // Constructor
    Interpreter(int ramSize = 65536, const std::vector<std::string>& args = {}, bool debug = false);

    // Memory access helpers
    int readRamInt(int address);
    void writeRamInt(int address, int value);
    char readRamChar(int address);
    void writeRamChar(int address, char value);
    std::string readRamString(int address);

    // Main operations
    void load(const std::string& bytecodeFile);
    void execute();

    // Public helper needed by MNI and internal logic
    int getValue(const BytecodeOperand& operand);
};

// Declare the standalone main function for the interpreter
int microasm_interpreter_main(int argc, char* argv[]);

#endif // MICROASM_INTERPRETER_H
