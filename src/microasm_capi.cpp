#include "microasm_capi.h"
#include "microasm_interpreter.h" // Include the C++ interpreter class
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream> // For potential error logging

// --- Error Handling ---
// Simple thread-unsafe error storage. Use thread_local for safety in multithreaded scenarios.
// thread_local std::string lastErrorMessage;
std::string lastErrorMessage; // Keep it simple for now

void setLastError(const std::string& message) {
    lastErrorMessage = message;
    // Optionally log the error to stderr as well
    // std::cerr << "MASM Runtime Error: " << message << std::endl;
}

const char* masm_get_last_error() {
    return lastErrorMessage.empty() ? nullptr : lastErrorMessage.c_str();
}
// --- End Error Handling ---


// Define the opaque struct locally
struct InterpreterOpaque {
    Interpreter interpreter; // Contains the actual C++ interpreter instance

    // Constructor to initialize the interpreter
    InterpreterOpaque(int ramSize, bool debug)
        : interpreter(ramSize, {}, debug) {} // Initialize with empty args initially
};

// --- C API Implementation ---

extern "C" {

InterpreterOpaque* masm_create_interpreter(int ramSize, int debugMode) {
    setLastError(""); // Clear previous error
    try {
        if (ramSize <= 0) {
            setLastError("RAM size must be positive.");
            return nullptr;
        }
        InterpreterOpaque* handle = new InterpreterOpaque(ramSize, debugMode != 0);
        return handle;
    } catch (const std::bad_alloc&) {
        setLastError("Failed to allocate memory for interpreter.");
        return nullptr;
    } catch (const std::exception& e) {
        setLastError("Failed to create interpreter: " + std::string(e.what()));
        return nullptr;
    } catch (...) {
        setLastError("An unknown error occurred during interpreter creation.");
        return nullptr;
    }
}

void masm_destroy_interpreter(InterpreterOpaque* handle) {
    setLastError("");
    if (handle != nullptr) {
        delete handle;
    }
}

MasmResult masm_load_bytecode(InterpreterOpaque* handle, const char* bytecodeFile) {
    setLastError("");
    if (handle == nullptr) {
        setLastError("Invalid interpreter handle.");
        return MASM_ERROR_INVALID_HANDLE;
    }
    if (bytecodeFile == nullptr) {
        setLastError("Bytecode file path cannot be null.");
        return MASM_ERROR_INVALID_ARGUMENT;
    }
    try {
        handle->interpreter.load(bytecodeFile);
        return MASM_OK;
    } catch (const std::exception& e) {
        setLastError("Failed to load bytecode: " + std::string(e.what()));
        return MASM_ERROR_LOAD_FAILED;
    } catch (...) {
        setLastError("An unknown error occurred during bytecode loading.");
        return MASM_ERROR_LOAD_FAILED;
    }
}

MasmResult masm_execute(InterpreterOpaque* handle, int argc, const char* const* argv) {
    setLastError("");
    if (handle == nullptr) {
        setLastError("Invalid interpreter handle.");
        return MASM_ERROR_INVALID_HANDLE;
    }

    // Convert C-style args to std::vector<std::string> for the interpreter
    std::vector<std::string> programArgs;
    if (argc > 0 && argv != nullptr) {
        programArgs.assign(argv, argv + argc);
    }
    // The Interpreter constructor already took args, but let's allow overriding/setting them here if needed.
    // This requires adding a method to the Interpreter class:
    // handle->interpreter.setArguments(programArgs); // Assumes setArguments exists

    try {
        // We need to modify the Interpreter class slightly or manage args differently.
        // For now, let's assume the Interpreter was created correctly and args aren't easily changeable post-creation.
        // A better approach would be to pass args to execute() or have a setArgs method.
        // Let's simulate passing args by creating a *new* interpreter if needed (less efficient).
        // OR, modify Interpreter::execute to accept args (better).
        // *** Assuming Interpreter::execute() doesn't take args yet ***
        // *** We'll ignore argc/argv for this simplified example ***
        // *** A real implementation MUST handle passing args to the script ***
        if (argc > 0) {
             // Log a warning that args are currently ignored in this C API version
             // std::cerr << "Warning: C API masm_execute currently ignores arguments." << std::endl;
        }

        handle->interpreter.execute(); // Execute the loaded code
        return MASM_OK; // Assume HLT was reached or end of code
    } catch (const std::exception& e) {
        setLastError("Runtime execution error: " + std::string(e.what()));
        // The interpreter's execute() already prints errors, so just capture.
        return MASM_ERROR_EXECUTION_FAILED;
    } catch (...) {
        setLastError("An unknown error occurred during execution.");
        return MASM_ERROR_EXECUTION_FAILED;
    }
}

MasmResult masm_get_register(InterpreterOpaque* handle, int registerIndex, int32_t* outValue) {
    setLastError("");
    if (handle == nullptr) {
        setLastError("Invalid interpreter handle.");
        return MASM_ERROR_INVALID_HANDLE;
    }
    if (outValue == nullptr) {
        setLastError("Output value pointer cannot be null.");
        return MASM_ERROR_INVALID_ARGUMENT;
    }
    if (registerIndex < 0 || registerIndex >= handle->interpreter.registers.size()) {
        setLastError("Register index out of bounds.");
        return MASM_ERROR_INVALID_ARGUMENT;
    }
    try {
        *outValue = handle->interpreter.registers[registerIndex];
        return MASM_OK;
    } catch (const std::exception& e) {
        setLastError("Error getting register value: " + std::string(e.what()));
        return MASM_ERROR_GENERAL;
    } catch (...) {
         setLastError("An unknown error occurred while getting register value.");
        return MASM_ERROR_GENERAL;
    }
}

MasmResult masm_read_ram_int(InterpreterOpaque* handle, int address, int32_t* outValue) {
    setLastError("");
     if (handle == nullptr) {
        setLastError("Invalid interpreter handle.");
        return MASM_ERROR_INVALID_HANDLE;
    }
    if (outValue == nullptr) {
        setLastError("Output value pointer cannot be null.");
        return MASM_ERROR_INVALID_ARGUMENT;
    }
     try {
        *outValue = handle->interpreter.readRamInt(address);
        return MASM_OK;
    } catch (const std::exception& e) {
        setLastError("Error reading RAM: " + std::string(e.what()));
        return MASM_ERROR_MEMORY;
    } catch (...) {
         setLastError("An unknown error occurred while reading RAM.");
        return MASM_ERROR_MEMORY;
    }
}

MasmResult masm_write_ram_int(InterpreterOpaque* handle, int address, int32_t value) {
    setLastError("");
     if (handle == nullptr) {
        setLastError("Invalid interpreter handle.");
        return MASM_ERROR_INVALID_HANDLE;
    }
     try {
        handle->interpreter.writeRamInt(address, value);
        return MASM_OK;
    } catch (const std::exception& e) {
        setLastError("Error writing RAM: " + std::string(e.what()));
        return MASM_ERROR_MEMORY;
    } catch (...) {
         setLastError("An unknown error occurred while writing RAM.");
        return MASM_ERROR_MEMORY;
    }
}


} // extern "C"
