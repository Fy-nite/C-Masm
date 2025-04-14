using System;
using System.Runtime.InteropServices;
using System.Text;

public static class MicroAsmRuntime
{

    public const string DllName = "libmicroasm";

    // --- Opaque Handle ---
    // Use IntPtr for opaque handles
    public struct MasmInterpreterHandle : IEquatable<MasmInterpreterHandle>
    {
        private readonly IntPtr _handle;
        public static readonly MasmInterpreterHandle Zero = new(IntPtr.Zero);

        private MasmInterpreterHandle(IntPtr handle) => _handle = handle;
        public bool IsZero => _handle == IntPtr.Zero;
        public override bool Equals(object? obj) => obj is MasmInterpreterHandle other && Equals(other);
        public bool Equals(MasmInterpreterHandle other) => _handle.Equals(other._handle);
        public override int GetHashCode() => _handle.GetHashCode();
        public static bool operator ==(MasmInterpreterHandle left, MasmInterpreterHandle right) => left.Equals(right);
        public static bool operator !=(MasmInterpreterHandle left, MasmInterpreterHandle right) => !(left == right);
    }

    // --- Error Codes Enum ---
    public enum MasmResult
    {
        Ok = 0,
        ErrorGeneral = -1,
        ErrorInvalidHandle = -2,
        ErrorLoadFailed = -3,
        ErrorExecutionFailed = -4,
        ErrorInvalidArgument = -5,
        ErrorMemory = -6
    }

    // --- P/Invoke Declarations ---

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, EntryPoint = "masm_create_interpreter")]
    public static extern MasmInterpreterHandle CreateInterpreter(int ramSize, int debugMode);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, EntryPoint = "masm_destroy_interpreter")]
    public static extern void DestroyInterpreter(MasmInterpreterHandle handle);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi, EntryPoint = "masm_load_bytecode")]
    public static extern MasmResult LoadBytecode(MasmInterpreterHandle handle, string bytecodeFile);

    // For masm_execute, passing string arrays requires careful marshalling.
    // Option 1: Simple (pass null if no args)
    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, EntryPoint = "masm_execute")]
    public static extern MasmResult Execute(MasmInterpreterHandle handle, int argc, IntPtr argv); // Pass IntPtr.Zero for argv if no args

    // Option 2: More complex marshalling (requires helper)
    // [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi, EntryPoint = "masm_execute")]
    // public static extern MasmResult Execute(MasmInterpreterHandle handle, int argc, [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] argv);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, EntryPoint = "masm_get_register")]
    public static extern MasmResult GetRegister(MasmInterpreterHandle handle, int registerIndex, out int outValue);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, EntryPoint = "masm_read_ram_int")]
    public static extern MasmResult ReadRamInt(MasmInterpreterHandle handle, int address, out int outValue);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, EntryPoint = "masm_write_ram_int")]
    public static extern MasmResult WriteRamInt(MasmInterpreterHandle handle, int address, int value);

    // Helper function to get string from C pointer
    private static string? PtrToStringAnsi(IntPtr ptr)
    {
        return Marshal.PtrToStringAnsi(ptr);
    }

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, EntryPoint = "masm_get_last_error")]
    private static extern IntPtr GetLastErrorPtr(); // Get pointer

    // Public wrapper to get the last error string
    public static string? GetLastError()
    {
        IntPtr errorPtr = GetLastErrorPtr();
        return (errorPtr == IntPtr.Zero) ? null : PtrToStringAnsi(errorPtr);
    }

    // --- Helper for Execute with Args (if using Option 1) ---
    public static MasmResult Execute(MasmInterpreterHandle handle, string[]? args)
    {
        if (args == null || args.Length == 0)
        {
            return Execute(handle, 0, IntPtr.Zero);
        }

        // Allocate unmanaged memory for the array of pointers
        IntPtr argvPtr = Marshal.AllocHGlobal(args.Length * IntPtr.Size);
        if (argvPtr == IntPtr.Zero) throw new OutOfMemoryException("Failed to allocate memory for argv.");

        try
        {
            // Marshal each string to an unmanaged ANSI string and store the pointer
            for (int i = 0; i < args.Length; i++)
            {
                IntPtr stringPtr = Marshal.StringToHGlobalAnsi(args[i]);
                Marshal.WriteIntPtr(argvPtr, i * IntPtr.Size, stringPtr);
            }

            // Call the native function
            MasmResult result = Execute(handle, args.Length, argvPtr);

            // Free the allocated unmanaged strings
            for (int i = 0; i < args.Length; i++)
            {
                IntPtr stringPtr = Marshal.ReadIntPtr(argvPtr, i * IntPtr.Size);
                Marshal.FreeHGlobal(stringPtr);
            }

            return result;
        }
        finally
        {
            // Free the array of pointers
            Marshal.FreeHGlobal(argvPtr);
        }
    }
}
