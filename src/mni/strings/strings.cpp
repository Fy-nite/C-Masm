#include "../../microasm_interpreter.h"
#include <cstring>
#include <stdexcept>

// Register the MNI function when this file is loaded

    void StringMniRegister() {
        registerMNI("StringOperations", "cmp", [](Interpreter& machine, const std::vector<BytecodeOperand>& args) {
            if (args.size() < 2) throw std::runtime_error("StringOperations.cmp requires 2 arguments (addr1, addr2)");
            int addr1 = machine.getValue(args[0]);
            int addr2 = machine.getValue(args[1]);
            std::string s1 = machine.readRamString(addr1);
            std::string s2 = machine.readRamString(addr2);
            machine.zeroFlag = (s1 == s2);
        });
    }
