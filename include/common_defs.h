
#include <iostream>

enum Opcode {
    MOV = 0x00,
    ADD = 0x01,
    SUB = 0x02,
    MUL = 0x03,
    DIV = 0x04,
    INC = 0x05,
    JMP = 0x06,
    CMP = 0x07,
    JE = 0x08,
    JL = 0x09,
    CALL = 0x0A,
    RET = 0x0B,
    PUSH = 0x0C,
    POP = 0x0D,
    OP_OUT = 0x0E,
    COUT = 0x0F,
    OUTSTR = 0x10,
    OUTCHAR = 0x11,
    HLT = 0x12,
    ARGC = 0x13,
    GETARG = 0x14,
    DB = 0x15,
    LBL = 0x16,
    AND = 0x17,
    OR = 0x18,
    XOR = 0x19,
    NOT = 0x1A,
    SHL = 0x1B,
    SHR = 0x1C,
    MOVADDR = 0x1D,
    MOVTO = 0x1E,
    JNE = 0x1F,
    JG = 0x20,
    JLE = 0x21,
    JGE = 0x22,
    ENTER = 0x23,
    LEAVE = 0x24,
    COPY = 0x25,
    FILL = 0x26,
    CMP_MEM = 0x27,
    MNI = 0x28,
    OP_IN = 0x29
};

#pragma pack(push, 1)
struct BinaryHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t codeSize;
    uint32_t dataSize;
    uint32_t entryPoint;
};
#pragma pack(pop)