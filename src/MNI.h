#ifndef MNI_H
#define MNI_H

#include <string>
#include <vector>
#include "microasm_interpreter.h"

// MNI registry and registration function
typedef std::function<void(Interpreter&, const std::vector<BytecodeOperand>&)> MniFunctionType;
extern std::map<std::string, MniFunctionType> mniRegistry;
void registerMNI(const std::string& module, const std::string& name, MniFunctionType func);

// MNI call stack and call function
void callMNI(Interpreter& machine, const std::string& name, const std::vector<BytecodeOperand>& args);

#endif // MNI_H
