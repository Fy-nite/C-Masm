#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // Required for std::find
#include <sstream>   // Required by compiler/interpreter includes indirectly
#include <fstream>   // Required by compiler/interpreter includes indirectly

#define CLR_RESET   "\033[0m"
#define CLR_ERROR   "\033[1;31m"    
#define CLR_OPCODE  "\033[1;36m"
#define CLR_OFFSET  "\033[1;33m"
#define CLR_OPERAND "\033[1;32m"
#define CLR_HEX     "\033[1;35m"
#define CLR_COMMENT "\033[1;90m"
#define CLR_STRING  "\033[1;34m"

// Filesystem includes
#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #error "Neither <filesystem> nor <experimental/filesystem> is available."
#endif
extern int decoder_main(int argc, char* argv[]); // Forward declaration for decoder_main
// Include the NEW header files
#include "microasm_compiler.h"
#include "microasm_interpreter.h"


void drawAsciiBox(const std::string& title, const std::vector<std::string>& content) {
    size_t maxWidth = title.size();
    for (const auto& line : content) {
        if (line.size() > maxWidth) {
            maxWidth = line.size();
        }
    }

    // Helper string for the horizontal bar character
    const std::string hBar = "═";
    std::string horizontalLine;
    for (size_t i = 0; i < maxWidth + 2; ++i) {
        horizontalLine += hBar;
    }

    // Use UTF-8 strings for box characters and the constructed horizontal line
    std::string topBorder = "\033[1;35m╔" + horizontalLine + "╗\033[0m";
    std::string bottomBorder = "\033[1;35m╚" + horizontalLine + "╝\033[0m";
    std::string titleBorder = "\033[1;35m║ \033[1;36m" + title + std::string(maxWidth - title.size(), ' ') + "\033[1;35m ║\033[0m";
    std::string separator = "\033[1;35m╠" + horizontalLine + "╣\033[0m"; // Separator line

    // Use std::cout instead of std::wcout
    std::cout << topBorder << "\n";
    std::cout << titleBorder << "\n";
    std::cout << separator << "\n";

    for (const auto& line : content) {
        // Use std::cout and std::string
        std::cout << "\033[1;35m║ \033[1;32m" << line << std::string(maxWidth - line.size(), ' ') << "\033[1;35m ║\033[0m" << "\n";
    }

    std::cout << bottomBorder << "\n";
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: No mode or file specified.\n";
        // Print usage information
        std::vector<std::string> usageContent = {
            "Usage: masm [mode] [options] [file]",
            "Modes:",
            "  -c  Compile a .masm file to binary.",
            "  -i  Interpret a .masm file or binary.",
            "  -u  Decode/disassemble a binary file.", // <-- Add this line
            "Options:",
            "  -d, --debug  Enable debug mode.",
            "Examples:",
            "  microasm -c example.masm",
            "  microasm -i example.masm",
            "  microasm -i example.bin"
        };
        drawAsciiBox("MicroASM Usage", usageContent);
        return 1;
    }

    // --- Debug Flag Handling ---
    bool enableDebug = false;
    std::vector<char*> args_filtered;
    args_filtered.push_back(argv[0]); // Keep program name

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--debug") {
            enableDebug = true;
        } else {
            args_filtered.push_back(argv[i]);
        }
    }

    argc = args_filtered.size();
    argv = args_filtered.data();
    // --- End Debug Flag Handling ---


    if (argc < 2) { // Check again after filtering
         std::cerr << "Error: No mode or file specified after options.\n";
         // Print usage again?
         return 1;
    }

    std::string mode = argv[1];

    if (mode == "-c") {
        if (argc < 3) {
             std::cerr << CLR_ERROR << "Error: No source file specified for compilation." << CLR_RESET << "\n";
             return 1;
        }
        std::string inputFile = argv[2];
        if (!fs::exists(inputFile)) {
            std::cerr << CLR_ERROR << "Error: Source file does not exist: " << inputFile << CLR_RESET << "\n";
            return 1;
        }
         if (fs::path(inputFile).extension() != ".masm") {
             std::cerr << CLR_ERROR << "Error: Input file for compilation must be a .masm file: " << inputFile << CLR_RESET << "\n";
             return 1;
         }
        if (enableDebug) std::cout << "[Debug] Compile mode selected.\n";
     
        return microasm_compiler_main(argc - 2, argv + 2); // Pass debug flag
    } else if (mode == "-i") {
        if (argc < 3) {
            std::cerr << CLR_ERROR << "Error: No file specified for interpretation." << CLR_RESET << "\n";
            return 1;
        }
        // Check if the file exists and is a valid file
        std::string inputFile = argv[2]; // File is at index 2

        if (!fs::exists(inputFile)) {
            std::cerr << CLR_ERROR << "Error: Input file does not exist: " << inputFile << CLR_RESET << "\n";
            return 1;
        }
        if (enableDebug) std::cout << "[Debug] Interpret mode selected.\n";
        // Pass remaining args (file onwards) to interpreter main
        return microasm_interpreter_main(argc - 2, argv + 2); // Pass debug flag
     } else if (mode == "-u") {
        if (argc < 3) {
            std::cerr << CLR_ERROR << "Error: No file specified for decoding." << CLR_RESET << "\n";
            return 1;
        }
        std::string inputFile = argv[2]; // File is at index 2
        if (!fs::exists(inputFile)) {
            std::cerr << CLR_ERROR << "Error: Input file does not exist: " << inputFile << CLR_RESET << "\n";
            return 1;
        }
         // Check if it's a .bin file?
         if (fs::path(inputFile).extension() != ".bin") {
             std::cerr << CLR_ERROR << "Warning: Input file for decoding might not be a .bin file: " << inputFile << CLR_RESET << "\n";
         }
        if (enableDebug) std::cout << "[Debug] Decode mode selected.\n";
        // Pass remaining args (file onwards) and debug flag to decoder main
        return decoder_main(argc - 2, argv + 2); // Pass debug flag
    }
    // Handle direct .masm file execution (mode is the filename)
    else if (fs::path(mode).extension() == ".masm") {
        std::string sourceFile = mode; // mode is the filename argv[1]
        fs::path inputPath = fs::absolute(sourceFile);

        // Check if source file exists before proceeding
        if (!fs::exists(inputPath)) {
             std::cerr << CLR_ERROR << "Error: Source file does not exist: " << sourceFile << CLR_RESET << std::endl;
             return 1;
        }

        if (enableDebug) std::cout << "[Debug] Direct execution mode selected for: " << sourceFile << "\n";

        fs::path parentDir = inputPath.parent_path();
        std::string stem = inputPath.stem().string();
        fs::path tempBinaryPath = parentDir / (stem + ".bin");
        std::string tempBinary = tempBinaryPath.string();

        try {
            // --- Compile Step ---
            if (enableDebug) std::cout << "[Debug] Compiling " << sourceFile << " to " << tempBinary << "\n";
            Compiler compiler;
            compiler.setDebugMode(enableDebug); // Pass debug flag to compiler class

             std::ifstream fileStream(sourceFile);
             if (!fileStream) {
                 throw std::runtime_error("Could not open source file: " + sourceFile);
             }
             std::ostringstream buffer;
             buffer << fileStream.rdbuf();
             fileStream.close();

            compiler.parse(buffer.str());
            compiler.compile(tempBinary);

            if (enableDebug) std::cout << "[Debug] Compilation successful.\n";

            // --- Interpret Step ---
            // Prepare arguments for the interpreted program (skip program name and source file)
            std::vector<std::string> programArgs;
            // Arguments start from argv[2] onwards
            for (int i = 2; i < argc; ++i) {
                programArgs.push_back(argv[i]);
            }

            if (enableDebug) std::cout << "[Debug] Interpreting " << tempBinary << "\n";
            Interpreter interpreter(65536, programArgs, enableDebug); // Pass debug flag to interpreter class
            interpreter.load(tempBinary);
            interpreter.execute();

            // Only remove temp file if debug mode is OFF
            if (!enableDebug && fs::exists(tempBinary)) {
                 fs::remove(tempBinary);
                 // Optional debug message even when debug is off, confirming cleanup
                 // std::cout << "[Info] Removed temporary binary: " << tempBinary << "\n";
            } else if (enableDebug && fs::exists(tempBinary)) {
                 std::cout << "[Debug] Kept temporary binary (debug mode enabled): " << tempBinary << "\n";
            }
            return 0; // Success

        } catch (const std::exception& e) {
            std::cerr << CLR_ERROR << "Error: " << e.what() << CLR_RESET << std::endl;
            // Attempt cleanup even on failure, but check if file exists and debug is off
            if (!enableDebug && fs::exists(tempBinary)) {
                fs::remove(tempBinary);
            }
            return 1;
        }

    } else {
        std::cerr << CLR_ERROR << "Error: Unknown mode or invalid file type: " << mode << CLR_RESET << "\n";
        // Print usage information again
         std::vector<std::string> usageContent = {
             "Usage: masm [mode] [options] [file]",
             "Modes:",
             "  -c <file.masm> Compile a .masm file to binary.",
             "  -i <file>      Interpret a .masm file or binary.",
             "  -u <file.bin>  Decode/disassemble a binary file.",
             "  <file.masm>    Compile and run a .masm file directly.",
             "Options:",
             "  -d, --debug    Enable debug mode.",
             "Examples:",
             "  microasm -c example.masm",
             "  microasm -i example.masm",
             "  microasm -i example.bin",
             "  microasm -u example.bin",
             "  microasm example.masm [program args...]"
         };
        drawAsciiBox("MicroASM Usage", usageContent);
        return 1;
    }
}
