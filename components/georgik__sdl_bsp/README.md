# ESP-BSP-SDL Abstraction Layer

This component provides a board-agnostic abstraction layer between SDL and ESP-BSP (Board Support Package). It solves the problem of having to specify board configurations via command-line parameters by moving board selection into ESP-IDF's native Kconfig system.

## Problem Solved

**Before**: Developers had to use complex command-line syntax for every build:
```bash
idf.py @boards/esp-box-3.cfg build flash monitor
```

**After**: Simple, standard ESP-IDF workflow:
```bash
idf.py menuconfig  # Select board once
idf.py build flash monitor  # Use for all subsequent builds
```

## Architecture

```
┌─────────────────┐
│ SDL Application │
└─────────┬───────┘
          │
┌─────────▼───────┐
│  SDL Component  │ (Board-agnostic)
└─────────┬───────┘
          │
┌─────────▼───────┐
│ ESP-BSP-SDL     │ (Abstraction Layer)
│ Component       │
└─────────┬───────┘
          │
┌─────────▼───────┐
│ ESP-BSP         │ (Board-specific)
│ Components      │
└─────────────────┘
```

## Key Benefits

1. **Board Agnostic**: SDL no longer needs to know about specific boards
2. **Clean Separation**: BSP-specific code is isolated in the abstraction layer  
3. **Easy Configuration**: Board selection through menuconfig, no command-line args needed
4. **Maintainable**: Adding new boards only requires implementing the abstraction interface
5. **Portable**: The same SDL code works across all supported boards
6. **Flexible**: Easy to switch boards without changing build commands

## Usage

### 1. Select Your Board
```bash
idf.py menuconfig
```
Navigate to: `ESP-BSP SDL Configuration` → `Select Target Board`

Choose from supported boards:
- **ESP-Box-3** - ESP32-S3 with 320x240 display and touch
- **M5 Atom S3** - Compact ESP32-S3 with 128x128 round display
- **M5Stack Core S3** - ESP32-S3 with 320x240 display and touch
- **ESP32-P4 Function EV Board** - High-performance ESP32-P4 with 1280x800 MIPI-DSI
- **ESP32-S3-LCD-EV-Board** - ESP32-S3 evaluation board with 800x480 RGB display
- **M5Stack Tab5** - ESP32-P4 tablet with 720x1280 MIPI-DSI display
- **ESP32-C6 DevKit** - Generic BSP for ESP32-C6 with external display
- **ESP32-C3-LCDkit** - ESP32-C3 development kit with LCD

### 2. Build and Flash
```bash
idf.py build flash monitor
```

That's it! No need to specify `@boards/BOARD.cfg` anymore.

### 3. Switching Boards
To switch to a different board:
1. Run `idf.py menuconfig`
2. Change the board selection
3. Run `idf.py build flash monitor`

The abstraction layer will automatically handle the board-specific configuration.

## Installation

### Option 1: ESP Component Registry (Recommended)
```bash
idf.py add-dependency "georgik/sdl_bsp"
```

### Option 2: Git Submodule
```bash
cd components
git submodule add https://github.com/georgik/esp-idf-component-SDL_bsp.git georgik__sdl_bsp
```

### Option 3: Manual Download
1. Download from GitHub: https://github.com/georgik/esp-idf-component-SDL_bsp
2. Extract to `components/georgik__sdl_bsp/` in your project

## Quick Start

### 1. Add to Your Project
```bash
# Add the component
idf.py add-dependency "georgik/sdl_bsp"

# Configure your board
idf.py menuconfig
```

### 2. Select Board in Menuconfig
Navigate to: `ESP-BSP SDL Configuration` → `Select Target Board`

### 3. Use in Your Code
```c
#include "esp_bsp_sdl.h"

void app_main(void)
{
    // Initialize display and get configuration
    esp_bsp_sdl_display_config_t config;
    esp_lcd_panel_handle_t panel_handle;
    esp_lcd_panel_io_handle_t panel_io_handle;
    
    ESP_ERROR_CHECK(esp_bsp_sdl_init(&config, &panel_handle, &panel_io_handle));
    
    printf("Display: %dx%d, Touch: %s\n", 
           config.width, config.height, 
           config.has_touch ? "Yes" : "No");
    
    // Turn on backlight
    ESP_ERROR_CHECK(esp_bsp_sdl_backlight_on());
    
    // Your SDL code here...
}
```

## Implementation Details

### Component Structure
```
components/esp_bsp_sdl/
├── include/
│   └── esp_bsp_sdl.h          # Public API
├── src/
│   ├── esp_bsp_sdl_common.c   # Common implementation
│   └── boards/                # Board-specific implementations
│       ├── esp_bsp_sdl_esp_box_3.c
│       ├── esp_bsp_sdl_esp_box.c
│       └── ...
├── Kconfig                    # Board selection configuration
└── CMakeLists.txt            # Build configuration
```

