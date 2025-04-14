#ifndef OPERAND_TYPES_H
#define OPERAND_TYPES_H

#include <cstdint> // For uint8_t

// Enum to represent the type of an operand in the bytecode
enum class OperandType : uint8_t {
    NONE = 0x00,       // Should not appear in valid bytecode
    REGISTER = 0x01,   // Value is a register index (0-23)
    IMMEDIATE = 0x02,  // Value is a direct integer literal
    LABEL_ADDRESS = 0x03, // Value is a code address (for JMP, CALL, etc.) - resolved by compiler
    DATA_ADDRESS = 0x04,  // Value is an offset/address in the data segment (e.g., from DB $1) - resolved by compiler
    REGISTER_AS_ADDRESS = 0x05 // Register holding a memory address (e.g., $R1)
};

#endif // OPERAND_TYPES_H
