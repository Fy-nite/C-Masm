local term = {}
function term.write(text)
    io.write(text)
end
local fs = {}
function fs.exists(path)
    return io.open(path, "r") ~= nil
end
local bit32 = {}

local RegisterMachine = {}
RegisterMachine.__index = RegisterMachine

function RegisterMachine.new(numRegisters)
    local self = setmetatable({}, RegisterMachine)
    
    -- Initialize registers
    self.RAX = 0
    self.RBX = 0
    self.RCX = 0
    self.RDX = 0
    self.RSI = 0
    self.RDI = 0
    self.RBP = 0
    self.RSP = 0
    self.RIP = 0
    self.RFLAGS = 0
    
    -- Initialize R0-R15 registers
    for i = 0, 15 do
        self['R' .. i] = 0
    end
    
    -- Machine state
    self.memory = {}
    for i = 1, 1024 do
        self.memory[i] = 0
    end
    self.labels = {}
    self.callStack = {}
    self.currentCode = {}
    self.debugMode = false
    self.stepMode = false
    self.cpuSpeed = 200000.0
    -- Function table should reference the machine's methods, not global functions
    self.functionTable = {
        printi = function(...) self:printi(...) end,
        prints = function(...) self:prints(...) end,
        printc = function(...) self:printc(...) end
    }
    -- List of supported operations
    self.ops = {
        MOV = true, ADD = true, SUB = true, MUL = true,
        DIV = true, JMP = true, CMP = true, JE = true,
        JNE = true, PUSH = true, POP = true, CALL = true,
        HLT = true, INC = true, DEC = true, JG = true,
        JEQ = true, JL = true, JGE = true, JLE = true,
        ROR = true, ROL = true, AND = true, OR = true,
        XOR = true, NOT = true, SHL = true, SHR = true,
        NEG = true, RET = true, STRING = true, LBL = true,
        MOVADDR = true, MOVTO = true, ENTER = true, LEAVE = true,
        FILL = true, COPY = true, CMP_MEM = true, OUT = true
    }
    
    return self
end

-- Modify print functions to use term.write
function RegisterMachine:printi(reg)
    if type(reg) == "number" then
        term.write(tostring(reg) .. "\n")
    else
        term.write(tostring(self[reg]) .. "\n")
    end
end

function RegisterMachine:prints(address)
    local str = ""
    local i = tonumber(address) or self[address]
    while self.memory[i] ~= 0 do
        str = str .. string.char(self.memory[i])
        i = i + 1
    end
    term.write(str .. "\n")
end

function RegisterMachine:printc(reg)
    local value = tonumber(reg) or self[reg]
    term.write(string.char(value))
end

function RegisterMachine:inc(reg)
    self[reg] = self[reg] + 1
end

function RegisterMachine:dec(reg)
    self[reg] = self[reg] - 1
end

function RegisterMachine:mul(dest, src)
    self[dest] = self[dest] * (tonumber(src) or self[src])
end

function RegisterMachine:div(dest, src)
    self[dest] = self[dest] / (tonumber(src) or self[src])
end

function RegisterMachine:sub(dest, src)
    self[dest] = self[dest] - (tonumber(src) or self[src])
end

function RegisterMachine:ror(dest, src)
    -- OpenComputers bit32 API
    self[dest] = bit32.rrotate(self[dest], tonumber(src) or self[src])
end

function RegisterMachine:rol(dest, src)
    self[dest] = bit32.lrotate(self[dest], tonumber(src) or self[src])
end

function RegisterMachine:and_(dest, src)
    self[dest] = bit32.band(self[dest], (tonumber(src) or self[src]))
end

function RegisterMachine:or_(dest, src)
    self[dest] = bit32.bor(self[dest], (tonumber(src) or self[src]))
end

function RegisterMachine:xor(dest, src)
    self[dest] = bit32.bxor(self[dest], (tonumber(src) or self[src]))
