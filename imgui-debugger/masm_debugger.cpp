#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <stdio.h>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <set>
#include "microasm_interpreter.h"

// Helper: Load file into buffer
static std::vector<uint8_t> loadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// Debugger state
struct DebuggerState {
    std::unique_ptr<Interpreter> interpreter;
    std::string loadedFile;
    bool running = false;
    bool stepRequested = false;
    bool breakOnHLT = true;
    int lastIP = 0;
    std::vector<std::string> programArgs;
    std::set<int> breakpoints;
};

void DrawDebuggerUI(DebuggerState& state) {
    ImGui::Begin("MASM Debugger");

    // File controls
    static char fileBuf[256] = "example.bin";
    static std::string lastLoadError; // Add this line
    ImGui::InputText("Bytecode File", fileBuf, sizeof(fileBuf));
    if (ImGui::Button("Load")) {
        state.interpreter = std::make_unique<Interpreter>(65536, state.programArgs, true);
        try {
            state.interpreter->load(fileBuf);
            state.loadedFile = fileBuf;
            state.running = false;
            lastLoadError.clear(); // Clear previous error
        } catch (const std::exception& e) {
            lastLoadError = e.what(); // Save error message
            ImGui::OpenPopup("Load Error");
        }
    }
    if (ImGui::BeginPopup("Load Error")) {
        ImGui::Text("Failed to load file!");
        if (!lastLoadError.empty())
            ImGui::TextWrapped("%s", lastLoadError.c_str());
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Run controls
    if (state.interpreter) {
        if (ImGui::Button(state.running ? "Pause" : "Run")) {
            state.running = !state.running;
        }
        ImGui::SameLine();
        if (ImGui::Button("Step")) {
            state.stepRequested = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            state.interpreter = std::make_unique<Interpreter>(65536, state.programArgs, true);
            state.interpreter->load(state.loadedFile);
            state.running = false;
        }
        ImGui::Checkbox("Break on HLT", &state.breakOnHLT);
    }

    // Registers
    if (state.interpreter) {
        ImGui::Separator();
        ImGui::Text("Registers:");
        for (size_t i = 0; i < state.interpreter->registers.size(); ++i) {
            ImGui::Text("R%-2zu: %10d (0x%08X)", i, state.interpreter->registers[i], state.interpreter->registers[i]);
            if ((i+1) % 4 != 0) ImGui::SameLine();
        }
    }

    // TODO: Add RAM, stack, disassembly, breakpoints, etc.

    ImGui::End();
}

int main(int, char**) {
    // SDL2 + ImGui setup
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Window* window = SDL_CreateWindow("MASM Debugger", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 800, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    DebuggerState dbgState;

    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }

        // Step/run logic
        if (dbgState.interpreter && (dbgState.running || dbgState.stepRequested)) {
            try {
                dbgState.interpreter->executeStep();
                dbgState.lastIP = dbgState.interpreter->getIP();
                dbgState.stepRequested = false;
                // TODO: Check for breakpoints, HLT, etc.
            } catch (const std::exception& e) {
                dbgState.running = false;
            }
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        DrawDebuggerUI(dbgState);

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
