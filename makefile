TARGET_NAME := data-generator
BUILD_DIR       := build
SOURCE_DIR      := src
INCLUDE_DIR     := include
DEPS_DIR        := .deps

TARGET          := $(BUILD_DIR)/$(TARGET_NAME)

CXX             := g++
CXXFLAGS        := -std=c++20 -Wall -Wextra -Wpedantic -O2
DEBUG_FLAGS     := -g -DDEBUG -O0
RELEASE_FLAGS   := -DNDEBUG -O3
INCLUDE_FLAGS   := -I$(INCLUDE_DIR)

# Linker flags (add your libraries here)
LDFLAGS         :=
LIBS            :=

# ================================ COLOR SETUP ===============================
# ANSI color codes for beautiful output
RESET           := \033[0m
BOLD            := \033[1m
DIM             := \033[2m

# Text colors
BLACK           := \033[30m
RED             := \033[31m
GREEN           := \033[32m
YELLOW          := \033[33m
BLUE            := \033[34m
MAGENTA         := \033[35m
CYAN            := \033[36m
WHITE           := \033[37m

# Background colors
BG_BLACK        := \033[40m
BG_RED          := \033[41m
BG_GREEN        := \033[42m
BG_YELLOW       := \033[43m
BG_BLUE         := \033[44m
BG_MAGENTA      := \033[45m
BG_CYAN         := \033[46m
BG_WHITE        := \033[47m

# Bright colors
BRIGHT_RED      := \033[91m
BRIGHT_GREEN    := \033[92m
BRIGHT_YELLOW   := \033[93m
BRIGHT_BLUE     := \033[94m
BRIGHT_MAGENTA  := \033[95m
BRIGHT_CYAN     := \033[96m
BRIGHT_WHITE    := \033[97m

# ============================= FILE DISCOVERY ==============================
# Automatically find all source files recursively
SOURCES         := $(shell find $(SOURCE_DIR) -name "*.cpp" -type f)
HEADERS         := $(shell find $(INCLUDE_DIR) -name "*.hpp" -type f)
OBJECTS         := $(patsubst $(SOURCE_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS            := $(patsubst $(SOURCE_DIR)/%.cpp,$(DEPS_DIR)/%.d,$(SOURCES))

# Create the main executable
$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	@printf "$(BOLD)$(BRIGHT_CYAN)🔗 Linking executable: $(BRIGHT_WHITE)$@$(RESET)\n"
	@$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS)
	@printf "$(BOLD)$(BRIGHT_GREEN)✅ Build completed successfully!$(RESET)\n"
	@printf "$(BOLD)$(BRIGHT_BLUE)📁 Executable: $(BRIGHT_WHITE)$@$(RESET)\n"

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp $(DEPS_DIR)/%.d | $(BUILD_DIR) $(DEPS_DIR)
	@printf "$(BOLD)$(BRIGHT_BLUE)🔨 Compiling: $(BRIGHT_WHITE)$<$(RESET)\n"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

# Generate dependency files
$(DEPS_DIR)/%.d: $(SOURCE_DIR)/%.cpp | $(DEPS_DIR)
	@printf "$(DIM)$(CYAN)📋 Generating dependencies: $<$(RESET)\n"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -MM -MT $(BUILD_DIR)/$*.o $< > $@

# Create build directories
$(BUILD_DIR):
	@printf "$(BOLD)$(YELLOW)📁 Creating build directory: $(BRIGHT_WHITE)$@$(RESET)\n"
	@mkdir -p $@

$(DEPS_DIR):
	@printf "$(DIM)$(YELLOW)📁 Creating deps directory: $(BRIGHT_WHITE)$@$(RESET)\n"
	@mkdir -p $@

# Include dependency files (only if they exist)
-include $(DEPS)

# Generate compile_commands.json for LSP support
compile_commands:
	@printf "$(BOLD)$(BRIGHT_BLUE)📝 Generating compile_commands.json for LSP...$(RESET)\n"
	@echo '[' > compile_commands.json
	@first=true; for src in $(SOURCES); do \
		[ "$$first" = true ] && first=false || echo ',' >> compile_commands.json; \
		echo '  {' >> compile_commands.json; \
		echo '    "directory": "'$(shell pwd)'",' >> compile_commands.json; \
		obj_path=$$(echo "$$src" | sed 's|$(SOURCE_DIR)/|$(BUILD_DIR)/|' | sed 's|\.cpp$$|.o|'); \
		echo "    \"command\": \"$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $$src -o $$obj_path\"," >> compile_commands.json; \
		echo '    "file": "'$$src'"' >> compile_commands.json; \
		echo '  }' >> compile_commands.json; \
	done
	@echo ']' >> compile_commands.json
	@printf "$(BOLD)$(BRIGHT_GREEN)✅ compile_commands.json generated successfully!$(RESET)\n"
