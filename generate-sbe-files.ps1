<#
Generate SBE C++ codec files from B3 UMDF XML schema.

Copyright (c) 2026, Augusto Damasceno.
All rights reserved.

SPDX-License-Identifier: BSD-2-Clause
#>

#############################################################################
# Generate SBE C++ Codec Files from B3 UMDF XML Schema (PowerShell)
#
# Usage:
#   .\generate-sbe-files.ps1 -XmlFile <path-to-xml-file>
#   .\generate-sbe-files.ps1 clean
#
# Example:
#   .\generate-sbe-files.ps1 -XmlFile "schemas/b3/b3-market-data-messages-2.3.1.xml"
#   .\generate-sbe-files.ps1 clean
#
# If you get an execution policy error, use one of these:
#   Unblock-File -Path ".\generate-sbe-files.ps1"
#   powershell -ExecutionPolicy Bypass -File ".\generate-sbe-files.ps1" -XmlFile <path>
#
# This script will:
#   1. Validate Java is installed
#   2. Clone the SBE repository to a temporary directory
#   3. Build the SBE tool with Gradle
#   4. Generate C++ codec files from the B3 UMDF XML schema
#   5. Display the output location
#############################################################################

param(
    [string]$XmlFile,
    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]$Arguments
)

# ============================================================================
# Helper Functions
# ============================================================================

