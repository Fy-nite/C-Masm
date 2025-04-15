enum Opcode {
    MOV, ADD, SUB, MUL, DIV, INC,
    JMP, CMP, JE, JL, CALL, RET,
    PUSH, POP,
    OUT, COUT, OUTSTR, OUTCHAR,
    HLT, ARGC, GETARG,
    DB,
    AND, OR, XOR, NOT, SHL, SHR,
    MOVADDR, MOVTO,
    JNE, JG, JLE, JGE,
    ENTER, LEAVE,
    COPY, FILL, CMP_MEM,
    MNI,
    IN // Read from stdin to memory
};