end

function RegisterMachine:not_(dest)
    self[dest] = bit32.bnot(self[dest])
end

function RegisterMachine:shl(dest, src)
    self[dest] = bit32.lshift(self[dest], tonumber(src) or self[src])
end

function RegisterMachine:call(func, ...)
    if string.sub(func, 1, 1) == "#" then
        -- Internal label call
        local label = string.sub(func, 2)
        if self.labels[label] then
            table.insert(self.callStack, self.RIP)
            self.RIP = self.labels[label] - 1
        else
            error("Label " .. label .. " not found")
        end
    elseif string.sub(func, 1, 1) == "$" then
        -- External MNI function call
        local class, name = string.match(string.sub(func, 2), "^(%w+)%.(%w+)$")
        if class and name and self.functionTable[class] and self.functionTable[class][name] then
            self.functionTable[class][name](self, ...)
        else
            error("MNI function " .. func .. " not found")
        end
    else
        error("Invalid CALL target: " .. func)
    end
end

-- Basic operations
function RegisterMachine:mov(dest, src)
    if string.sub(dest, 1, 1) == '$' then
        -- Memory addressing
        local address = tonumber(string.sub(dest, 2))
        local value = tonumber(src) or self[src]
        self.memory[address] = value
    elseif string.sub(src, 1, 1) == '$' then
        -- Load from memory
        local address = tonumber(string.sub(src, 2))
        if address then
            self[dest] = self.memory[address]
        else
            local reg = string.sub(src, 2)
            self[dest] = self.memory[self[reg]]
        end
    else
        -- Register or immediate value
        self[dest] = tonumber(src) or self[src]
    end
end

function RegisterMachine:add(dest, src)
    self[dest] = self[dest] + (tonumber(src) or self[src])
end

function RegisterMachine:cmp(reg1, reg2)
    local val1 = tonumber(reg1) or self[reg1]
    local val2 = tonumber(reg2) or self[reg2]
    if val1 == val2 then self.RFLAGS = 1
    elseif val1 > val2 then self.RFLAGS = 2
    else self.RFLAGS = 0 end
end

function RegisterMachine:jmp(addr)
    if string.sub(addr, 1, 1) == '#' then
        addr = string.sub(addr, 2)
    end
    if self.labels[addr] then
        self.RIP = self.labels[addr]
    else
        self.RIP = tonumber(addr) - 1
    end
end

-- Stack operations
function RegisterMachine:push(reg)
    if reg == 'RIP' then
        table.insert(self.callStack, self.RIP)
    else
        self.memory[self.RSP] = self[reg]
        self.RSP = self.RSP + 1
    end
end

function RegisterMachine:pop(reg)
    self.RSP = self.RSP - 1
    self[reg] = self.memory[self.RSP]
end

function RegisterMachine:ret()
    if #self.callStack > 0 then
        self.RIP = table.remove(self.callStack) + 1
    else
        error("Return called with empty call stack")
    end
end

function RegisterMachine:movaddr(dest, src, offset)
    local address = self[src] + (tonumber(offset) or self[offset])
    self[dest] = self.memory[address]
end

function RegisterMachine:movto(dest, offset, src)
    local address = self[dest] + (tonumber(offset) or self[offset])
    self.memory[address] = tonumber(src) or self[src]
end

function RegisterMachine:enter(framesize)
    self:push("RBP")
    self:mov("RBP", "RSP")
    self.RSP = self.RSP - (tonumber(framesize) or self[framesize])
end

function RegisterMachine:leave()
    self:mov("RSP", "RBP")
    self:pop("RBP")
end

function RegisterMachine:jne(label)
    if self.RFLAGS ~= 1 then
        self:jmp(label)
    else
        self.RIP = self.RIP + 1
    end
end

function RegisterMachine:jg(label)
    if self.RFLAGS == 2 then
        self:jmp(label)
    else
        self.RIP = self.RIP + 1
    end