function Write-Header {
    param([string]$Message)
    Write-Host ""
    Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
    Write-Host $Message -ForegroundColor Cyan
    Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Step {
    param([string]$Step, [string]$Message)
    Write-Host "[$Step] $Message" -ForegroundColor Yellow
}

function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

function Write-Error {
    param([string]$Message)
    Write-Host "ERROR: $Message" -ForegroundColor Red
}

function Write-Warning {
    param([string]$Message)
    Write-Host "⚠ $Message" -ForegroundColor Yellow
}

# Determine if the action is clean
$IsClean = $false
if ($XmlFile -eq "clean") {
    $IsClean = $true
}
elseif ($Arguments -and $Arguments.Count -gt 0 -and $Arguments[0] -eq "clean") {
    $IsClean = $true
}

# Handle clean action
if ($IsClean) {
    Write-Host ""
    Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
    Write-Host "Cleaning up temporary SBE generation directories" -ForegroundColor Cyan
    Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
    Write-Host ""
    
    $TempPattern = Join-Path $env:TEMP "sbe-generation-*"
    $TempDirectories = Get-Item -Path $TempPattern -ErrorAction SilentlyContinue | Where-Object { $_.PSIsContainer }
    $DirsToDelete = @()
    
    # Collect all directories to be deleted
    foreach ($tempDir in $TempDirectories) {
        $DirsToDelete += $tempDir.FullName
    }
    
    if ($DirsToDelete.Count -eq 0) {
        Write-Warning "No temporary SBE generation directories found."
        Write-Host ""
        exit 0
    }
    
    # Display directories to be deleted
    $plural = if ($DirsToDelete.Count -eq 1) { "directory" } else { "directories" }
    Write-Host "The following $plural will be deleted:" -ForegroundColor Yellow
    Write-Host ""
    foreach ($dir in $DirsToDelete) {
        Write-Host "  $dir" -ForegroundColor Red
    }
    Write-Host ""
    
    # Ask for confirmation
    $confirmation = Read-Host "Are you sure you want to delete these $plural ? (yes/no)"
    Write-Host ""
    
    if ($confirmation -ne "yes") {
        Write-Warning "Cleanup cancelled."
        Write-Host ""
        exit 0
    }
    
    # Delete confirmed directories
    $DeletedCount = 0
    foreach ($tempDir in $DirsToDelete) {
        Write-Host "Removing: $tempDir" -ForegroundColor Yellow
        try {
            Remove-Item -Path $tempDir -Recurse -Force -ErrorAction Stop
            Write-Success "Removed"
            $DeletedCount++
        }
        catch {
            Write-Host "✗ Failed to remove: $($_.Exception.Message)" -ForegroundColor Red
        }
        Write-Host ""
    }
    
    $plural = if ($DeletedCount -eq 1) { "directory" } else { "directories" }
    Write-Success "Cleanup completed. $DeletedCount $plural removed."
    Write-Host ""
    exit 0
}

# Determine if we have no arguments
$HasNoArgs = $false
if ([string]::IsNullOrEmpty($XmlFile) -and ($null -eq $Arguments -or $Arguments.Count -eq 0)) {
    $HasNoArgs = $true
}

if ($HasNoArgs) {
    Write-Host ""
    Write-Host "SBE C++ Codec Generator for B3 UMDF" -ForegroundColor Cyan -BackgroundColor Black
    Write-Host "=====================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Error "Missing required argument"
    Write-Host ""
    Write-Host "Usage:"
    Write-Host "  .\\generate-sbe-files.ps1 -XmlFile <path-to-xml-file>"
    Write-Host "  .\\generate-sbe-files.ps1 clean"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\\generate-sbe-files.ps1 -XmlFile `'C:\path\to\b3-market-data-messages-2.3.1.xml`'"
    Write-Host "  .\\generate-sbe-files.ps1 clean    # Remove all temporary SBE generation directories"
    Write-Host ""
    exit 1
}

# Parse/Validate XmlFile
$IsValidXmlFile = $false
if (-not [string]::IsNullOrEmpty($XmlFile) -and $XmlFile -like "*.xml" -and ($null -eq $Arguments -or $Arguments.Count -eq 0)) {
    $IsValidXmlFile = $true
}

if (-not $IsValidXmlFile) {
    Write-Host ""
    Write-Error "Invalid arguments"
    Write-Host ""
    Write-Host "Usage:"
    Write-Host "  .\\generate-sbe-files.ps1 -XmlFile <path-to-xml-file>"
    Write-Host "  .\\generate-sbe-files.ps1 clean"
    Write-Host ""
    exit 1
}

# Set error action preference
$ErrorActionPreference = "Stop"

# ============================================================================
# Configuration
# ============================================================================

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$TempBasePath = Join-Path $env:TEMP "sbe-generation-$([System.Diagnostics.Process]::GetCurrentProcess().Id)"
$SbeRepoUrl = "https://github.com/aeron-io/simple-binary-encoding.git"
$SbeRepoDir = Join-Path $TempBasePath "simple-binary-encoding"
$OutputDir = Join-Path $TempBasePath "output"
$XmlFilename = "b3-market-data-messages-2.3.1.xml"

Write-Host ""
Write-Host "SBE C++ Codec Generator for B3 UMDF" -ForegroundColor Cyan -BackgroundColor Black
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# ============================================================================
# PHASE 1: Validation
# ============================================================================

Write-Header "PHASE 1: Validation"

# Check if Java is installed
Write-Step "1/4" "Checking if Java is installed..."
$JavaCmd = Get-Command java -ErrorAction SilentlyContinue
if ($null -eq $JavaCmd) {
    Write-Error "Java is not installed or not in PATH"
    Write-Host ""
    Write-Host "Please install Java (JDK 11 or later) and make sure it's accessible via the 'java' command."
    Write-Host ""
    
    Write-Host "Installation Methods:" -ForegroundColor Yellow
    Write-Host ""
    
    Write-Host "Option 1: Using Chocolatey (recommended)" -ForegroundColor Cyan
    Write-Host "  choco install openjdk17 -y"
    Write-Host ""
    
    Write-Host "Option 2: Using Windows Package Manager" -ForegroundColor Cyan
    Write-Host "  winget install Oracle.OpenJDK.17"
    Write-Host ""
    
    Write-Host "Option 3: Download manually" -ForegroundColor Cyan
    Write-Host "  Visit: https://openjdk.org/install/"
    Write-Host ""
    
    Write-Host "After installation:" -ForegroundColor Yellow
    Write-Host "  1. Add Java to your system PATH"
    Write-Host "  2. Restart PowerShell or your terminal"
    Write-Host "  3. Verify with: java -version"
    Write-Host ""
    
    exit 1
}

# Safely extract Java version
try {
    $OldEAP = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    $JavaVersion = java -version 2>&1 | Select-Object -First 1
    $ErrorActionPreference = $OldEAP
    Write-Success "Java found: $JavaVersion"
}
catch {
    Write-Warning "Could not retrieve Java version: $($_.Exception.Message)"
}
Write-Host ""

# Check if XML file exists
Write-Step "2/4" "Checking if XML file exists..."
if (-not (Test-Path $XmlFile -PathType Leaf)) {
    Write-Error "XML file not found: $XmlFile"
    exit 1
}
Write-Success "XML file found: $XmlFile"
$XmlFileAbsolute = (Resolve-Path $XmlFile).Path
Write-Host "  Absolute path: $XmlFileAbsolute"
Write-Host ""

# Validate XML file
Write-Step "3/4" "Validating XML file format..."
try {
    $XmlContent = [xml](Get-Content -Path $XmlFileAbsolute)
    Write-Success "XML file is valid"
}
catch {
    Write-Warning "XML validation failed: $($_.Exception.Message)"
    Write-Host "  Continuing anyway. SBE tool will validate the schema."
}
Write-Host ""

# Check if Git is available
Write-Step "4/4" "Checking for Git..."
$GitCmd = Get-Command git -ErrorAction SilentlyContinue
if ($null -eq $GitCmd) {
    Write-Error "Git is not installed or not in PATH"
    Write-Host ""
    Write-Host "Git is required to clone the SBE repository."
    Write-Host "Please install Git and make sure it's in your PATH."
    Write-Host ""
    exit 1
}

try {
    $OldEAP = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    $GitVersion = git --version 2>&1
    $ErrorActionPreference = $OldEAP
    Write-Success "Git found: $GitVersion"
}
catch {
    Write-Warning "Could not retrieve Git version."
}
Write-Host ""

# ============================================================================
# PHASE 2: Setup
# ============================================================================

Write-Header "PHASE 2: Setup"

# Create temporary directories
Write-Step "1/3" "Creating temporary directories..."
Write-Host "  Base temp directory: $TempBasePath"
New-Item -ItemType Directory -Path $TempBasePath -Force | Out-Null
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
Write-Success "Directories created"
Write-Host ""

# Clone SBE repository
Write-Step "2/3" "Cloning SBE repository..."
Write-Host "  URL: $SbeRepoUrl"
Write-Host "  Target: $SbeRepoDir"
try {
    $OldEAP = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    $gitOutput = & git clone --depth 1 $SbeRepoUrl $SbeRepoDir 2>&1
    $ErrorActionPreference = $OldEAP
    
    if ($LASTEXITCODE -ne 0) {
        throw "git clone failed with exit code $LASTEXITCODE"
    }
    
    Write-Success "Repository cloned successfully"
}
catch {
    Write-Error "Failed to clone SBE repository"
    Write-Host ""
    Write-Host "Error: $($_.Exception.Message)"
    Write-Host ""
    Write-Host "Ensure you have git installed and internet access."
    Write-Host ""
    Write-Host "Cleanup command:" -ForegroundColor Yellow
    Write-Host "  Remove-Item -Path `"$TempBasePath`" -Recurse -Force" -ForegroundColor Green
    Write-Host ""
    exit 1
}
Write-Host ""

# Copy XML file
Write-Step "3/3" "Copying XML file to SBE repository..."
Copy-Item -Path $XmlFileAbsolute -Destination (Join-Path $SbeRepoDir $XmlFilename) -Force
Write-Success "XML file copied"
Write-Host "  Source: $XmlFileAbsolute"
Write-Host "  Destination: $(Join-Path $SbeRepoDir $XmlFilename)"
Write-Host ""

# ============================================================================
# PHASE 3: Build SBE Tool
# ============================================================================

Write-Header "PHASE 3: Build SBE Tool with Gradle"

Write-Step "1/1" "Running Gradle build (this may take a while)..."
Write-Host "  Working directory: $SbeRepoDir"
Write-Host ""

Push-Location $SbeRepoDir
try {
    # Determine Gradle command
    $GradleCmd = "gradle"
    if (-not (Get-Command gradle -ErrorAction SilentlyContinue)) {
        $GradlewPath = Join-Path $SbeRepoDir "gradlew.bat"
        if (Test-Path $GradlewPath) {
            $GradleCmd = $GradlewPath
        }
        else {
            Write-Warning "Gradle not found, attempting to use system gradle"
        }
    }

    # Run Gradle build
    $BuildLogPath = Join-Path $TempBasePath "gradle-build.log"
    
    $OldEAP = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    & $GradleCmd assemble 2>&1 | Tee-Object -FilePath $BuildLogPath | Write-Host
    $ErrorActionPreference = $OldEAP
    
    if ($LASTEXITCODE -ne 0) {
        throw "gradle build failed with exit code $LASTEXITCODE"
    }

    Write-Host ""
    Write-Success "Gradle build completed successfully"
}
catch {
    Write-Host ""
    Write-Error "Gradle build failed"
    Write-Host ""
    Write-Host "Build error: $($_.Exception.Message)"
    Write-Host ""
    Write-Host "Possible causes:"
    Write-Host "  - Java version mismatch"
    Write-Host "  - Network issues"
    Write-Host "  - Disk space issues"
    Write-Host ""
    if (Test-Path $BuildLogPath) {
        Write-Host "Build log saved to: $BuildLogPath"
    }
    Write-Host ""
    Write-Host "Cleanup command:" -ForegroundColor Yellow
    Write-Host "  Remove-Item -Path `"$TempBasePath`" -Recurse -Force" -ForegroundColor Green
    Write-Host ""
    Pop-Location
    exit 1
}
finally {
    Pop-Location
}

Write-Host ""

# ============================================================================
# PHASE 4: Generate SBE Codecs
# ============================================================================

Write-Header "PHASE 4: Generate C++ SBE Codecs"

$SbeJarPath = Join-Path $SbeRepoDir "sbe-all\build\libs"
$SbeJar = Get-ChildItem -Path $SbeJarPath -Name "*.jar" -ErrorAction SilentlyContinue | Select-Object -First 1

if ([string]::IsNullOrEmpty($SbeJar)) {
    Write-Error "SBE JAR file not found"
    Write-Host "  Expected location: $SbeJarPath\*.jar"
    Write-Host ""
    Write-Host "Cleanup command:" -ForegroundColor Yellow
    Write-Host "  Remove-Item -Path `"$TempBasePath`" -Recurse -Force" -ForegroundColor Green
    Write-Host ""
    exit 1
}

$SbeJar = Join-Path $SbeJarPath $SbeJar

Write-Step "1/1" "Running SBE code generator..."
Write-Host "  SBE JAR: $SbeJar"
Write-Host "  XML Schema: $XmlFilename"
Write-Host "  XML Location: $(Join-Path $SbeRepoDir $XmlFilename)"
Write-Host "  Output Directory: $OutputDir"
Write-Host "  Target Language: C++"
Write-Host "  Namespace: b3"
Write-Host ""

# Verify XML file exists before running SBE
$XmlFilePath = Join-Path $SbeRepoDir $XmlFilename
if (-not (Test-Path $XmlFilePath)) {
    Write-Error "XML file not found in SBE repository"
    Write-Host "  Expected: $XmlFilePath"
    Write-Host ""
    Write-Host "Cleanup command:" -ForegroundColor Yellow
    Write-Host "  Remove-Item -Path `"$TempBasePath`" -Recurse -Force" -ForegroundColor Green
    Write-Host ""
    exit 1
}

$GenerationLogPath = Join-Path $TempBasePath "sbe-generation.log"
$SbeArgs = @(
    "--add-opens",
    "java.base/jdk.internal.misc=ALL-UNNAMED",
    "-Dsbe.generate.ir=true",
    "-Dsbe.target.language=Cpp",
    "-Dsbe.target.namespace=b3",
    "-Dsbe.output.dir=$OutputDir",
    "-Dsbe.errorLog=yes",
    "-jar",
    $SbeJar,
    $XmlFilePath
)

Push-Location $SbeRepoDir
try {
    # Run SBE command and capture output to log file
    $OldEAP = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    & java @SbeArgs > $GenerationLogPath 2>&1
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = $OldEAP
    Pop-Location
    
    if ($exitCode -eq 0) {
        Write-Host ""
        Write-Success "SBE code generation completed successfully"
    }
    else {
        Write-Host ""
        Write-Error "SBE code generation failed"
        Write-Host ""
        Write-Host "Exit code: $exitCode"
        Write-Host ""
        Write-Host "Possible causes:"
        Write-Host "  - Invalid XML schema"
        Write-Host "  - Unsupported SBE schema version"
        Write-Host "  - Missing required XML elements"
        Write-Host ""
        Write-Host "Generation log saved to: $GenerationLogPath"
        Write-Host ""
        Write-Host "Generation output:"
        Write-Host "---"
        if (Test-Path $GenerationLogPath) {
            Get-Content $GenerationLogPath
        }
        Write-Host "---"
        Write-Host ""
        Write-Host "Cleanup command:" -ForegroundColor Yellow
        Write-Host "  Remove-Item -Path `"$TempBasePath`" -Recurse -Force" -ForegroundColor Green
        Write-Host ""
        exit 1
    }
}
catch {
    Pop-Location
    Write-Host ""
    Write-Error "SBE code generation failed"
    Write-Host ""
    Write-Host "Generation error: $($_.Exception.Message)"
    Write-Host ""
    Write-Host "Generation log saved to: $GenerationLogPath"
    Write-Host ""
    Write-Host "Cleanup command:" -ForegroundColor Yellow
    Write-Host "  Remove-Item -Path `"$TempBasePath`" -Recurse -Force" -ForegroundColor Green
    Write-Host ""
    exit 1
}

Write-Host ""

# ============================================================================
# PHASE 5: Verification and Report
# ============================================================================

Write-Header "PHASE 5: Verification and Report"

# Check if output directory exists
if (-not (Test-Path $OutputDir)) {
    Write-Error "Output directory was not created"
    Write-Host "  Expected: $OutputDir"
    Write-Host ""
    Write-Host "Cleanup command:" -ForegroundColor Yellow
    Write-Host "  Remove-Item -Path `"$TempBasePath`" -Recurse -Force" -ForegroundColor Green
    Write-Host ""
    exit 1
}

# Count generated files
$HeaderCount = @(Get-ChildItem -Path $OutputDir -Recurse -Include *.h, *.hpp -ErrorAction SilentlyContinue).Count
$CppCount = @(Get-ChildItem -Path $OutputDir -Recurse -Include *.cpp -ErrorAction SilentlyContinue).Count

Write-Host "Generated Files:" -ForegroundColor Yellow
Write-Host "  Header files `(.h, .hpp`): $HeaderCount"
Write-Host "  Implementation files `(.cpp`): $CppCount"
Write-Host ""

# Debug: Show directory contents if nothing was generated
if ($HeaderCount -eq 0 -and $CppCount -eq 0) {
    Write-Host "⚠ Debug Information:" -ForegroundColor Yellow
    Write-Host "  Output directory: $OutputDir"
    Write-Host "  Directory exists: YES"
    Write-Host "  Directory contents:"
    
    $DirectoryContents = @(Get-ChildItem -Path $OutputDir -Recurse -ErrorAction SilentlyContinue | Select-Object -First 20)
    if ($DirectoryContents.Count -gt 0) {
        foreach ($item in $DirectoryContents) {
            Write-Host "    - $($item.Name)"
        }
    }
    else {
        Write-Host "    (empty)"
    }
    
    Write-Host ""
    Write-Host "WARNING: No C++ files were generated" -ForegroundColor Red
    Write-Host "  This may indicate:"
    Write-Host "  1. Invalid or unsupported XML schema"
    Write-Host "  2. SBE tool failed silently (check generation log)"
    Write-Host "  3. Output directory permission issue"
    Write-Host ""
    Write-Host "  Generation log: $GenerationLogPath"
    
    if (Test-Path $GenerationLogPath) {
        $LogSize = (Get-Item $GenerationLogPath).Length
        if ($LogSize -gt 0) {
            Write-Host ""
            Write-Host "Log file content:" -ForegroundColor Yellow
            $LogContent = @(Get-Content $GenerationLogPath)
            $LogContent | Select-Object -First 30 | Write-Host
            if ($LogContent.Count -gt 30) {
                Write-Host "  ... `(truncated, see full log for details`)"
            }
        }
        else {
            Write-Host "  Log file is empty - SBE tool produced no output"
        }
    }
    Write-Host ""
}
else {
    Write-Success "Files generated successfully"
}

# Success message
Write-Host ""
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Green
Write-Host "SUCCESS: SBE Codecs Generated Successfully" -ForegroundColor Green
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Green
Write-Host ""

Write-Host "Output Directory:" -ForegroundColor Cyan
Write-Host "  $OutputDir" -ForegroundColor Green
Write-Host ""

Write-Host "Next Steps:" -ForegroundColor Cyan
Write-Host "  1. Review the generated C++ files"
Write-Host "  2. Copy the desired headers/files to your project`'s include/sbe directory"
Write-Host "  3. Update your CMakeLists.txt if needed to include these files in your build"
Write-Host ""

Write-Host "Temporary Files:" -ForegroundColor Cyan
Write-Host "  Repository: $SbeRepoDir"
Write-Host "  Logs:"
Write-Host "    - Gradle: $BuildLogPath"
Write-Host "    - SBE Generation: $GenerationLogPath"
Write-Host ""
Write-Host "Cleanup:" -ForegroundColor Cyan
Write-Host "  Manual cleanup:" -ForegroundColor Yellow
Write-Host "    Remove-Item -Path `"$TempBasePath`" -Recurse -Force" -ForegroundColor Green
Write-Host ""
Write-Host "  Or use the interactive cleanup command to remove all SBE generation directories:" -ForegroundColor Yellow
Write-Host "    .\\generate-sbe-files.ps1 clean" -ForegroundColor Green
Write-Host ""