### API Functions
- `esp_bsp_sdl_init()` - Initialize display and touch for selected board
- `esp_bsp_sdl_backlight_on/off()` - Control display backlight
- `esp_bsp_sdl_display_on_off()` - Enable/disable display
- `esp_bsp_sdl_touch_init/read()` - Touch interface (if supported)
- `esp_bsp_sdl_get_board_name()` - Get current board name
- `esp_bsp_sdl_deinit()` - Cleanup resources

### Board-Specific Implementation
Each supported board has its own implementation file that:
1. Includes the appropriate ESP-BSP headers
2. Implements the abstraction API functions
3. Handles board-specific initialization and configuration
4. Provides display parameters (resolution, pixel format, etc.)
5. Manages touch interface (if available)

## Adding New Boards

To add support for a new board:

1. **Update Kconfig** (`Kconfig`):
   ```kconfig
   config ESP_BSP_SDL_BOARD_NEW_BOARD
       bool "New Board Name"
       help
           Description of the new board.
   ```

2. **Update CMakeLists.txt**:
   ```cmake
   elseif(CONFIG_ESP_BSP_SDL_BOARD_NEW_BOARD)
       list(APPEND srcs "src/boards/esp_bsp_sdl_new_board.c")
       list(APPEND board_deps "new_board_bsp_component")
   ```

3. **Create board implementation** (`src/boards/esp_bsp_sdl_new_board.c`):
   - Include board-specific BSP headers
   - Implement all required abstraction API functions
   - Handle board-specific display and touch initialization

## Migration from Old Approach

### Old SDL Integration
```c
// Direct BSP includes
#include "bsp/esp-bsp.h"
#include "bsp/display.h"

// BSP constants used directly
mode.w = BSP_LCD_H_RES;
mode.h = BSP_LCD_V_RES;

// Direct BSP calls
ESP_ERROR_CHECK(bsp_display_new(&bsp_disp_cfg, &panel_handle, &panel_io_handle));
ESP_ERROR_CHECK(bsp_display_backlight_on());
```

### New SDL Integration
```c
// Abstraction layer include
#include "esp_bsp_sdl.h"

// Get configuration through abstraction
esp_bsp_sdl_display_config_t config;
esp_bsp_sdl_init(&config, &panel_handle, &panel_io_handle);
mode.w = config.width;
mode.h = config.height;

// Abstracted function calls
ESP_ERROR_CHECK(esp_bsp_sdl_backlight_on());
```

## Supported Boards

Currently implemented boards:
- ✅ **ESP-Box-3** (`esp-box-3_noglib`) - 320x240 ILI9341, Touch, OCTAL PSRAM
- ✅ **M5 Atom S3** (`m5_atom_s3_noglib`) - 128x128 GC9A01, No PSRAM
- ✅ **M5Stack Core S3** (`m5stack_core_s3_noglib`) - 320x240 ILI9341, Touch, QUAD PSRAM
- ✅ **ESP32-P4 Function EV** (`esp32_p4_function_ev_board_noglib`) - 1280x800 MIPI-DSI, Touch, 32MB PSRAM
- ✅ **ESP32-S3-LCD-EV-Board** (`esp32_s3_lcd_ev_board_noglib`) - 800x480 RGB, Touch, OCTAL PSRAM
- ✅ **M5Stack Tab5** (`m5stack_tab5`) - 720x1280 MIPI-DSI (portrait), GT911 Touch, 32MB PSRAM @ 200MHz
- 🚧 **ESP32-C6 DevKit** (`esp_bsp_generic`) - Generic BSP template
- 🚧 **ESP32-C3-LCDkit** (`esp32_c3_lcdkit`) - Template provided

Legend: ✅ Fully implemented, 🚧 Template provided (needs completion)

### Board-Specific Features

| Board | Display | Resolution | Touch | PSRAM | Interface |
|-------|---------|------------|-------|-------|-----------|
| ESP-Box-3 | ILI9341 | 320x240 | FT5x06 | 8MB OCTAL | SPI |
| M5 Atom S3 | GC9A01 | 128x128 | - | - | SPI |
| M5Stack Core S3 | ILI9341 | 320x240 | FT5x06 | 8MB QUAD | SPI |
| ESP32-P4 Function EV | EK9716B | 1280x800 | GT1151 | 32MB | MIPI-DSI |
| ESP32-S3-LCD-EV | RGB Panel | 800x480 | GT1151 | 32MB OCTAL | RGB |
| M5Stack Tab5 | ILI9881C | 720x1280* | GT911 | 32MB @ 200MHz | MIPI-DSI |

*M5Stack Tab5 uses 1280x720 landscape mode in SDL for better compatibility

## Dependencies

The abstraction layer uses **conditional dependencies** to load only the required BSP for your selected board:

### Automatic Dependency Management
- **Runtime Selection**: Only the selected board's BSP is loaded
- **No Conflicts**: Prevents symbol conflicts between different BSPs
- **Minimal Footprint**: Only necessary components are included

### Core Dependencies (Always Loaded)
- `esp_lcd` - LCD panel operations
- `espressif__esp_lcd_touch` - Touch interface support

