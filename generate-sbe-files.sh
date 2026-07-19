#!/bin/bash

# Generate SBE C++ codec files from B3 UMDF XML schema.
#
# Copyright (c) 2026, Augusto Damasceno.
# All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause

#############################################################################
# Generate SBE C++ Codec Files from B3 UMDF XML Schema
#
# Usage:
#   ./generate-sbe-files.sh <path-to-xml-file>
#
# Example:
#   ./generate-sbe-files.sh schemas/b3/b3-market-data-messages-2.3.1.xml
#
# This script will:
#   1. Validate Java is installed
#   2. Clone the SBE repository to a temporary directory
#   3. Build the SBE tool with Gradle
#   4. Generate C++ codec files from the B3 UMDF XML schema
#   5. Display the output location
#############################################################################

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEMP_BASE_DIR="${TMPDIR:-/tmp}/sbe-generation-$$"
SBE_REPO_URL="https://github.com/aeron-io/simple-binary-encoding.git"
SBE_REPO_DIR="${TEMP_BASE_DIR}/simple-binary-encoding"
OUTPUT_DIR="${TEMP_BASE_DIR}/output"
XML_FILENAME="b3-market-data-messages-2.3.1.xml"

# Parse arguments
if [[ $# -eq 0 ]]; then
    echo -e "${RED}ERROR: Missing required argument${NC}"
    echo "Usage: $0 <path-to-xml-file>"
    echo "       $0 clean"
    echo ""
    echo "Examples:"
    echo "  $0 ./b3-market-data-messages-2.3.1.xml"
    echo "  $0 clean                    # Remove all temporary SBE generation directories"
    exit 1
fi

XML_FILE="$1"

# Check if user wants to clean up temporary directories
if [[ "$XML_FILE" == "clean" ]]; then
    echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}Cleaning up temporary SBE generation directories${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    TEMP_PATTERN="${TMPDIR:-/tmp}/sbe-generation-*"
    FOUND_DIRS=0
    DIRS_TO_DELETE=()
    
    # First, find and list all directories
    for temp_dir in $TEMP_PATTERN; do
        if [[ -d "$temp_dir" ]]; then
            DIRS_TO_DELETE+=("$temp_dir")
            FOUND_DIRS=$((FOUND_DIRS + 1))
        fi
    done
    
    if [[ $FOUND_DIRS -eq 0 ]]; then
        echo -e "${YELLOW}No temporary SBE generation directories found.${NC}"
        echo ""
        exit 0
    fi
    
    # Display directories to be deleted
    echo -e "${YELLOW}The following director$([ $FOUND_DIRS -eq 1 ] && echo 'y' || echo 'ies') will be deleted:${NC}"
    echo ""
    for dir in "${DIRS_TO_DELETE[@]}"; do
        echo -e "  ${RED}$dir${NC}"
    done
    echo ""
    
    # Ask for confirmation
    echo -e "${YELLOW}Are you sure you want to delete these $([ $FOUND_DIRS -eq 1 ] && echo 'directory' || echo 'directories')? (yes/no)${NC}"
    read -p "Enter your choice: " -r user_choice
    echo ""
    
    if [[ "$user_choice" != "yes" ]]; then
        echo -e "${YELLOW}Cleanup cancelled.${NC}"
        echo ""
        exit 0
    fi
    
    # Delete confirmed directories
    DELETED_COUNT=0
    for temp_dir in "${DIRS_TO_DELETE[@]}"; do
        echo -e "${YELLOW}Removing:${NC} $temp_dir"
        if rm -rf "$temp_dir" 2>/dev/null; then
            echo -e "${GREEN}✓ Removed${NC}"
            DELETED_COUNT=$((DELETED_COUNT + 1))
        else
            echo -e "${RED}✗ Failed to remove${NC}"
        fi
        echo ""
    done
    
    echo -e "${GREEN}Cleanup completed. ${DELETED_COUNT} director$([ $DELETED_COUNT -eq 1 ] && echo 'y' || echo 'ies') removed.${NC}"
    echo ""
    exit 0
fi

# ============================================================================
# PHASE 1: Validation
# ============================================================================
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}PHASE 1: Validation${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Check if Java is installed
echo -e "${YELLOW}[1/4]${NC} Checking if Java is installed..."
if ! command -v java &> /dev/null; then
    echo -e "${RED}ERROR: Java is not installed or not in PATH${NC}"
    echo ""
    echo "Please install Java (JDK 11 or later) and make sure it's accessible via the 'java' command."
    echo ""
    
    # Detect OS and provide installation instructions
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Linux
        if command -v apt-get &> /dev/null; then
            # Debian/Ubuntu
            echo -e "${YELLOW}Detected: Debian/Ubuntu${NC}"
            echo "Install Java with:"
            echo "  ${GREEN}sudo apt-get update && sudo apt-get install -y openjdk-17-jdk${NC}"
            echo ""
        elif command -v dnf &> /dev/null; then
            # Fedora/RHEL
            echo -e "${YELLOW}Detected: Fedora/RHEL${NC}"
            echo "Install Java with:"
            echo "  ${GREEN}sudo dnf install -y java-17-openjdk-devel${NC}"
            echo ""
        elif command -v pacman &> /dev/null; then
            # Arch Linux
            echo -e "${YELLOW}Detected: Arch Linux${NC}"
            echo "Install Java with:"
            echo "  ${GREEN}sudo pacman -S jdk-openjdk${NC}"
            echo ""
        elif command -v yum &> /dev/null; then
            # CentOS/RHEL
            echo -e "${YELLOW}Detected: CentOS/RHEL${NC}"
            echo "Install Java with:"
            echo "  ${GREEN}sudo yum install -y java-17-openjdk-devel${NC}"
            echo ""
        else
            # Generic Linux
            echo -e "${YELLOW}Detected: Linux (distribution unknown)${NC}"
            echo "Install Java using your package manager or visit:"
            echo "  https://openjdk.org/install/"
            echo ""
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        echo -e "${YELLOW}Detected: macOS${NC}"
        echo "Install Java with Homebrew:"
        echo "  ${GREEN}brew install openjdk@17${NC}"
        echo ""
        echo "Then add it to your PATH:"
        echo "  ${GREEN}sudo ln -sfn /opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk /Library/Java/JavaVirtualMachines/openjdk-17.jdk${NC}"
        echo ""
    elif [[ "$OSTYPE" == "freebsd"* ]]; then
        # FreeBSD
        echo -e "${YELLOW}Detected: FreeBSD${NC}"
        echo "Install Java with:"
        echo "  ${GREEN}sudo pkg install -y openjdk17${NC}"
        echo ""
    else
        echo "For other systems, visit: https://openjdk.org/install/"
        echo ""
    fi
    
    echo "After installation, verify Java is available:"
    echo "  ${GREEN}java -version${NC}"
    echo ""
    echo -e "${YELLOW}Cleanup command (if temporary files were created):${NC}"
    echo -e "  ${GREEN}sudo apt-get update && sudo apt-get install -y openjdk-17-jdk${NC}"
    echo ""
    exit 1
fi

JAVA_VERSION=$(java -version 2>&1 | head -n 1)
echo -e "${GREEN}✓ Java found: ${JAVA_VERSION}${NC}"
echo ""

# Check if XML file exists
echo -e "${YELLOW}[2/4]${NC} Checking if XML file exists..."
if [[ ! -f "$XML_FILE" ]]; then
    echo -e "${RED}ERROR: XML file not found: ${XML_FILE}${NC}"
    echo ""
    echo -e "${YELLOW}Cleanup command:${NC}"
    echo -e "  ${GREEN}rm -rf ${TEMP_BASE_DIR}${NC}"
    echo ""
    exit 1
fi

echo -e "${GREEN}✓ XML file found: ${XML_FILE}${NC}"
XML_FILE_ABS="$(cd "$(dirname "$XML_FILE")" && pwd)/$(basename "$XML_FILE")"
echo -e "  Absolute path: ${XML_FILE_ABS}"
echo ""

# Validate XML file is valid
echo -e "${YELLOW}[3/4]${NC} Validating XML file format..."
if ! xml_file_valid=$(xmllint --noout "$XML_FILE_ABS" 2>&1); then
    echo -e "${YELLOW}⚠ XML validation warning (xmllint not available or XML has issues):${NC}"
    echo "  Note: Continuing anyway. SBE tool will validate the schema."
else
    echo -e "${GREEN}✓ XML file is valid${NC}"
fi
echo ""

# Check if Gradle is available (optional, but recommended)
echo -e "${YELLOW}[4/4]${NC} Checking for Gradle..."
if command -v gradle &> /dev/null; then
    echo -e "${GREEN}✓ Gradle found: $(gradle --version 2>&1 | head -n 1)${NC}"
else
    echo -e "${YELLOW}⚠ Gradle not found in PATH (will attempt to use gradlew from repository)${NC}"
fi
echo ""

# ============================================================================
# PHASE 2: Setup
# ============================================================================
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}PHASE 2: Setup${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

echo -e "${YELLOW}[1/3]${NC} Creating temporary directories..."
echo "  Base temp directory: ${TEMP_BASE_DIR}"
mkdir -p "$TEMP_BASE_DIR"
mkdir -p "$OUTPUT_DIR"
echo -e "${GREEN}✓ Directories created${NC}"
echo ""

echo -e "${YELLOW}[2/3]${NC} Cloning SBE repository..."
echo "  URL: ${SBE_REPO_URL}"
echo "  Target: ${SBE_REPO_DIR}"
if git clone --depth 1 "$SBE_REPO_URL" "$SBE_REPO_DIR" 2>&1; then
    echo -e "${GREEN}✓ Repository cloned successfully${NC}"
else
    echo -e "${RED}ERROR: Failed to clone SBE repository${NC}"
    echo "  Ensure you have git installed and internet access."
    echo ""
    echo -e "${YELLOW}Cleanup command:${NC}"
    echo -e "  ${GREEN}rm -rf ${TEMP_BASE_DIR}${NC}"
    echo ""
    exit 1
fi
echo ""

echo -e "${YELLOW}[3/3]${NC} Copying XML file to SBE repository..."
cp "$XML_FILE_ABS" "$SBE_REPO_DIR/$XML_FILENAME"
echo -e "${GREEN}✓ XML file copied${NC}"
echo "  Source: ${XML_FILE_ABS}"
echo "  Destination: ${SBE_REPO_DIR}/${XML_FILENAME}"
echo ""

# ============================================================================
# PHASE 3: Build SBE Tool
# ============================================================================
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}PHASE 3: Build SBE Tool with Gradle${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

cd "$SBE_REPO_DIR"

echo -e "${YELLOW}[1/1]${NC} Running Gradle build (this may take a while)..."
echo "  Working directory: $(pwd)"
echo ""

if command -v gradle &> /dev/null; then
    GRADLE_CMD="gradle"
else
    GRADLE_CMD="./gradlew"
    chmod +x ./gradlew 2>/dev/null || true
fi

if ! $GRADLE_CMD assemble 2>&1 | tee "$TEMP_BASE_DIR/gradle-build.log"; then
    echo ""
    echo -e "${RED}ERROR: Gradle build failed${NC}"
    echo ""
    echo "Build log saved to: ${TEMP_BASE_DIR}/gradle-build.log"
    echo ""
    echo "Possible causes:"
    echo "  - Java version mismatch"
    echo "  - Network issues"
    echo "  - Disk space issues"
    echo ""
    echo "Please check the build log above for more details."
    echo ""
    echo -e "${YELLOW}Cleanup command:${NC}"
    echo -e "  ${GREEN}rm -rf ${TEMP_BASE_DIR}${NC}"
    echo ""
    exit 1
fi

# ============================================================================
# PHASE 4: Generate SBE Codecs
# ============================================================================
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}PHASE 4: Generate C++ SBE Codecs${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

SBE_JAR=$(find "${SBE_REPO_DIR}/sbe-all/build/libs/" -maxdepth 1 -name "*.jar" -type f | head -1)

if [[ -z "$SBE_JAR" ]] || [[ ! -f "$SBE_JAR" ]]; then
    echo -e "${RED}ERROR: SBE JAR file not found${NC}"
    echo "  Expected location: ${SBE_REPO_DIR}/sbe-all/build/libs/*.jar"
    echo ""
    echo -e "${YELLOW}Cleanup command:${NC}"
    echo -e "  ${GREEN}rm -rf ${TEMP_BASE_DIR}${NC}"
    echo ""
    exit 1
fi
echo "  SBE JAR: ${SBE_JAR}"
echo "  XML Schema: ${XML_FILENAME}"
echo "  XML Location: ${SBE_REPO_DIR}/${XML_FILENAME}"
echo "  Output Directory: ${OUTPUT_DIR}"
echo "  Target Language: C++"
echo "  Namespace: b3"
echo ""

# Verify XML file exists before running SBE
if [[ ! -f "${SBE_REPO_DIR}/${XML_FILENAME}" ]]; then
    echo -e "${RED}ERROR: XML file not found in SBE repository${NC}"
    echo "  Expected: ${SBE_REPO_DIR}/${XML_FILENAME}"
    echo ""
    echo -e "${YELLOW}Cleanup command:${NC}"
    echo -e "  ${GREEN}rm -rf ${TEMP_BASE_DIR}${NC}"
    echo ""
    exit 1
fi

SBE_CMD=(
    "java"
    "--add-opens"
    "java.base/jdk.internal.misc=ALL-UNNAMED"
    "-Dsbe.generate.ir=true"
    "-Dsbe.target.language=Cpp"
    "-Dsbe.target.namespace=b3"
    "-Dsbe.output.dir=${OUTPUT_DIR}"
    "-Dsbe.errorLog=yes"
    "-jar"
    "$SBE_JAR"
    "${SBE_REPO_DIR}/${XML_FILENAME}"
)

# Run SBE command and capture output to log file
if "${SBE_CMD[@]}" > "$TEMP_BASE_DIR/sbe-generation.log" 2>&1; then
    echo ""
    echo -e "${GREEN}✓ SBE code generation completed successfully${NC}"
else
    echo ""
    echo -e "${RED}ERROR: SBE code generation failed${NC}"
    echo ""
    echo "Generation log saved to: ${TEMP_BASE_DIR}/sbe-generation.log"
    echo ""
    echo "Possible causes:"
    echo "  - Invalid XML schema"
    echo "  - Unsupported SBE schema version"
    echo "  - Missing required XML elements"
    echo ""
    echo "Generation output:"
    echo "---"
    cat "$TEMP_BASE_DIR/sbe-generation.log"
    echo "---"
    echo ""
    echo -e "${YELLOW}Cleanup command:${NC}"
    echo -e "  ${GREEN}rm -rf ${TEMP_BASE_DIR}${NC}"
    echo ""
    exit 1
fi

# ============================================================================
# PHASE 5: Verify and Report
# ============================================================================
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}PHASE 5: Verification and Report${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Check if output directory exists and has content
if [[ ! -d "$OUTPUT_DIR" ]]; then
    echo -e "${RED}ERROR: Output directory was not created${NC}"
    echo "  Expected: ${OUTPUT_DIR}"
    echo ""
    echo -e "${YELLOW}Cleanup command:${NC}"
    echo -e "  ${GREEN}rm -rf ${TEMP_BASE_DIR}${NC}"
    echo ""
    exit 1
fi

# Count generated files
HEADER_COUNT=$(find "$OUTPUT_DIR" -type f \( -name "*.h" -o -name "*.hpp" \) 2>/dev/null | wc -l)
CPP_COUNT=$(find "$OUTPUT_DIR" -type f -name "*.cpp" 2>/dev/null | wc -l)

echo -e "${YELLOW}Generated Files:${NC}"
echo "  Header files (.h, .hpp): ${HEADER_COUNT}"
echo "  Implementation files (.cpp): ${CPP_COUNT}"
echo ""

# Debug: List all files in output directory
if [[ $HEADER_COUNT -eq 0 ]] && [[ $CPP_COUNT -eq 0 ]]; then
    echo -e "${YELLOW}⚠ Debug Information:${NC}"
    echo "  Output directory: ${OUTPUT_DIR}"
    echo "  Directory exists: $([ -d "$OUTPUT_DIR" ] && echo 'YES' || echo 'NO')"
    echo "  Directory contents:"
    if find "$OUTPUT_DIR" -type f 2>/dev/null | head -20 | while read file; do
        echo "    - $file"
    done | grep -q .; then
        find "$OUTPUT_DIR" -type f 2>/dev/null | head -20 | while read file; do
            echo "    - $file"
        done
    else
        echo "    (empty)"
    fi
    echo ""
    echo -e "${RED}WARNING: No C++ files were generated${NC}"
    echo "  This may indicate:"
    echo "  1. Invalid or unsupported XML schema"
    echo "  2. SBE tool failed silently (check generation log)"
    echo "  3. Output directory permission issue"
    echo ""
    echo "  Generation log: ${TEMP_BASE_DIR}/sbe-generation.log"
    if [[ -s "$TEMP_BASE_DIR/sbe-generation.log" ]]; then
        echo ""
        echo -e "${YELLOW}Log file content:${NC}"
        cat "$TEMP_BASE_DIR/sbe-generation.log" | head -30
        if [ $(wc -l < "$TEMP_BASE_DIR/sbe-generation.log") -gt 30 ]; then
            echo "  ... (truncated, see full log for details)"
        fi
    else
        echo "  Log file is empty - SBE tool produced no output"
    fi
    echo ""
else
    echo -e "${GREEN}✓ Files generated successfully${NC}"
fi

echo ""
echo -e "${GREEN}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}SUCCESS: SBE Codecs Generated Successfully${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${BLUE}Output Directory:${NC}"
echo -e "  ${GREEN}${OUTPUT_DIR}${NC}"
echo ""
echo -e "${BLUE}Next Steps:${NC}"
echo "  1. Review the generated C++ files"
echo "  2. Copy the desired headers/files to your project's include/sbe directory"
echo "  3. Update your CMakeLists.txt if needed to include these files in your build"
echo ""
echo -e "${BLUE}Temporary Files:${NC}"
echo "  Repository: ${SBE_REPO_DIR}"
echo "  Logs:"
echo "    - Gradle: ${TEMP_BASE_DIR}/gradle-build.log"
echo "    - SBE Generation: ${TEMP_BASE_DIR}/sbe-generation.log"
echo ""
echo -e "${BLUE}Cleanup:${NC}"
echo "  Manual cleanup:"
echo -e "    ${GREEN}rm -rf ${TEMP_BASE_DIR}${NC}"
echo ""
echo "  Or use the interactive cleanup command to remove all SBE generation directories:"
echo -e "    ${GREEN}./generate-sbe-files.sh clean${NC}"
echo ""
