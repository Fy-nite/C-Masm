#include <iostream>
#include <string>
#include <vector> // Include vector for dynamic argument list

// Use std::filesystem or fallback to std::experimental::filesystem
#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #error "Neither <filesystem> nor <experimental/filesystem> is available."
#endif

int microasm_compiler_main(int argc, char* argv[]);
int microasm_interpreter_main(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << R"(
Usage: program <mode|file> [args...]
Modes:
  -c    Compile a .masm file into a binary
  -i    Run a .masm file directly
  <file.masm> Automatically compile and run the file
)";
        return 1;
    }

    std::string mode = argv[1];
    if (mode == "-c") {
        return microasm_compiler_main(argc - 1, argv + 1);
    } else if (mode == "-i") {
        return microasm_interpreter_main(argc - 1, argv + 1);
    } else if (fs::path(mode).extension() == ".masm") {
        // Compile and run the .masm file
        fs::path inputPath = fs::absolute(mode); // Get absolute path for robustness
        fs::path parentDir = inputPath.parent_path();
        std::string stem = inputPath.stem().string();
        fs::path tempBinaryPath = parentDir / (stem + ".bin"); // Combine directory, stem, and extension
        std::string tempBinary = tempBinaryPath.string(); // Convert path to string

        // Prepare arguments for the compiler
        std::vector<std::string> compilerArgStrings;
        compilerArgStrings.push_back(argv[0]); // Program name (can be anything, traditionally the command)
        compilerArgStrings.push_back(mode);    // Source file
        compilerArgStrings.push_back(tempBinary); // Output file

        std::vector<char*> compilerArgsVec;
        for (std::string& s : compilerArgStrings) {
            compilerArgsVec.push_back(s.data()); // Use .data() for non-const access if needed, or manage copies
        }
        // for later use, if you need to modify the strings in the compiler:
        // Note: If microasm_compiler_main modifies argv, you MUST create writable copies.
        // Example of creating writable copies (safer):
        // std::vector<std::vector<char>> compilerArgBuffers;
        // std::vector<char*> compilerArgsVec;
        // for(const std::string& s : compilerArgStrings) {
        //     compilerArgBuffers.emplace_back(s.begin(), s.end());
        //     compilerArgBuffers.back().push_back('\0'); // Null-terminate
        //     compilerArgsVec.push_back(compilerArgBuffers.back().data());
        // }

  
        int compileoutput = microasm_compiler_main(compilerArgsVec.size(), compilerArgsVec.data());
        if (compileoutput != 0) {
            std::cerr << "Compilation failed.\n";
            fs::remove(tempBinary); // Clean up temp file on failure
            return 1;
        }

        // Prepare arguments for the interpreter
        std::vector<std::string> interpreterArgStrings;

        interpreterArgStrings.push_back(tempBinary); // Compiled binary pat

        // Add any additional arguments passed after the .masm file
        for (int i = 2; i < argc; ++i) {
            interpreterArgStrings.push_back(argv[i]);
        }

        std::vector<char*> interpreterArgsVec;
        for (std::string& s : interpreterArgStrings) {
            interpreterArgsVec.push_back(s.data()); // Again, consider writable copies if needed
        }
        // Example of creating writable copies (safer):
        // std::vector<std::vector<char>> interpreterArgBuffers;
        // std::vector<char*> interpreterArgsVec;
        // for(const std::string& s : interpreterArgStrings) {
        //     interpreterArgBuffers.emplace_back(s.begin(), s.end());
        //     interpreterArgBuffers.back().push_back('\0'); // Null-terminate
        //     interpreterArgsVec.push_back(interpreterArgBuffers.back().data());
        // }


        printf("\n");
        int result = microasm_interpreter_main(interpreterArgsVec.size(), interpreterArgsVec.data());
        fs::remove(tempBinary); // Clean up the temporary binary file
        return result;
    } else {
        std::cerr << "Unknown mode or file: " << mode << "\n";
        return 1;
    }
}
