#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include <cstdint> // Required for uint types

// --- API Export/Import Macros ---
#ifdef _WIN32
    #ifdef MASM_DLL_EXPORT
        #define MASM_API __declspec(dllexport) // Export for DLL
    #else
        #define MASM_API __declspec(dllimport) // Import for client
    #endif
#else
    #define MASM_API __attribute__((visibility("default"))) // GCC/Clang
#endif
// --- End API Macros ---

// Define a simple binary header structure
struct BinaryHeader {
    uint32_t magic = 0x4D53414D; // "MASM" in ASCII (little-endian)
    uint16_t version = 1;
    uint16_t reserved = 0; // Padding/Reserved for future use
    uint32_t codeSize = 0;
    uint32_t dataSize = 0;
    uint32_t entryPoint = 0; // Offset within the code segment
};

// Opcode Enum
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
    // Data Definition
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
    COPY, FILL, CMP_MEM,
    // Module Native Interface call
    MNI,
    // Pseudo-instructions (handled during compilation, not runtime)
    INCLUDE = 0xF2, // Placeholder for include directive logic (handled pre-compilation)
    IN
};

#endif // COMMON_DEFS_H
