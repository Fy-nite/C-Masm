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
    // args_filtered.push_back(argv[0]); // Keep program name

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--debug") {
            enableDebug = true;
        } else {
            args_filtered.push_back(argv[i]);
        }
    }

    // // Update argc and argv to the filtered list
    // argc = args_filtered.size();
    // argv = args_filtered.data();
    // --- End Debug Flag Handling ---


    if (argc < 2) { // Check again after filtering
         std::cerr << "Error: No mode or file specified after options.\n";
         // Print usage again?
         return 1;
    }

    std::string mode = argv[1];

    if (mode == "-c") {
        // Pass filtered args to compiler main
        return microasm_compiler_main(argc - 2, argv + 2); // argc-2/argv+2 skips the program name
    } else if (mode == "-i") {
        // Pass filtered args to interpreter main
        return microasm_interpreter_main(argc - 2, argv + 2); // argc-1/argv+1 skips the program name
     } else if (mode == "-u") {
        printf("Decoder mode selected.\n");
        // Pass filtered args to decoder main
        for (int i = 0; i < argc; ++i) {
            std::cout << "Arg " << i << ": " << argv[i] << "\n";
        }
        return decoder_main(argc, argv + 1); // Call the decoder main function
    }

    else if (fs::path(mode).extension() == ".masm") {
        // Compile and run the .masm file directly using classes
        std::string sourceFile = mode;
        fs::path inputPath = fs::absolute(sourceFile);
        fs::path parentDir = inputPath.parent_path();
        std::string stem = inputPath.stem().string();
        fs::path tempBinaryPath = parentDir / (stem + ".bin");
        std::string tempBinary = tempBinaryPath.string();

        try {
            // --- Compile Step ---
            if (enableDebug) std::cout << "[Debug] Compiling " << sourceFile << " to " << tempBinary << "\n";
            Compiler compiler; // Now uses declaration from microasm_compiler.h
            compiler.setDebugMode(enableDebug);

            // The compiler's parse method needs the source *content*, not filename
            // Read the source file content first
             std::ifstream fileStream(sourceFile);
             if (!fileStream) {
                 throw std::runtime_error("Could not open source file: " + sourceFile);
             }
             std::ostringstream buffer;
             buffer << fileStream.rdbuf();
             fileStream.close(); // Close the file stream

            // Parse the source content
            compiler.parse(buffer.str()); // Use parse method with string content

            // Compile to output file
            compiler.compile(tempBinary);

            if (enableDebug) std::cout << "[Debug] Compilation successful.\n";

            // --- Interpret Step ---
            // Prepare arguments for the interpreted program (skip program name and source file)
            std::vector<std::string> programArgs;
            for (int i = 2; i < argc; ++i) {
                programArgs.push_back(argv[i]);
            }

            if (enableDebug) std::cout << "[Debug] Interpreting " << tempBinary << "\n";
            Interpreter interpreter(65536, programArgs, enableDebug); // Now uses declaration from microasm_interpreter.h
            interpreter.load(tempBinary);
            interpreter.execute();

            fs::remove(tempBinary); // Clean up the temporary binary file
            return 0; // Success

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            fs::remove(tempBinary); // Clean up temp file on failure
            return 1;
        }

    } else {
        std::cerr << "Unknown mode or file: " << mode << "\n";
        return 1;
    }
}
