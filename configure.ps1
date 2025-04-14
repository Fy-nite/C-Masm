param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('windows', 'linux')]
    [string]$TargetOS
)

# Get the directory where this script is located
$ScriptDir = $PSScriptRoot

# Determine the build directory based on the TargetOS parameter
if ($TargetOS -eq 'windows') { # Use $TargetOS parameter for check
    $BuildDirName = "build_windows"
} else { # Assumes 'linux' if not 'windows' due to ValidateSet
    $BuildDirName = "build_linux"
}

$BuildDirPath = Join-Path -Path $ScriptDir -ChildPath $BuildDirName

Write-Host "Configuring for target OS '$($TargetOS)' in directory: $BuildDirPath" # Updated message

# Create the build directory if it doesn't exist
if (-not (Test-Path -Path $BuildDirPath -PathType Container)) {
    Write-Host "Creating build directory..."
    New-Item -Path $BuildDirPath -ItemType Directory -Force | Out-Null
}

# Run CMake
Write-Host "Running CMake..."
cmake ..

Write-Host "Configuration complete in $BuildDirPath"
Write-Host "To build, navigate to '$BuildDirPath' and run your build command (e.g., 'cmake --build .' or 'make')."