end

function RegisterMachine:jle(label)
    if self.RFLAGS ~= 2 then
        self:jmp(label)
    else
        self.RIP = self.RIP + 1
    end
end

function RegisterMachine:jge(label)
    if self.RFLAGS ~= 0 then
        self:jmp(label)
    else
        self.RIP = self.RIP + 1
    end
end

function RegisterMachine:fill(dest, value, len)
    local address = tonumber(dest) or self[dest]
    local fillValue = tonumber(value) or self[value]
    local length = tonumber(len) or self[len]
    for i = 0, length - 1 do
        self.memory[address + i] = fillValue
    end
end

function RegisterMachine:copy(dest, src, len)
    local destAddr = tonumber(dest) or self[dest]
    local srcAddr = tonumber(src) or self[src]
    local length = tonumber(len) or self[len]
    for i = 0, length - 1 do
        self.memory[destAddr + i] = self.memory[srcAddr + i]
    end
end

function RegisterMachine:cmp_mem(dest, src, len)
    local destAddr = tonumber(dest) or self[dest]
    local srcAddr = tonumber(src) or self[src]
    local length = tonumber(len) or self[len]
    for i = 0, length - 1 do
        if self.memory[destAddr + i] ~= self.memory[srcAddr + i] then
            self.RFLAGS = 0
            return
        end
    end
    self.RFLAGS = 1
end

function RegisterMachine:out(port, value)
    -- Resolve the port (can be a number or a register)
    local resolvedPort = tonumber(port) or self[port]
    if not resolvedPort then
        error("Invalid port: " .. tostring(port))
    end

    -- Resolve the value (can be a number, memory address, or a register)
    local resolvedValue
    if string.sub(value, 1, 1) == "$" then
        -- Memory address
        local address = tonumber(string.sub(value, 2)) or self[string.sub(value, 2)]
        if not address then
            error("Invalid memory address: " .. tostring(value))
        end

        -- Read the string from memory until a null byte is encountered
        resolvedValue = ""
        while self.memory[address] and self.memory[address] ~= 0 do
            resolvedValue = resolvedValue .. string.char(self.memory[address])
            address = address + 1
        end
    else
        -- Number or register
        resolvedValue = tonumber(value) or self[value]
    end

    -- Output the value based on the port
    if resolvedPort == 1 then
        -- Standard output
        term.write(tostring(resolvedValue) .. "\n")
    elseif resolvedPort == 2 then
        -- Standard error
        io.stderr:write(tostring(resolvedValue) .. "\n")
    else
        error("Unsupported port: " .. resolvedPort)
    end
end

-- Execution control
function RegisterMachine:scanLabels(code)
    self.labels = {}  -- Reset labels
    self.mainLabel = nil  -- Track the position of the 'main' label
    for i, line in ipairs(code) do
        local parts = {}
        for word in string.gmatch(line, "%S+") do
            table.insert(parts, word)
        end
        
        if string.upper(parts[1]) == "LBL" then
            local label = string.gsub(parts[2], ":", "")
            self.labels[label] = i
            if label == "main" then
                self.mainLabel = i
            end
            if self.debugMode then
                print(string.format("Found label: %s at line %d", label, i))
            end
        end
    end
end

function RegisterMachine:execute(code)
    self.currentCode = code
    self:scanLabels(code)
    
    -- Start execution from the 'main' label if it exists
    if self.mainLabel then
        self.RIP = self.mainLabel - 1
        print(string.format("Starting execution at 'main' label (line %d)", self.mainLabel))  -- Debugging log
    else
        self.RIP = 0
        print("No 'main' label found. Starting execution from the beginning.")  -- Debugging log
    end
    
    -- If in debug mode, initialize debugger
    if self.debugMode then
        local ok, debugger = pcall(require, "debugger")  -- Fix require path
        if not ok then
            term.write("Error loading debugger module: " .. debugger .. "\n")
            return
        end
        local dbg = debugger.new(self)
        dbg:start()
        return
    end
    
    -- Normal execution
    while self.RIP < #code do
        local line = code[self.RIP + 1]
        if not line or string.match(line, "^%s*;") then
            self.RIP = self.RIP + 1
            goto continue
        end
        
        if self.debugMode then
            print(string.format("Executing line %d: %s", self.RIP, line))
        end
        
        self:executeInstruction(line)
        
        ::continue::
    end
