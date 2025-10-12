#!/bin/bash

# run-all.sh - Automation script for data generation, building, and execution
# Usage: ./run-all.sh <output_dir>

set -e  # Exit on any error

# ANSI color codes for output
RESET='\033[0m'
BOLD='\033[1m'
GREEN='\033[32m'
BLUE='\033[34m'
YELLOW='\033[33m'
RED='\033[31m'
CYAN='\033[36m'

# Function to print colored output
print_step() {
    echo -e "${BOLD}${BLUE}==>${RESET} ${BOLD}$1${RESET}"
}

print_success() {
    echo -e "${BOLD}${GREEN}✅ $1${RESET}"
}

print_error() {
    echo -e "${BOLD}${RED}❌ $1${RESET}"
}

print_info() {
    echo -e "${CYAN}ℹ️  $1${RESET}"
}

# Check if output directory is provided
if [ $# -eq 0 ]; then
    print_error "Usage: $0 <output_dir>"
    echo -e "${YELLOW}Example: $0 ./my_output${RESET}"
    exit 1
fi

OUTPUT_DIR="$1"
TARGET_NAME="data-generator"
BUILD_DIR="build"
EXECUTABLE="$BUILD_DIR/$TARGET_NAME"

# Get the absolute path of the script directory (data-generator root)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

print_step "Starting automated build process..."
print_info "Output directory: $OUTPUT_DIR"
print_info "Working directory: $SCRIPT_DIR"

# Step 1: Run the data generator
print_step "Step 1: Running data generator..."
if [ ! -f "server/run-gen.sh" ]; then
    print_error "server/run-gen.sh not found!"
    exit 1
fi

cd server
if [ ! -f "server.jar" ]; then
    print_error "server.jar not found in server directory!"
    exit 1
fi

print_info "Executing server/run-gen.sh..."
bash run-gen.sh
print_success "Data generation completed"

# Return to project root
cd "$SCRIPT_DIR"

# Step 2: Build the project
print_step "Step 2: Building project with make..."
if [ ! -f "makefile" ]; then
    print_error "makefile not found!"
    exit 1
fi

make
print_success "Project build completed"

# Step 3: Check if executable exists
print_step "Step 3: Verifying executable..."
if [ ! -f "$EXECUTABLE" ]; then
    print_error "Executable $EXECUTABLE not found after build!"
    exit 1
fi

print_success "Executable found: $EXECUTABLE"

# Step 4: Create output directory if it doesn't exist
print_step "Step 4: Preparing output directory..."
if [ ! -d "$OUTPUT_DIR" ]; then
    print_info "Creating output directory: $OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"
fi

# Step 5: Run the executable
print_step "Step 5: Running executable..."
print_info "Command: ./$EXECUTABLE"
print_info "Output will be saved to: $OUTPUT_DIR"

# Run the executable and capture its exit code
if ./"$EXECUTABLE" "$OUTPUT_DIR"; then
    print_success "Execution completed successfully!"
else
    EXIT_CODE=$?
    print_error "Execution failed with exit code: $EXIT_CODE"
    exit $EXIT_CODE
fi

print_step "All steps completed successfully! 🎉"
print_info "Generated data: server/generated (or as specified by run-gen.sh)"
print_info "Built executable: $EXECUTABLE"
print_info "Output directory: $OUTPUT_DIR"