### Board-Specific Dependencies (Conditional)
| Board | BSP Component | Additional Dependencies |
|-------|---------------|------------------------|
| ESP-Box-3 | `espressif__esp-box-3_noglib` | - |
| M5 Atom S3 | `espressif__m5_atom_s3_noglib` | - |
| M5Stack Core S3 | `espressif__m5stack_core_s3_noglib` | `espressif__esp_lcd_touch` |
| ESP32-P4 Function EV | `espressif__esp32_p4_function_ev_board_noglib` | `espressif__esp_lcd_touch` |
| ESP32-S3-LCD-EV | `espressif__esp32_s3_lcd_ev_board_noglib` | `espressif__esp_lcd_touch` |
| M5Stack Tab5 | `georgik__m5stack_tab5` | - |

### ESP32-P4 Specific
ESP32-P4 boards automatically include:
- `esp_driver_ppa` - Pixel Processing Accelerator
- MIPI-DSI interface support

## Critical Configuration Settings

### ESP-Box-3 Requirements

For ESP-Box-3 to work properly, these configurations are **mandatory**:

```ini
# Hardware Requirements
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y      # 8MB flash memory
CONFIG_SPIRAM_MODE_OCT=y              # Octal PSRAM mode (NOT quad!)
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y # 240MHz for optimal performance

# 🔧 CRITICAL DEBUG Setting 🔧
CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT=y   # Halt on panic instead of reboot
                                       # Essential for debugging!
```

### Why `CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT=y` is Essential

**Without this setting**: 
- System reboots continuously on panic/crash
- Makes debugging nearly impossible
- You see endless reboot loops

**With this setting**:
- System halts after printing complete panic information
- Stack traces are fully visible
- Easy to identify crash locations
- Memory issues become debuggable

This setting is **critical** for development and should always be enabled during the development phase.

## Touch Support

The abstraction layer provides optional touch support for compatible boards:

### Enabling Touch
Touch support is disabled by default to avoid initialization conflicts. Enable it via menuconfig:

```bash
idf.py menuconfig
```
Navigate to: `ESP-BSP SDL Configuration` → `Enable Touch Support`

### Touch-Compatible Boards
- **ESP-Box-3**: FT5x06 capacitive touch controller
- **M5Stack Core S3**: FT5x06 capacitive touch controller
- **ESP32-P4 Function EV**: GT1151 capacitive touch controller
- **ESP32-S3-LCD-EV-Board**: GT1151 capacitive touch controller
- **M5Stack Tab5**: GT911 multi-point capacitive touch controller

### Using Touch in Your Application
```c
#include "esp_bsp_sdl.h"

// Initialize touch (if enabled and supported)
if (esp_bsp_sdl_touch_init() == ESP_OK) {
    printf("Touch initialized successfully\n");
    
    // Read touch data
    esp_bsp_sdl_touch_info_t touch_info;
    if (esp_bsp_sdl_touch_read(&touch_info) == ESP_OK) {
        if (touch_info.pressed) {
            printf("Touch at: %d, %d\n", touch_info.x, touch_info.y);
        }
    }
}
```

## M5Stack Tab5 Special Requirements

The **M5Stack Tab5** is an advanced ESP32-P4 tablet requiring special configuration:

### Critical PSRAM Configuration
```ini
# MANDATORY for M5Stack Tab5
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_HEX=y
CONFIG_SPIRAM_SPEED_200M=y  # CRITICAL: Must be 200MHz!
CONFIG_SPIRAM_FETCH_INSTRUCTIONS=y
CONFIG_SPIRAM_RODATA=y
```

**⚠️ Warning**: M5Stack Tab5 **requires 200MHz PSRAM speed**. Lower speeds (120MHz, 80MHz) will cause instability.

### Display Orientation
- **Physical orientation**: 720x1280 (portrait)
- **SDL configuration**: 1280x720 (landscape) for better compatibility
- **Touch coordinates**: Automatically rotated to match SDL orientation

### Features
- **Display**: 5-inch 720×1280 IPS via MIPI-DSI interface
- **Touch**: GT911 multi-point capacitive touch controller
- **Memory**: 32MB PSRAM @ 200MHz, 16MB Flash
- **SoC**: ESP32-P4 dual-core RISC-V
- **Interface**: MIPI-DSI for high-speed display communication

## Configuration Files

The old approach required separate configuration files for each board:
- `sdkconfig.defaults.esp-box-3`
- `sdkconfig.defaults.esp32_c6_devkit`
- etc.

These files are still used internally by the ESP-BSP components, but developers no longer need to manually specify them via command-line arguments. However, the main `sdkconfig.defaults` should contain the critical settings mentioned above.

## Troubleshooting

### Build Errors
- Ensure you've selected a board via `idf.py menuconfig`
- Check that the ESP-BSP component for your board is available
- Verify that your target matches the selected board (e.g., ESP32-S3 for ESP-Box-3)

### Display Issues  
- Verify board selection matches your hardware
- Check display connections and power supply
- Review board-specific documentation for pin configurations

### Touch Not Working
- Ensure your board supports touch (check `has_touch` in display config)
- Verify touch interface initialization in board-specific implementation
- Check touch hardware connections
