#include <iostream>
#include <fstream>
#include <list>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdint>
#include <map>
#include <functional>
#include <set>
#include <vector>
#include <iomanip>
#include <cstring>
#include <math.h>
#ifdef _WIN32
#include <libloaderapi.h>
#else
#include <limits.h> // For PATH_MAX
#include <sys/stat.h> // For stat()
#endif
#ifndef _WIN32
#include <unistd.h>
#endif
// Filesystem includes
#if __has_include(<filesystem>)
    #include <filesystem>
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #error "Neither <filesystem> nor <experimental/filesystem> is available."
#endif
#ifdef _WIN32
namespace fs = std::experimental::filesystem;
#endif
#ifndef _WIN32
namespace fs = std::filesystem;
#endif

// Include own header FIRST
#include "microasm_compiler.h"

// Make readFileLines static to limit scope to this file
static std::vector<std::string> readFileLines(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

// --- Compiler Method Definitions ---

int power_10_lookup[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

std::unordered_map<char, struct binary_search> token_search = {
    {'R', {'R', false, {(MathOperatorTokenType)3, 0}, {
        {'A', {'A', false, {(MathOperatorTokenType)3, 0}, {
                {'X', {'X', true, {(MathOperatorTokenType)1, 0}, {}}},
    
        }}},
        {'B', {'B', false, {(MathOperatorTokenType)3, 0}, {
                {'X', {'X', true, {(MathOperatorTokenType)1, 1}, {}}},
            {'P', {'P', true, {(MathOperatorTokenType)1, 6}, {}}},
    
        }}},
        {'C', {'C', false, {(MathOperatorTokenType)3, 0}, {
                {'X', {'X', true, {(MathOperatorTokenType)1, 2}, {}}},
    
        }}},
        {'D', {'D', false, {(MathOperatorTokenType)3, 0}, {
                {'X', {'X', true, {(MathOperatorTokenType)1, 3}, {}}},
            {'I', {'I', true, {(MathOperatorTokenType)1, 5}, {}}},
    
        }}},
        {'S', {'S', false, {(MathOperatorTokenType)3, 0}, {
                {'I', {'I', true, {(MathOperatorTokenType)1, 4}, {}}},
            {'P', {'P', true, {(MathOperatorTokenType)1, 7}, {}}},
    
        }}},
        {'0', {'0', true, {(MathOperatorTokenType)1, 8}, {}}},
        {'1', {'1', false, {(MathOperatorTokenType)3, 0}, {
                {'0', {'0', true, {(MathOperatorTokenType)1, 18}, {}}},
            {'1', {'1', true, {(MathOperatorTokenType)1, 19}, {}}},
            {'2', {'2', true, {(MathOperatorTokenType)1, 20}, {}}},
            {'3', {'3', true, {(MathOperatorTokenType)1, 21}, {}}},
            {'4', {'4', true, {(MathOperatorTokenType)1, 22}, {}}},
            {'5', {'5', true, {(MathOperatorTokenType)1, 23}, {}}},
    
        }}},
        {'2', {'2', true, {(MathOperatorTokenType)1, 10}, {}}},
        {'3', {'3', true, {(MathOperatorTokenType)1, 11}, {}}},
        {'4', {'4', true, {(MathOperatorTokenType)1, 12}, {}}},
        {'5', {'5', true, {(MathOperatorTokenType)1, 13}, {}}},
        {'6', {'6', true, {(MathOperatorTokenType)1, 14}, {}}},
        {'7', {'7', true, {(MathOperatorTokenType)1, 15}, {}}},
        {'8', {'8', true, {(MathOperatorTokenType)1, 16}, {}}},
        {'9', {'9', true, {(MathOperatorTokenType)1, 17}, {}}},
    
    }}},
    {'+', {'+', true, {(MathOperatorTokenType)0, 0}, {}}},
    {'-', {'-', true, {(MathOperatorTokenType)0, 1}, {}}},
    {'*', {'*', true, {(MathOperatorTokenType)0, 2}, {}}},
    {'/', {'/', true, {(MathOperatorTokenType)0, 3}, {}}},
    {'>', {'>', false, {(MathOperatorTokenType)3, 0}, {
        {'>', {'>', true, {(MathOperatorTokenType)0, 5}, {}}},
    
    }}},
    {'<', {'<', false, {(MathOperatorTokenType)3, 0}, {
        {'<', {'<', true, {(MathOperatorTokenType)0, 6}, {}}},
    
    }}},
    {'|', {'|', true, {(MathOperatorTokenType)0, 7}, {}}},
    {'&', {'&', true, {(MathOperatorTokenType)0, 8}, {}}},
    {'^', {'^', true, {(MathOperatorTokenType)0, 9}, {}}},
};

void Compiler::setFlags(bool debug, bool write_dbg) {
    debugMode = debug;
    if (debugMode) std::cout << "[Debug][Compiler] Debug mode enabled.\n";
    write_dbg_data = write_dbg;
    if (debugMode) std::cout << "[Debug][Compiler] write debug data enabled.\n";
}

int getmin(int i)
{
    if(i == (int8_t)(i & 0xFF))
        return 1;
    if(i == (int16_t)(i & 0xFFFF))
        return 2;
    if(i == (int32_t)(i & 0xFFFFFFFF))
        return 4;
    return 8;
}

int reverseDigits(int n) {
    int revNum = 0;
    while (n > 0) {
        revNum = revNum * 10 + n % 10;
        n = n / 10;
    }
    return revNum;
}

MathOperator getMathOperatorTokens(std::string op) {
    MathOperatorOperators o = op_NONE;
    struct MathOperatorToken first = {None, 0};
    struct MathOperatorToken other = {None, 0};
    op = op.substr(2, op.length()-3);
    std::unordered_map<char, struct binary_search> *cur_token = &token_search;
    bool isnum = false;
    int num = 0;
    int numidx = 0;
    for (int i=0;i<op.length();i++) {
        char c = toupper(op[i]);
        if (cur_token->count(c)) {
            struct binary_search *data = &cur_token->at(c);
            if (isnum && (data->final_token.type == Operator || data->further.begin()->second.final_token.type == Operator)) {
                isnum = false;
                numidx = 0;
                num = reverseDigits(num);
                if (first.type == None) {
                    first = {Immediate, num};
                } else if (other.type == None) {
                    other = {Immediate, num};
                } else {
                    throw std::runtime_error("To many values");
                }
                num = 0;
            } else if (isnum) {
                throw std::runtime_error("Syntax Error");
            }
            cur_token = &data->further;
            if (data->end) {
                cur_token = &token_search;
                switch (data->final_token.type)
                {
                    case Register:
                        if (first.type == None) {
                            first = data->final_token;
                        } else if (other.type == None) {
                            other = data->final_token;
                        } else {
                            throw std::runtime_error("To many values");
                        }
                        break;
                    case Operator:
                        if (o == op_NONE) {
                            o = (MathOperatorOperators)data->final_token.val;
                        } else {
                            throw std::runtime_error("To many Operators");
                        }
                    default:
                        break;
                }
            }
        } else if (cur_token == &token_search && abs(c-'5') < 6) {
            isnum = true;
            num += power_10_lookup[numidx]*(c-'0');
            numidx++;
        } else {
            std::string tmp;
            tmp += c;
            throw std::runtime_error(std::string("unknown token ") + tmp + " idx: " + std::to_string(i));
            cur_token = &token_search;
        }
    }
    if (isnum) {
        num = reverseDigits(num);
        if (first.type == None) {
            first = {Immediate, num};
        } else if (other.type == None) {
            other = {Immediate, num};
        } else {
            throw std::runtime_error("To many values");
        }
    }
    MathOperator ret;
    if (first.type == Immediate && other.type == Immediate) {
        ret.can_be_simpler = true;
        switch (o)
        {
            case op_ADD:
                ret.reg = first.val + other.val;
                break;
            case op_SUB:
                ret.reg = first.val - other.val;
                break;
            case op_MUL:
                ret.reg = first.val * other.val;
                break;
            case op_DIV:
                ret.reg = first.val / other.val;
                break;
            case op_LSR:
                ret.reg = first.val >> other.val;
                break;
            case op_LSL:
                ret.reg = first.val << other.val;
                break;
            case op_AND:
                ret.reg = first.val & other.val;
                break;
            case op_OR:
                ret.reg = first.val | other.val;
                break;
            case op_XOR:
                ret.reg = first.val ^ other.val;
                break;
            default:
                throw std::runtime_error("Unknown operand think");
                break;
        }
        return ret;
    } else if (first.type == Register) {
        ret.reg = first.val;
        ret.other = other;
        ret.operand = o;
        ret.can_be_simpler = false;
    } else if (first.type == Immediate) {
        ret.reg = other.val;
        ret.other = first;
        if (o == op_DIV) {o = op_BDIV;} // reverse division. Operators are done (reg)(op)(other) so this makes 5/rax impossible cause we cannot do that. We made bdiv for that. That makes 5/rax -> rax(bdiv)5
        if (o == op_SUB) {o = op_BSUB;} // reverse division. Operators are done (reg)(op)(other) so this makes 5/rax impossible cause we cannot do that. We made bdiv for that. That makes 5/rax -> rax(bdiv)5
        if (o == op_LSL) {o = op_BLSL;} // reverse division. Operators are done (reg)(op)(other) so this makes 5/rax impossible cause we cannot do that. We made bdiv for that. That makes 5/rax -> rax(bdiv)5
        if (o == op_LSR) {o = op_BLSR;} // reverse division. Operators are done (reg)(op)(other) so this makes 5/rax impossible cause we cannot do that. We made bdiv for that. That makes 5/rax -> rax(bdiv)5
        ret.operand = o;
        ret.can_be_simpler = false;
    }
    return ret;
}

int calculateOperandSize(std::string op) {
    std::string op2 = op;
    if (op2[0] == '$') {
        op2.erase(0, 1);
    }
    if (toupper(op2[0]) == 'R') {
        return 1;
    } else if (op2[0] == '#') {
        return 4;
    } else if (op2[0] == '[') {
        MathOperator data = getMathOperatorTokens(op);
        if (data.can_be_simpler) {return getmin(data.reg);}
        if (data.other.type == Register) {
            return 3; // reg, operand, reg
        } else {
            return 2 + getmin(data.other.val);
        }
    } else {
        return getmin(std::stoi(op2));
    }
}

int Compiler::calculateInstructionSize(const Instruction& instr) {
    if (instr.opcode == ENTER && instr.operands.size() == 0) return 3;
    if (instr.opcode == DB || instr.opcode == LBL) return 0; // Pseudo-instructions

    if (instr.opcode == MNI) {
        int size = 1; // Opcode
        size += instr.mniFunctionName.length() + 1; // Name + Null terminator
        for (auto& op : instr.operands) {
            size += calculateOperandSize(op)+1;
        }
        size += 1; // End marker (Type + Value)
        return size;
    } else {
        // Regular instruction size
        int size = 1;
        for (int i=0; i<instr.operands.size(); i++) {
            std::string op = instr.operands[i];
            size += 1 + calculateOperandSize(op);
        }
        return size;
    }
}

std::string Compiler::trim(const std::string& instr) {
    std::string str = instr;
    for (int i=0;i<str.length();i++) {
        if (str[i] == ';') {
            str = str.substr(0, i);
            break;
        }
    }
    // Python str.strip function modified for masm
    int i = 0;
    while (i < str.length() && !(str[i] > 32 && str[i] != 127)) {
        i++;
    }

    int j = str.length();
    do {
        j--;
    } while (j >= i && !(str[i] > 32 && str[i] != 127));
    j++;
    return str.substr(i, j-i);
}

std::string Compiler::resolveIncludePath(const std::string& includePath) {
    fs::path pathObj(includePath);
    fs::path resolvedPath;

    bool isLocal = includePath.find('/') != std::string::npos || includePath.find('\\') != std::string::npos;
    if (isLocal) {
        // Local include: relative to the current file's directory
        resolvedPath = fs::absolute(fs::path(currentFileDir) / pathObj);
    } else {
        // Standard library include: relative to stdLibRoot, dots replaced by separators
        std::string stdPathStr = includePath;
        std::replace(stdPathStr.begin(), stdPathStr.end(), '.', static_cast<char>(fs::path::preferred_separator));
        resolvedPath = fs::absolute(fs::path(stdLibRoot) / stdPathStr);
    }

    // Check for existence with .mas and .masm extensions
    fs::path finalPathMas = resolvedPath;
    finalPathMas.replace_extension(".mas");
    if (fs::exists(finalPathMas) && fs::is_regular_file(finalPathMas)) {
        return finalPathMas.string();
    }

    fs::path finalPathMasm = resolvedPath;
    finalPathMasm.replace_extension(".masm");
    if (fs::exists(finalPathMasm) && fs::is_regular_file(finalPathMasm)) {
        return finalPathMasm.string();
    }

    // --- Fallback: check current working directory ---
    fs::path cwd = fs::current_path();
    fs::path cwdMas = cwd / pathObj;
    cwdMas.replace_extension(".mas");
    if (fs::exists(cwdMas) && fs::is_regular_file(cwdMas)) {
        return cwdMas.string();
    }
    fs::path cwdMasm = cwd / pathObj;
    cwdMasm.replace_extension(".masm");
    if (fs::exists(cwdMasm) && fs::is_regular_file(cwdMasm)) {
        return cwdMasm.string();
    }
    // --- End fallback ---

    // --- Additional fallback: check next to the executable ---
    #if defined(_WIN32)
        
        char exePath[500];
        GetModuleFileNameA(NULL, exePath, sizeof(exePath));
        fs::path exeDir = fs::path(exePath).parent_path();
    #else
        char exePath[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", exePath, PATH_MAX);
        fs::path exeDir = (count != -1) ? fs::path(std::string(exePath, count)).parent_path() : fs::path();
    #endif

    fs::path exeMas, exeMasm; // Declare these variables here
    if (!exeDir.empty()) {
        // Handle standard library includes relative to the executable directory
        fs::path exeStdLibPath = exeDir / stdLibRoot;
        std::string stdPathStr = includePath;
        std::replace(stdPathStr.begin(), stdPathStr.end(), '.', static_cast<char>(fs::path::preferred_separator));
        fs::path stdLibResolvedPath = exeStdLibPath / stdPathStr;

        exeMas = stdLibResolvedPath;
        exeMas.replace_extension(".mas");
        if (fs::exists(exeMas) && fs::is_regular_file(exeMas)) {
            return exeMas.string();
        }

        exeMasm = stdLibResolvedPath;
        exeMasm.replace_extension(".masm");
        if (fs::exists(exeMasm) && fs::is_regular_file(exeMasm)) {
            return exeMasm.string();
        }

        // Debug output for resolved paths
        if (debugMode) {
            std::cout << "[Debug][Compiler] Checked executable stdlib paths: " << exeMas << ", " << exeMasm << "\n";
        }
    }
    // --- End additional fallback ---

    throw std::runtime_error("Include file not found: " + includePath + " (tried " + finalPathMas.string() + ", " + finalPathMasm.string() + ", " + cwdMas.string() + ", " + cwdMasm.string() + ", " + exeMas.string() + ", " + exeMasm.string() + ")");
}

void Compiler::parseFile(const std::string& filePath) {
    fs::path absPath = fs::absolute(filePath);
    std::string absPathStr = absPath.string();

    // Include guard
    if (includedFiles.count(absPathStr)) {
        // std::cout << "Skipping already included file: " << absPathStr << std::endl; // Optional debug info
        return;
    }
    includedFiles.insert(absPathStr);
    // std::cout << "Parsing file: " << absPathStr << std::endl; // Optional debug info

    // Save current context
    std::string previousFilePath = currentFilePath;
    std::string previousFileDir = currentFileDir;

    // Set new context
    currentFilePath = absPathStr;
    currentFileDir = absPath.parent_path().string();

    try {
        std::vector<std::string> lines = readFileLines(currentFilePath);
        int lineNumber = 0; // Track line number
        for (const std::string& line : lines) {
            lineNumber++;
            try {
                parseLine(line, lineNumber); // Pass line number to parseLine
            } catch (const std::exception& e) {
                throw std::runtime_error("Error in file '" + absPathStr + "' at line " + std::to_string(lineNumber) + ": " + e.what());
            }
        }
    } catch (const std::exception& e) {
        // Restore context before re-throwing to provide better error location
        currentFilePath = previousFilePath;
        currentFileDir = previousFileDir;
        throw std::runtime_error("Error in file '" + absPathStr + "': " + e.what());
    }

    // Restore previous context
    currentFilePath = previousFilePath;
    currentFileDir = previousFileDir;
}

void Compiler::parse(const std::string& source) {
    std::istringstream stream(source);
    std::string line;
    int lineNumber = 0; // Track line number
    while (std::getline(stream, line)) {
        lineNumber++;
        try {
            parseLine(line, lineNumber); // Pass line number to parseLine
        } catch (const std::exception& e) {
            throw std::runtime_error("Error at line " + std::to_string(lineNumber) + ": " + e.what());
        }
    }
}

void Compiler::parseLine(const std::string& line, int lineNumber) {
    std::string trimmedLine = trim(line);
    if (trimmedLine.empty() || trimmedLine[0] == ';') return; // Skip comments and empty lines

    if (debugMode) std::cout << "[Debug][Compiler] Parsing line " << lineNumber << ": " << trimmedLine << "\n";
    std::istringstream stream(trimmedLine);
    std::string token;
    stream >> token;

    // Track column number for error reporting
    int columnNumber = static_cast<int>(trimmedLine.find(token)) + 1;

    // Convert token to uppercase for case-insensitivity for directives/opcodes
    std::string upperToken = token;
    std::transform(upperToken.begin(), upperToken.end(), upperToken.begin(), ::toupper);

    try {
        // --- Handle #include directive ---
        if (token == "#include") { // Use original token case for directive check
            std::string includePathRaw;
            stream >> std::quoted(includePathRaw); // Read path within quotes

            if (includePathRaw.empty()) {
                throw std::runtime_error("Invalid #include directive: Path missing or not quoted.");
            }

            try {
                std::string resolvedPath = resolveIncludePath(includePathRaw);
                if (debugMode) std::cout << "[Debug][Compiler]   Resolved include '" << includePathRaw << "' to '" << resolvedPath << "'\n";
                parseFile(resolvedPath); // Recursively parse the included file
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to process include '" + includePathRaw + "': " + e.what());
            }
            return; // Done processing this line
        }
        // --- End #include handling ---

        if (upperToken == "LBL") {
            std::string label;
            stream >> label;
            if (label.empty()) throw std::runtime_error("Label name missing");
            labelMap["#" + label] = currentAddress; // Store labels with # prefix
            if (debugMode) std::cout << "[Debug][Compiler]   Defined label '" << label << "' at address " << currentAddress << "\n";
        } else if (upperToken == "DB") {
            // Simplified DB handling: Assume address is a label like $1, $2 etc.
            // and string follows. Compiler needs to manage this data.
            // For now, we'll just note it requires more work.
            // Example: DB $1 "Hello"
            std::string dataLabel, dataValue;
            stream >> dataLabel; // e.g., $1
            std::getline(stream >> std::ws, dataValue); // Read rest of line as string

            // Trim trailing whitespace from dataValue
            dataValue.erase(std::find_if_not(dataValue.rbegin(), dataValue.rend(), [](unsigned char ch) {
                return std::isspace(ch);
            }).base(), dataValue.end());

            // Now check for quotes on the potentially trimmed string
            if (dataValue.length() >= 2 && dataValue.front() == '"' && dataValue.back() == '"') {
                dataValue = dataValue.substr(1, dataValue.length() - 2); // Remove quotes
            } else {
                // If it still fails, throw the error
                throw std::runtime_error("DB requires a quoted string (check quotes and content): [" + dataValue + "]");
            }
            // Handle escape sequences like \n (basic implementation)
            std::string processedValue;
            for (size_t i = 0; i < dataValue.length(); ++i) {
            if (dataValue[i] == '\\' && i + 1 < dataValue.length()) {
            switch (dataValue[i+1]) {
            case 'n': processedValue += '\n'; i++; break;
            case 't': processedValue += '\t'; i++; break;
            case '\\': processedValue += '\\'; i++; break;
            case '"': processedValue += '"'; i++; break;
    
                        default: processedValue += dataValue[i]; break; // Keep backslash if not a known escape
                        }
                        } else {
                        processedValue += dataValue[i];
                        }
                        }
                        std::string addr = dataLabel;
                        addr.erase(0, 1);
                        int addre = std::stoi(addr);
                        int size = processedValue.length() + 1;
                        dataSegment.push_back(addre & 0xFF);
                        dataSegment.push_back(addre >> 8);
                        dataSegment.push_back(size & 0xFF);
                        dataSegment.push_back(size >> 8);
                        for(char c : processedValue) {
                        dataSegment.push_back(c);
                        }
            dataSegment.push_back('\0'); // Null-terminate for convenience
            dataAddress += processedValue.length() + 1;
            if (debugMode) std::cout << "[Debug][Compiler]   Defined data label '" << dataLabel << " with value \"" << processedValue << "\"\n";

        } else if (upperToken == "MNI") {
            Instruction instr;
            instr.opcode = MNI;
            stream >> instr.mniFunctionName; // Read Module.Function name
            if (instr.mniFunctionName.empty()) {
                throw std::runtime_error("MNI instruction requires a function name (e.g., Module.Function)");
            }
            // Validate name format roughly (contains '.')
            if (instr.mniFunctionName.find('.') == std::string::npos) {
                throw std::runtime_error("Invalid MNI function name format: " + instr.mniFunctionName + " (expected Module.Function)");
            }

            std::string operand;
            while (stream >> operand) {
                instr.operands.push_back(operand);
            }
            instructions.push_back(instr);
            currentAddress += calculateInstructionSize(instr); // Use helper for size calculation
            if (debugMode) std::cout << "[Debug][Compiler]   Parsed MNI instruction: " << instr.mniFunctionName << " with " << instr.operands.size() << " operands. New address: " << currentAddress << "\n";
        } else {
            Instruction instr;
            instr.opcode = getOpcode(upperToken);
            std::string operand;
            std::string tmp;
            while (stream >> operand) {
                if (operand.find("[") != std::string::npos) {
                    while (operand.find("]") == std::string::npos) {
                        stream >> tmp;
                        operand += tmp;
                    }
                }
                instr.operands.push_back(operand);
            }
            instructions.push_back(instr);
            currentAddress += calculateInstructionSize(instr);
            if (debugMode) std::cout << "[Debug][Compiler]   Parsed instruction: " << upperToken
                                    << " with " << instr.operands.size() << " operands. New address: "
                                    << currentAddress << "\n";
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Error at line " + std::to_string(lineNumber) + ", column " + std::to_string(columnNumber) + ": " + e.what());
    }
}

Opcode Compiler::getOpcode(const std::string& mnemonic) {
     // Case-insensitive lookup
    std::string upperMnemonic = mnemonic;
    std::transform(upperMnemonic.begin(), upperMnemonic.end(), upperMnemonic.begin(), ::toupper);

    // Map definition remains, but the enum itself is now in common_defs.h
    static std::unordered_map<std::string, Opcode> opcodeMap = {
        {"MOV", MOV}, {"ADD", ADD}, {"SUB", SUB}, {"MUL", MUL}, {"DIV", DIV}, {"INC", INC},
        {"JMP", JMP}, {"CMP", CMP}, {"JE", JE}, {"JL", JL}, {"CALL", CALL}, {"RET", RET},
        {"PUSH", PUSH}, {"POP", POP},
        {"OUT", OUT}, {"COUT", COUT}, {"OUTSTR", OUTSTR}, {"OUTCHAR", OUTCHAR},
        {"HLT", HLT}, {"ARGC", ARGC}, {"GETARG", GETARG},
        {"DB", DB},
        {"AND", AND}, {"OR", OR}, {"XOR", XOR}, {"NOT", NOT}, {"SHL", SHL}, {"SHR", SHR},
        {"MOVADDR", MOVADDR}, {"MOVTO", MOVTO},
        {"JNE", JNE}, {"JG", JG}, {"JLE", JLE}, {"JGE", JGE},
        {"ENTER", ENTER}, {"LEAVE", LEAVE},
        {"COPY", COPY}, {"FILL", FILL}, {"CMP_MEM", CMP_MEM},
        {"MALLOC", MALLOC}, {"FREE", FREE},
        {"MNI", MNI},
        {"IN", IN},
        {"MOVB", MOVB}
    };
    return opcodeMap.at(upperMnemonic);
}

void Compiler::compile(const std::string& outputFile) {
    std::ofstream out(outputFile, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot open output file: " + outputFile);

    if (debugMode) std::cout << "[Debug][Compiler] Starting compilation to " << outputFile << "\n";

    // Calculate actual code size using the helper
    uint32_t actualCodeSize = 0;
    for (const auto& instr : instructions) {
        actualCodeSize += calculateInstructionSize(instr);
    }

    // --- Check for main label and set entry point ---
    uint32_t entryPointAddress = 0;
    std::string mainLabelName = "#main"; // The label name we expect
    if (labelMap.count(mainLabelName)) {
        entryPointAddress = labelMap.at(mainLabelName);
    } else {
        throw std::runtime_error("Compilation failed: Entry point label '#main' not found.");
    }
    // --- End check ---

    // Prepare the header
    BinaryHeader header;
    header.magic = 0x4D53414D; // "MASM"
    header.version = VERSION;
    header.codeSize = actualCodeSize;
    header.dataSize = dataSegment.size();
    header.dbgSize = 0;
    header.entryPoint = entryPointAddress; // Set the found entry point
    if (write_dbg_data) {
        for (auto& p : labelMap) {
            header.dbgSize += p.first.size()+1;
            header.dbgSize += sizeof(p.second);
        }
    }

    // Write the header
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write code segment
    if (debugMode) std::cout << "[Debug][Compiler] Writing code segment (" << header.codeSize << " bytes)...\n";
    int byteOffset = 0; // Track offset for debug output
    for (const auto& instr : instructions) {
        if (instr.opcode == DB || instr.opcode == LBL) continue; // Skip pseudo-instructions

        // THIS LINE IS CRITICAL:
        out.put(static_cast<char>(instr.opcode)); // Write Opcode (1 byte)
        byteOffset += 1;

        if (instr.opcode == MNI) {
            // Write null-terminated function name
            if (debugMode) std::cout << "[Debug][Compiler]     MNI Name: " << instr.mniFunctionName << "\n";
            out.write(instr.mniFunctionName.c_str(), instr.mniFunctionName.length());
            out.put('\0');
            byteOffset += instr.mniFunctionName.length() + 1;

            // Write operands as [type][value] pairs
            for (const auto& operand : instr.operands) {
                ResolvedOperand resolved = resolveOperand(operand, instr.opcode);
                if (debugMode) std::cout << "[Debug][Compiler]     Operand Type: 0x" << std::hex << static_cast<int>(resolved.type) << ", Value: " << std::dec << resolved.value << " (0x" << std::hex << resolved.value << std::dec << ")\n";
                int value_size = calculateOperandSize(operand);
                out.put(static_cast<char>(resolved.type) | (value_size << 4));
                const char * value = reinterpret_cast<const char*>(&resolved.value);
                for (int i=0; i<value_size; i++) {
                    out.put(value[i]);
                }
                byteOffset += 1 + value_size;
            }
            // Write end marker: type=NONE, value=0
            out.put(static_cast<char>(OperandType::NONE));
            byteOffset += 1;

        } else {
            // Write regular operands
            for (const auto& operand : instr.operands) {
                ResolvedOperand resolved = resolveOperand(operand, instr.opcode);
                if (debugMode) std::cout << "[Debug][Compiler]     Operand Type: 0x" << std::hex << static_cast<int>(resolved.type) << ", Value: " << std::dec << resolved.value << " (0x" << std::hex << resolved.value << std::dec << ")\n";
                int value_size = calculateOperandSize(operand);

                out.put(static_cast<char>(resolved.type) | ((value_size << 4) * -1 * resolved.size));
                const char * value = reinterpret_cast<const char*>(&resolved.value);
                for (int i=0; i<value_size; i++) {
                    out.put(value[i]);
                }
                byteOffset += 1 + value_size;
            }
            if (instr.opcode == ENTER && instr.operands.size() == 0) {
                if (debugMode) std::cout << "[Debug][Compiler]     Putting zero in ENTER";
                out.put(static_cast<char>((int)OperandType::IMMEDIATE | 0x10));
                out.put(0);
                byteOffset += 2;
            }
        }
    }

    // Write data segment
    if (!dataSegment.empty()) {
        if (debugMode) std::cout << "[Debug][Compiler] Writing data segment (" << header.dataSize << " bytes)...\n";
        out.write(dataSegment.data(), dataSegment.size());
    }
    if (write_dbg_data) {
        for (const auto& pair : labelMap) {
            std::string lbl = pair.first;
            int addr = pair.second;
            out.write(lbl.c_str(), lbl.length() + 1);
            out.write(reinterpret_cast<const char*>(&addr), sizeof(addr));
        }
    }
    if (debugMode) std::cout << "[Debug][Compiler] Compilation finished.\n";
    // Note: Interpreter needs to read the header to know segment sizes and entry point.
}

ResolvedOperand Compiler::resolveOperand(const std::string& operand, Opcode contextOpcode) {
    ResolvedOperand result;
    if (operand.empty()) throw std::runtime_error("Empty operand encountered");

    try {
        if (operand[0] == '#') { // Label address (for jumps/calls)
            if (labelMap.count(operand)) {
                result.type = OperandType::LABEL_ADDRESS;
                result.value = labelMap.at(operand);
            } else {
                throw std::runtime_error("Undefined label: " + operand);
            }
        } else if (operand[0] == '$') {
            if (operand.length() > 1 && toupper(operand[1]) == 'R') { // Potential register address like $R1 or $RBX
                std::string regName = operand.substr(1); // Extract potential register name (e.g., R1, RBX)
                std::string upperRegName = regName;
                std::transform(upperRegName.begin(), upperRegName.end(), upperRegName.begin(), ::toupper);

                static const std::unordered_map<std::string, int> regMap = {
                    {"RAX", 0}, {"RBX", 1}, {"RCX", 2}, {"RDX", 3},
                    {"RSI", 4}, {"RDI", 5}, {"RBP", 6}, {"RSP", 7},
                    {"R0", 8}, {"R1", 9}, {"R2", 10}, {"R3", 11},
                    {"R4", 12}, {"R5", 13}, {"R6", 14}, {"R7", 15},
                    {"R8", 16}, {"R9", 17}, {"R10", 18}, {"R11", 19},
                    {"R12", 20}, {"R13", 21}, {"R14", 22}, {"R15", 23}
                };

                if (regMap.count(upperRegName)) {
                    int regIndex = regMap.at(upperRegName);
                    result.type = OperandType::REGISTER_AS_ADDRESS; // Correctly mark as register-based memory address
                    result.value = regIndex;
                } else {
                    throw std::runtime_error("Unknown register format for $ operand: " + operand);
                }
            } else if (operand.length() > 1 && toupper(operand[1]) == '[') { // square brackets like $[rax + 4]
                std::string operand_short = operand.substr(2, operand.length()-3);
                MathOperator data = getMathOperatorTokens(operand);
                result.type = OperandType::MATH_OPERATOR;
                if (data.can_be_simpler) {result.type = OperandType::DATA_ADDRESS; result.value = data.reg;} else {
                result.value = data.reg + (data.operand << 8) + (data.other.val << 16);
                if (data.other.type == Register) {
                    result.size = 0;
                }}
            } else {
                bool isNumber = true;
                std::string numStr = operand.substr(1);
                for (char c : numStr) {
                    if (!std::isdigit(c) && c != '-' && c != '+') { isNumber = false; break; }
                }
                if (isNumber) {
                    long long val = std::stoll(numStr);
                    if (val < 0 || val > INT_MAX) {
                        throw std::runtime_error("DATA_ADDRESS ($<number>) out of range: " + operand);
                    }
                    result.type = OperandType::DATA_ADDRESS;
                    result.value = val;
                } else {
                    // Try parsing as an immediate number (e.g., $500)
                    try {
                        long long val = std::stoll(numStr);
                        if (val < INT_MIN || val > INT_MAX) {
                            throw std::runtime_error("Immediate value ($) out of 32-bit range: " + operand);
                        }
                        result.type = OperandType::IMMEDIATE;
                        result.value = val;
                    } catch (...) { // Catch invalid_argument, out_of_range
                        throw std::runtime_error("Invalid immediate value or undefined data/register label starting with $: " + operand);
                    }
                }
            }
        } else if (toupper(operand[0]) == 'R') { // Register
            std::string regName = operand;
            std::transform(regName.begin(), regName.end(), regName.begin(), ::toupper);

            static const std::unordered_map<std::string, int> regMap = {
                {"RAX", 0}, {"RBX", 1}, {"RCX", 2}, {"RDX", 3},
                {"RSI", 4}, {"RDI", 5}, {"RBP", 6}, {"RSP", 7},
                {"RIP", -1}, // Special case, usually not directly settable
                {"R0", 8}, {"R1", 9}, {"R2", 10}, {"R3", 11},
                {"R4", 12}, {"R5", 13}, {"R6", 14}, {"R7", 15},
                {"R8", 16}, {"R9", 17}, {"R10", 18}, {"R11", 19},
                {"R12", 20}, {"R13", 21}, {"R14", 22}, {"R15", 23}
            };
            if (regMap.count(regName)) {
                int regIndex = regMap.at(regName);
                if (regIndex == -1) throw std::runtime_error("Cannot directly use RIP as operand");
                result.type = OperandType::REGISTER;
                result.value = regIndex;
            } else {
                try {
                     // Check R<num> format
                    if (regName.length() > 1 && std::isdigit(regName[1])) {
                        int regIndexNum = std::stoi(regName.substr(1));
                        if (regIndexNum >= 0 && regIndexNum <= 15) {
                            result.type = OperandType::REGISTER;
                            result.value = regIndexNum + 8; // Map R0-R15 to indices 8-23
                        } else {
                            throw std::runtime_error("Register index out of range (R0-R15): " + operand);
                        }
                    } else {
                        throw std::runtime_error("Unknown register format: " + operand);
                    }
                } catch (...) {
                    throw std::runtime_error("Unknown or invalid register: " + operand);
                }
            }
        } else { // Immediate value (not starting with $ or # or R)
            try {
                long long val = std::stoll(operand);
                if (val < INT_MIN || val > INT_MAX) {
                    throw std::runtime_error("Immediate value out of 32-bit range: " + operand);
                }
                result.type = OperandType::IMMEDIATE;
                result.value = static_cast<int>(val);
            } catch (...) { // Catch invalid_argument, out_of_range
                throw std::runtime_error("Invalid immediate value or unknown operand: " + operand);
            }
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to resolve operand '" + operand + "': " + e.what());
    }

    if (debugMode) {
        std::cout << "[Debug][Compiler]   Resolving operand '" << operand << "' -> Type: 0x" << std::hex << static_cast<int>(result.type) << ", Value: " << std::dec << result.value << " (0x" << std::hex << result.value << std::dec << ")\n";
    }
    return result;
}

// --- Standalone Compiler Main Function Definition ---

int microasm_compiler_main(int argc, char* argv[]) {
    // --- Argument Parsing for Standalone Compiler ---
    std::string sourceFile;
    std::string outputFile;
    bool enableDebug = false;
    bool write_dbg_data = false;
    std::vector<char*> filtered_args; // Store non-debug args for potential future use

    // argv[0] here is the *first argument* after "-c", not the program name
    for (int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--debug") {
            enableDebug = true;
        } else if (arg == "-g" || arg == "--dbg_data") {
            std::cout << "WARNING: Debug data being written to file" << std::endl;
            write_dbg_data = true;
        } else if (sourceFile.empty()) {
            sourceFile = arg;
            filtered_args.push_back(argv[i]);
        } else if (outputFile.empty()) {
            outputFile = arg;
            filtered_args.push_back(argv[i]);
        } else {
            // Handle extra arguments if necessary, or ignore/error
            filtered_args.push_back(argv[i]);
        }
    }

    if (sourceFile.empty() || outputFile.empty()) {
        std::cerr << "Compiler Usage: <source.masm> <output.bin> [debug.masmd] [-d|--debug]" << std::endl;
        return 1;
    }
    // --- End Argument Parsing ---


    try {
        std::ifstream fileStream(sourceFile);
        if (!fileStream) {
            throw std::runtime_error("Could not open source file: " + sourceFile);
        }
        std::ostringstream buffer;
        buffer << fileStream.rdbuf();
        fileStream.close();

        Compiler compiler;
        compiler.setFlags(enableDebug, write_dbg_data); // Set debug mode
        compiler.parse(buffer.str());       // Parse content
        compiler.compile(outputFile);       // Compile to output

        std::cout << "Compilation successful: " << sourceFile << " -> " << outputFile << std::endl;
    } catch (const std::exception& e) {
        
        std::cerr << "Compilation Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}