end

function RegisterMachine:executeInstruction(line)
    if not line then return false end
    
    -- Skip comments and empty lines
    if string.match(line, "^%s*$") or string.match(line, "^%s*;") then
        self.RIP = self.RIP + 1
        return true
    end
    
    local parts = {}
    local inQuotes = false
    local currentPart = ""
    for word in string.gmatch(line, "%S+") do
        if string.sub(word, 1, 1) == '"' then
            inQuotes = true
            currentPart = word
        elseif inQuotes then
            currentPart = currentPart .. " " .. word
            if string.sub(word, -1) == '"' then
                inQuotes = false
                table.insert(parts, currentPart)
                currentPart = ""
            end
        else
            table.insert(parts, word)
        end
    end
    if currentPart ~= "" then
        table.insert(parts, currentPart)
    end

    -- Normalize the instruction to uppercase
    local op = string.upper(parts[1])
    if op == "LBL" then
        self.RIP = self.RIP + 1
        return true
    end


    if op == "MNI" then
        local class, name = string.match(parts[2], "^(%w+)%.(%w+)$")
        if class and name and self.functionTable[class] and self.functionTable[class][name] then
            self.functionTable[class][name](self, table.unpack(parts, 3))
        else
            error("MNI function " .. parts[2] .. " not found")
        end
        self.RIP = self.RIP + 1
        return true
    end

    -- Handle the DB directive
    if op == "DB" then
        local address = tonumber(string.sub(parts[2], 2)) or self[parts[2]]
        if not address then
            error("Invalid memory address for DB: " .. tostring(parts[2]))
        end

        local data = parts[3]
        if string.sub(data, 1, 1) == '"' and string.sub(data, -1) == '"' then
            data = string.sub(data, 2, -2) -- Remove quotes
            for i = 1, #data do
                self.memory[address + i - 1] = string.byte(data, i)
            end
            self.memory[address + #data] = 0 -- Null-terminate the string
        else
            error("Invalid data format for DB: " .. tostring(data))
        end

        self.RIP = self.RIP + 1
        return true
    end

    if self.ops[op] then
        local method = string.lower(op)
        if self[method] then
            self[method](self, table.unpack(parts, 2))
        end
    end
    
    if op ~= "JMP" and op ~= "RET" then
        self.RIP = self.RIP + 1
    end
    
    return true
end

function RegisterMachine:registerMNI(class, name, func)
    if not self.functionTable[class] then
        self.functionTable[class] = {}
    end
    self.functionTable[class][name] = func
end

-- Example: Registering MNI functions
local function registerMNIExamples(machine)
    -- Math operations
    machine:registerMNI("Math", "sin", function(machine, src, dest)
        machine[dest] = math.sin(machine[src])
    end)
    machine:registerMNI("Math", "cos", function(machine, src, dest)
        machine[dest] = math.cos(machine[src])
    end)

    -- Memory management
    machine:registerMNI("Memory", "allocate", function(machine, size, dest)
        local address = #machine.memory + 1
        for i = 1, machine[size] do
            table.insert(machine.memory, 0)
        end
        machine[dest] = address
    end)
    machine:registerMNI("Memory", "free", function(machine, address)
        -- No-op for simplicity (real implementation would manage memory blocks)
    end)

    -- Debugging
    machine:registerMNI("Debug", "dumpRegisters", function(machine, dest)
        local dump = {}
        for reg, value in pairs(machine) do
            if type(value) == "number" then
                table.insert(dump, string.format("%s: %d", reg, value))
            end
        end
        machine.memory[machine[dest]] = table.concat(dump, "\n")
    end)
end

-- Call this function during initialization to register MNI examples
registerMNIExamples(RegisterMachine.new(16))

local function resolveIncludePath(rootFolder, localFolder, includePath)
    -- Convert structured includes like "stdio.print" to "stdio/print.masm"
    local structuredPath = includePath:gsub("%.", "/") .. ".masm"

    -- Check in the root folder
    local rootFullPath = rootFolder .. "/" .. structuredPath
    if fs.exists(rootFullPath) then
        return rootFullPath
    end

    -- Check in the local folder
    local localFullPath = localFolder .. "/" .. includePath
    if fs.exists(localFullPath) then
        return localFullPath
    end

    error("Include file not found: " .. includePath)
end

local function processIncludes(rootFolder, localFolder, code)
    local processedCode = {}
    for _, line in ipairs(code) do
        local includePath = line:match('^#include%s+"(.-)"$')
        if includePath then
            local fullPath = resolveIncludePath(rootFolder, localFolder, includePath)
            local file = io.open(fullPath, "r")
            for includeLine in file:lines() do
                table.insert(processedCode, includeLine)
            end
            file:close()
        else
            table.insert(processedCode, line)
        end
    end
    return processedCode
end

-- Modify init() to process includes and handle the ANSI TUI flag
local function init(filename, options)
    if not filename then
        term.write("Usage: interp <filename> [options]\n")
        term.write("Options:\n")
        term.write("  -d, --debug           Start in debug mode with text-based TUI\n")
        term.write("  -D, --debug-cli       Start in debug mode without TUI\n")
        term.write("  -A, --ansi-debug-tui  Start in debug mode with ANSI TUI (requires terminal support)\n")
        term.write("  -s, --step            Start in step mode\n")
        term.write("  --root=<folder>       Set the root folder for structured includes\n")
        return
    end
    
    if not fs.exists(filename) then
        term.write("Error: Could not find file " .. filename .. "\n")
        return
    end

    -- Determine the root folder and local folder
    local rootFolder = "root"
    local localFolder = filename:match("^(.*)/") or "."
    for _, arg in ipairs(options) do
        local key, value = arg:match("^%-%-(%w+)=(.+)$")
        if key == "root" then
            rootFolder = value
        end
    end

    local file = io.open(filename, "r")
    local code = {}
    for line in file:lines() do
        table.insert(code, line)
    end
    file:close()

    -- Process #include directives
    code = processIncludes(rootFolder, localFolder, code)
    
    local machine = RegisterMachine.new(16)
    
    -- Handle options
    options = options or {}
    
    -- Set global ANSI mode flag based on options
    _G.ANSI_MODE = options.ansiDebugTui
    
    if options.debug or options.ansiDebugTui then
        machine.debugMode = true
        machine.autoTUI = true
    elseif options.debugCli then
        machine.debugMode = true
        machine.autoTUI = false
    end
    if options.step then
        machine.stepMode = true
    end
    
    machine:execute(code)
end

-- Main program entry point
local function main(args)
    local filename = args[1]
    local options = {
        debug = false,
        debugCli = false,
        ansiDebugTui = false,
        step = false
    }
    
    -- Process command line arguments
    for i = 2, #args do
        local arg = args[i]
        if arg == "-d" or arg == "--debug" then
            options.debug = true
        elseif arg == "-D" or arg == "--debug-cli" then
            options.debugCli = true
        elseif arg == "-A" or arg == "--ansi-debug-tui" then
            options.ansiDebugTui = true
        elseif arg == "-s" or arg == "--step" then
            options.step = true
        elseif arg:match("^--(.+)=(.+)$") then
            -- This is a keyed option, already handled in init()
        end
    end
    
    init(filename, options)
end

main({...})