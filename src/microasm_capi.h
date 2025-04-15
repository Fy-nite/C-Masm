#ifndef MICROASM_CAPI_H
#define MICROASM_CAPI_H

#include "common_defs.h" // For MASM_API
#include <stdint.h>      // For standard integer types like int32_t

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to the interpreter instance
typedef struct InterpreterOpaque* MasmInterpreterHandle;

// Error codes
typedef enum {
    MASM_OK = 0,
    MASM_ERROR_GENERAL = -1,
    MASM_ERROR_INVALID_HANDLE = -2,
    MASM_ERROR_LOAD_FAILED = -3,
    MASM_ERROR_EXECUTION_FAILED = -4,
    MASM_ERROR_INVALID_ARGUMENT = -5,
    MASM_ERROR_MEMORY = -6
} MasmResult;

/**
 * @brief Creates a new MicroASM interpreter instance.
 * @param ramSize The size of the RAM for the interpreter in bytes.
 * @param debugMode 1 to enable debug mode, 0 otherwise.
 * @return A handle to the new interpreter, or NULL on failure.
 */
MASM_API MasmInterpreterHandle masm_create_interpreter(int ramSize, int debugMode);

/**
 * @brief Destroys a MicroASM interpreter instance and frees associated resources.
 * @param handle The handle to the interpreter instance.
 */
MASM_API void masm_destroy_interpreter(MasmInterpreterHandle handle);

/**
 * @brief Loads MicroASM bytecode from a file into the interpreter.
 * @param handle The handle to the interpreter instance.
 * @param bytecodeFile Path to the .bin file.
 * @return MASM_OK on success, or an error code on failure.
 */
MASM_API MasmResult masm_load_bytecode(MasmInterpreterHandle handle, const char* bytecodeFile);

/**
 * @brief Executes the loaded MicroASM bytecode.
 * @param handle The handle to the interpreter instance.
 * @param argc Number of command-line arguments for the script.
 * @param argv Array of command-line argument strings for the script.
 * @return MASM_OK on success (HLT encountered), or an error code on failure (runtime error).
 */
MASM_API MasmResult masm_execute(MasmInterpreterHandle handle, int argc, const char* const* argv);

/**
 * @brief Gets the value of a specific register.
 * @param handle The handle to the interpreter instance.
 * @param registerIndex The index of the register (0-23).
 * @param outValue Pointer to store the register value.
 * @return MASM_OK on success, or an error code on failure.
 */
MASM_API MasmResult masm_get_register(MasmInterpreterHandle handle, int registerIndex, int32_t* outValue);

/**
 * @brief Reads an integer from the interpreter's RAM.
 * @param handle The handle to the interpreter instance.
 * @param address The RAM address to read from.
 * @param outValue Pointer to store the read value.
 * @return MASM_OK on success, or an error code on failure.
 */
MASM_API MasmResult masm_read_ram_int(MasmInterpreterHandle handle, int address, int32_t* outValue);

/**
 * @brief Writes an integer to the interpreter's RAM.
 * @param handle The handle to the interpreter instance.
 * @param address The RAM address to write to.
 * @param value The value to write.
 * @return MASM_OK on success, or an error code on failure.
 */
MASM_API MasmResult masm_write_ram_int(MasmInterpreterHandle handle, int address, int32_t value);

/**
 * @brief Gets the last error message set by an API call.
 * NOTE: This is often thread-unsafe in simple implementations.
 *       Consider thread-local storage for production use.
 * @return A pointer to the last error message string, or NULL if no error.
 */
MASM_API const char* masm_get_last_error();


#ifdef __cplusplus
} // extern "C"
#endif

#endif // MICROASM_CAPI_H
