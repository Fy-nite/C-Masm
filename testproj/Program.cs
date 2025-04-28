using System;
using static MicroAsmRuntime; // Import static members for easier access

public class Program
{
    public static int Main(string[] args)
    {
        Console.WriteLine("MicroASM C# Host Test");

        // --- Configuration ---
        string bytecodeFile = "example.bin"; // Ensure this file exists and is copied to output
        int ramSize = 65536; // 64 KB
        bool debugMode = true;
        string[] scriptArgs = { "arg1_from_csharp", "arg2" }; // Example args for the script
        // --- End Configuration ---

        // Check if bytecode file exists
        if (!File.Exists(bytecodeFile))
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"Error: Bytecode file not found: {Path.GetFullPath(bytecodeFile)}");
            Console.WriteLine("Please ensure 'example.bin' is compiled and copied to the output directory.");
            Console.ResetColor();
            return 1;
        }

        MasmInterpreterHandle handle = MasmInterpreterHandle.Zero;
        try
        {
            // 1. Create Interpreter
            Console.WriteLine("Creating interpreter...");
            handle = CreateInterpreter(ramSize, debugMode ? 1 : 0);
            if (handle.IsZero)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"Failed to create interpreter: {GetLastError() ?? "Unknown error"}");
                Console.WriteLine("Ensure the runtime DLL exports 'masm_create_interpreter' with the correct name and calling convention.");
                Console.ResetColor();
                return 1;
            }
            Console.WriteLine("Interpreter created.");

            // 2. Load Bytecode
            Console.WriteLine($"Loading bytecode from '{bytecodeFile}'...");
            MasmResult loadResult = LoadBytecode(handle, bytecodeFile);
            if (loadResult != MasmResult.Ok)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"Failed to load bytecode: [{loadResult}] {GetLastError() ?? "Unknown error"}");
                Console.ResetColor();
                return 1;
            }
            Console.WriteLine("Bytecode loaded.");

            // 3. Execute
            Console.WriteLine("Executing bytecode...");
            Console.WriteLine("--- Script Output Start ---");
            Console.ForegroundColor = ConsoleColor.Cyan; // Differentiate script output

            MasmResult execResult = Execute(handle, scriptArgs); // Use helper to pass args

            Console.ResetColor();
            Console.WriteLine("--- Script Output End ---");

            if (execResult != MasmResult.Ok)
            {
                // Error message should have been printed by the interpreter's execute or captured by GetLastError
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"Execution failed: [{execResult}] {GetLastError() ?? "Interpreter internal error"}");
                Console.ResetColor();
                // Don't exit immediately, try to get register info if possible
            }
            else
            {
                Console.WriteLine("Execution finished (HLT likely reached).");
            }


            // 4. Example: Inspect a register after execution (e.g., R0)
            if (GetRegister(handle, 0, out int r0Value) == MasmResult.Ok)
            {
                Console.WriteLine($"Value of R0 after execution: {r0Value}");
            }
            else
            {
                 Console.WriteLine($"Could not get R0 value: {GetLastError() ?? "Unknown error"}");
            }

             // 5. Example: Write/Read RAM
             int testAddress = 100; // Choose an address not likely used by stack/data
             int testValue = 12345;
             Console.WriteLine($"Writing {testValue} to RAM address {testAddress}...");
             if (WriteRamInt(handle, testAddress, testValue) == MasmResult.Ok)
             {
                 Console.WriteLine($"Reading back from RAM address {testAddress}...");
                 if (ReadRamInt(handle, testAddress, out int readValue) == MasmResult.Ok)
                 {
                     Console.WriteLine($"Read value: {readValue} (Expected: {testValue})");
                     if(readValue != testValue) Console.WriteLine("RAM Read/Write Mismatch!");
                 } else {
                      Console.WriteLine($"Failed to read RAM: {GetLastError() ?? "Unknown error"}");
                 }
             } else {
                 Console.WriteLine($"Failed to write RAM: {GetLastError() ?? "Unknown error"}");
             }


        }
        catch (EntryPointNotFoundException ex)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"Error: Entry point not found in the runtime DLL. {ex.Message}");
            Console.WriteLine("Ensure the runtime DLL exports the required functions with the correct names.");
            Console.ResetColor();
            return 1;
        }
        catch (DllNotFoundException)
        {
             Console.ForegroundColor = ConsoleColor.Red;
             Console.WriteLine($"Error: Cannot find the MicroASM runtime DLL ('{DllName}.dll').");
             Console.WriteLine("Ensure the C++ project is built as a DLL and it's in the same directory as the C# executable or in the system's PATH.");
             Console.ResetColor();
             return 1;
        }
        catch (Exception ex)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"An unexpected C# error occurred: {ex.Message}");
            Console.WriteLine(ex.StackTrace);
            Console.ResetColor();
            return 1;
        }
        finally
        {
            // 6. Destroy Interpreter
            if (!handle.IsZero)
            {
                Console.WriteLine("Destroying interpreter...");
                DestroyInterpreter(handle);
                Console.WriteLine("Interpreter destroyed.");
            }
        }

        Console.WriteLine("Host program finished.");
        return 0;
    }
}
