# Data Generator

A tool for generating C++ header files from Minecraft server data reports. This project includes a Minecraft server data generator and a C++ code generator that converts JSON reports into usable C++ headers.

## Overview

This tool consists of two main components:

1. **Minecraft Data Generator**: Uses the official Minecraft server to generate JSON reports containing game data
2. **C++ Code Generator**: Converts the JSON reports into C++ header files for use in Minecraft-related projects

## Prerequisites

- Java (for running the Minecraft server)
- C++ compiler with C++17 support (g++)
- Make utility

## Project Structure

```
data-generator/
├── server.jar              # Minecraft server jar file
├── run-gen.sh              # Script to generate JSON reports
├── generated/              # Generated JSON reports from server
│   ├── data/              # Server data files
│   └── reports/           # JSON reports (blocks.json, items.json, etc.)
├── data_generator_tools/   # C++ code generator
│   ├── src/               # Source files
│   ├── include/           # Header files
│   ├── generated/         # Generated C++ headers
│   ├── bin/               # Compiled executables
│   └── makefile           # Build configuration
└── logs/                  # Server logs
```

## Usage

### Step 1: Generate JSON Reports

First, generate the JSON data reports using the Minecraft server:

```bash
# Make the script executable (if needed)
chmod +x run-gen.sh

# Run the data generator
./run-gen.sh
```

This will:
- Start the Minecraft server in data generation mode
- Generate JSON reports in the `generated/reports/` directory
- Create files like `blocks.json`, `items.json`, `registries.json`, etc.

### Step 2: Build the C++ Code Generator

Navigate to the C++ tools directory and build the generator:

```bash
cd data_generator_tools
make clean
make
```

This creates the executable at `bin/data_generator_tools`.

### Step 3: Generate C++ Headers

Run the code generator to create C++ header files:

```bash
# Generate all headers (default)
./bin/data_generator_tools

# Or specify an output directory
./bin/data_generator_tools /path/to/output

# Generate specific components
./bin/data_generator_tools generated items
./bin/data_generator_tools generated blocks
./bin/data_generator_tools generated block-types
./bin/data_generator_tools generated properties
./bin/data_generator_tools generated block-entities
./bin/data_generator_tools generated all
```

## Command Line Options

The C++ generator accepts the following arguments:

```bash
./bin/data_generator_tools [output_directory] [command]
```

**Arguments:**
- `output_directory` (optional): Directory to place generated headers (default: "generated")
- `command` (optional): Specific component to generate

**Available Commands:**
- `items` - Generate items.hpp with item definitions
- `blocks` - Generate blocks.hpp with block definitions  
- `block-types` - Generate block_types.hpp with block type enums
- `properties` - Generate properties.hpp with block property definitions
- `block-entities` - Generate block_entity_types.hpp with block entity types
- `all` - Generate all header files

**Default Behavior:**
If no command is specified, the tool generates block entity types only.

## Generated Files

The C++ generator creates the following header files:

- **`items.hpp`** - Item definitions and mappings
- **`blocks.hpp`** - Block definitions with properties
- **`block_types.hpp`** - Enumeration of all block types
- **`properties.hpp`** - Block property definitions
- **`block_entity_types.hpp`** - Block entity type mappings
- **`block_state_functions.hpp`** - Functions for block state manipulation
- **`property_functions.hpp`** - Functions for property handling

## Examples

### Basic Usage

```bash
# 1. Generate server data
./run-gen.sh

# 2. Build the generator
cd data_generator_tools
make

# 3. Generate all C++ headers
./bin/data_generator_tools
```

### Custom Output Directory

```bash
./bin/data_generator_tools ./my_headers all
```

### Generate Specific Components

```bash
# Generate only items
./bin/data_generator_tools generated items

# Generate only blocks
./bin/data_generator_tools generated blocks
```

## Troubleshooting

### Common Issues

1. **"Failed to open file: generated/reports/blocks.json"**
   - Ensure you've run `./run-gen.sh` first to generate the JSON reports
   - Check that the `generated/reports/` directory exists and contains JSON files
   - Verify you're running the C++ generator from the correct directory

2. **Build errors**
   - Ensure you have a C++17 compatible compiler
   - Make sure all header files are present in the `include/` directory

3. **Permission errors**
   - Make sure `run-gen.sh` is executable: `chmod +x run-gen.sh`
   - Check file permissions in the generated directories

### File Paths

The C++ generator expects to be run from the `data_generator_tools/` directory and looks for JSON reports at `../generated/reports/`. If you encounter path issues, ensure:

1. You're running the executable from the `data_generator_tools/` directory
2. The JSON reports exist at `../generated/reports/` relative to the executable
3. All required JSON files are present (blocks.json, items.json, registries.json)

## Development

### Adding New Generators

To add new code generation functionality:

1. Add a new method to the `DataGenerator` class in `src/main.cpp`
2. Implement the JSON parsing and C++ code generation logic
3. Add the command to the main function's command parsing
4. Update this README with the new command

### Modifying Output Format

The generated headers use specific C++ patterns. To modify the output:

1. Edit the relevant `do*()` methods in `src/main.cpp`
2. Modify the string output statements to change the generated code format
3. Rebuild and test with your target C++ project

## License

This project uses the Minecraft server jar file, which is subject to Minecraft's End User License Agreement (EULA). By using this tool, you agree to Minecraft's EULA terms.