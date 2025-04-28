#include "MNI.h"
#include <iostream>
#include <vector>
#include <map>
#include <stdexcept>

std::vector<std::string> mniCallStackInternal;

void callMNI(Interpreter& machine, const std::string& name, const std::vector<BytecodeOperand>& args) {
    mniCallStackInternal.push_back(name);
    bool isOutermost = (mniCallStackInternal.size() == 1);
    if (mniRegistry.count(name)) {
        try {
            mniRegistry[name](machine, args);
        } catch (...) {
            if (isOutermost) {
                std::cerr << "MNI Call Stack (most recent call last):\n";
                for (auto it = mniCallStackInternal.rbegin(); it != mniCallStackInternal.rend(); ++it) {
                    std::cerr << "  at " << *it << std::endl;
                }
            }
            mniCallStackInternal.pop_back();
            throw;
        }
    } else {
        mniCallStackInternal.pop_back();
        throw std::runtime_error("Unregistered MNI function called: " + name);
    }
    mniCallStackInternal.pop_back();
}