
# OpenTyrian - ESP32 Port

![ESP32-P4 with T-Keyboard S3 Pro](docs/images/opentyrian-esp32-p4.webp)

OpenTyrian with ESP32-P4 and [T-Keyboard S3 Pro](https://github.com/MatheyDeo/LilyGoT-keyboard-s3-pro-usb-boot-keyboard).

This is a port of OpenTyrian to the ESP32 platform, originally ported to ESP32 by Gadget Workbench and updated for new hardware with support for [Board Support Packages](https://components.espressif.com/components?q=Board+Support+Package).

The fork was updated to use all the game data directly from flash, as SD cards have a tendency to be unreliable. It now works with ESP-IDF and utilizes the latest [SDL3](https://components.espressif.com/components/georgik/sdl) available at the Espressif Component Registry.

## Storyline

OpenTyrian is an open-source port of the DOS game Tyrian.

Tyrian is an arcade-style vertical scrolling shooter. The story is set in 20,031 where you play as Trent Hawkins, a skilled fighter-pilot employed to fight MicroSol and save the galaxy.

## Supported Hardware

- ESP-IDF 5.4+ or later
- **ESP32-P4 boards (16MB Flash):**
  - [ESP32-P4 Function EV Board](https://components.espressif.com/components/espressif/esp32_p4_function_ev_board_noglib) - 1024x600 RGB display, 32MB PSRAM
  - [M5Stack Tab5](https://shop.m5stack.com/products/m5stack-tab5-esp32-p4-5-inch-1280-720-mipi-dsi-ips-display-touch-screen-development-board) - 5-inch 1280x720 MIPI-DSI display with touch, 32MB PSRAM
- **ESP32-S3 boards (16MB Flash):**
  - [ESP32-S3-BOX-3](https://components.espressif.com/components/espressif/esp-box-3) - 320x240 display, 8MB PSRAM (Octal)
  - [M5Stack-CoreS3](https://components.espressif.com/components/espressif/m5stack_core_s3) - 320x240 display, 8MB PSRAM (Quad)

## ESP32-P4 Features

The ESP32-P4 version leverages the **Pixel Processing Accelerator (PPA)** for enhanced graphics performance. The PPA is a hardware accelerator specifically designed for image and graphics processing operations, enabling smooth scaling of the original game graphics to full display resolution with minimal CPU overhead.

The [PPA peripheral](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ppa.html) provides hardware acceleration for:
- Image scaling and rotation
- Color format conversion
- Blending operations
- Real-time graphics transformations

This hardware acceleration ensures OpenTyrian runs smoothly at full display resolution while maintaining responsive gameplay.

## Flash Storage and Partition Tables

OpenTyrian uses custom partition tables optimized for each board architecture to maximize available storage for game assets:

### ESP32-P4 Boards (16MB Flash)
- **Partition Table**: `partitions.csv`
- **Factory App**: 3MB (main application)
- **LittleFS Storage**: 11MB (game data and assets)
- **Configuration**: All game assets loaded directly from flash storage for reliability

### ESP32-S3 Boards (16MB Flash)
- **Partition Table**: `partitions_esp32s3_16mb.csv`
- **Factory App**: 3MB (main application)
- **LittleFS Storage**: 12MB (game data and assets)
- **PSRAM Configuration**: Optimized for frame buffers and large data structures

### Storage Architecture
The game data is stored in a LittleFS filesystem containing:
- Original Tyrian game assets (sprites, levels, sounds, music)
- Configuration files and save data
- All assets pre-loaded during build process
- No SD card dependency for maximum reliability

## Game Controls

OpenTyrian supports both USB keyboard input and touch controls (on supported hardware):

### Keyboard Controls
- **Arrow Keys**: Move your ship
- **Space**: Fire primary weapon
- **Left Ctrl**: Fire secondary weapon / sidekicks
- **Left Alt**: Fire super bombs
- **Enter**: Change weapon mode
- **F1**: In-game help
- **Escape**: Pause game / access menu

### Touch Controls (M5Stack Tab5)
The M5Stack Tab5 features a 5-inch capacitive touch screen that provides intuitive control:
- **Touch and drag**: Move your ship
- **Tap**: Fire primary weapon
- **Two-finger tap**: Fire secondary weapon
- **Long press**: Fire super bombs

### USB Keyboard Support (ESP32-P4)
Connect any standard USB keyboard to the ESP32-P4 boards for full keyboard control. The game automatically detects and configures USB HID keyboards.

## Web-based Flashing (Easiest)

For the quickest installation, use our web-based installer (Chrome/Edge browsers recommended):

[![Try it with ESP Launchpad](https://espressif.github.io/esp-launchpad/assets/try_with_launchpad.png)](https://georgik.github.io/OpenTyrian/?flashConfigURL=https://georgik.github.io/OpenTyrian/config/config.toml)

1. Connect your ESP32-P4 board via USB
2. Click the button above to open the web installer
3. Click "Connect" and select your board's serial port
4. Choose "OpenTyrian" and your board type
5. Click "Flash" and wait for completion

## Building from Source

### Prerequisites
- ESP-IDF 5.4+ installed and configured
- Git for cloning the repository
- USB cable for flashing

### Build Method Comparison

| Method | Pros | Best For |
|--------|------|----------|
| **espbrew TUI** | Interactive interface, real-time monitoring, builds all boards simultaneously | Development, first-time users |
| **espbrew CLI** | Perfect for CI/CD, parallel builds, generates scripts | Automated builds, scripting |
| **Manual ESP-IDF** | Full control, standard ESP-IDF workflow | Advanced users, debugging |

### Quick Build with espbrew (Recommended)

[espbrew](https://github.com/georgik/espbrew) is a TUI/CLI build manager that automatically discovers board configurations and provides real-time build monitoring:

#### Installation

```shell
# Option 1: One-line install (recommended)
curl -L https://georgik.github.io/espbrew/install.sh | bash

# Option 2: Homebrew (macOS Apple Silicon)
brew tap georgik/espbrew
brew install espbrew

# Option 3: Build from source
cargo install espbrew
```

#### Usage

```shell
# Clone OpenTyrian
git clone https://github.com/georgik/OpenTyrian.git
cd OpenTyrian

# Interactive TUI mode (recommended) - shows real-time build progress
espbrew

# CLI mode - build all boards simultaneously
espbrew --cli-only build

# Use generated build scripts (created by espbrew)
./support/build_esp32_p4_function_ev_board.sh
./support/build_m5stack_tab5.sh
./support/build_esp-box-3.sh
./support/build_m5stack_core_s3.sh

# Flash using generated scripts
./support/flash_esp32_p4_function_ev_board.sh
```

### Manual Build with ESP-IDF

Alternatively, build manually using ESP-IDF with the SDKCONFIG_DEFAULTS environment variable:

```shell
git clone https://github.com/georgik/OpenTyrian.git
cd OpenTyrian

# Set up ESP-IDF environment
source $IDF_PATH/export.sh  # or export.bat on Windows

# Build for specific boards using SDKCONFIG_DEFAULTS environment variable

# ESP32-P4 Function EV Board (1024x600 RGB display)
SDKCONFIG_DEFAULTS="sdkconfig.defaults.esp32_p4_function_ev_board" idf.py -B "build.esp32_p4_function_ev_board" build
SDKCONFIG_DEFAULTS="sdkconfig.defaults.esp32_p4_function_ev_board" idf.py -B "build.esp32_p4_function_ev_board" flash monitor

# M5Stack Tab5 (1280x720 MIPI-DSI display with touch)
SDKCONFIG_DEFAULTS="sdkconfig.defaults.m5stack_tab5" idf.py -B "build.m5stack_tab5" build
SDKCONFIG_DEFAULTS="sdkconfig.defaults.m5stack_tab5" idf.py -B "build.m5stack_tab5" flash monitor

# ESP32-S3-BOX-3 (320x240 display)
SDKCONFIG_DEFAULTS="sdkconfig.defaults.esp-box-3" idf.py -B "build.esp-box-3" build
SDKCONFIG_DEFAULTS="sdkconfig.defaults.esp-box-3" idf.py -B "build.esp-box-3" flash monitor

# M5Stack CoreS3 (320x240 display)
SDKCONFIG_DEFAULTS="sdkconfig.defaults.m5stack_core_s3" idf.py -B "build.m5stack_core_s3" build
SDKCONFIG_DEFAULTS="sdkconfig.defaults.m5stack_core_s3" idf.py -B "build.m5stack_core_s3" flash monitor
```

### Board-Specific Features

#### M5Stack Tab5 (ESP32-P4)
- **Architecture**: ESP32-P4 RISC-V dual-core
- **Display**: 5-inch 1280x720 MIPI-DSI IPS with touch
- **Storage**: 16MB Flash + 32MB PSRAM (Hex mode)
- **Native Landscape Orientation**: Display configured for 1280x720 landscape mode
- **Touch Input**: Full capacitive touch support with coordinate transformation
- **Hardware Scaling**: PPA scaling disabled in favor of efficient display driver scaling
- **Partition Table**: `partitions.csv` (3MB app + 11MB storage)

#### ESP32-P4 Function EV Board
- **Architecture**: ESP32-P4 RISC-V dual-core
- **Display**: 1024x600 RGB LCD
- **Storage**: 16MB Flash + 32MB PSRAM (Hex mode)
- **PPA Acceleration**: Hardware-accelerated 3x scaling (320x200 → 960x600)
- **RGB Display**: Direct RGB interface for minimal latency
- **USB HID**: Full keyboard and mouse support
- **Partition Table**: `partitions.csv` (3MB app + 11MB storage)

#### ESP32-S3-BOX-3
- **Architecture**: ESP32-S3 Xtensa dual-core
- **Display**: 320x240 ILI9341 LCD
- **Storage**: 16MB Flash + 8MB PSRAM (Octal mode)
- **Touch Support**: Capacitive touch interface
- **Audio**: Built-in speaker and microphone
- **Partition Table**: `partitions_esp32s3_16mb.csv` (3MB app + 12MB storage)

#### M5Stack CoreS3
- **Architecture**: ESP32-S3 Xtensa dual-core
- **Display**: 320x240 ILI9341 LCD
- **Storage**: 16MB Flash + 8MB PSRAM (Quad mode)
- **IMU**: Built-in accelerometer and gyroscope
- **Audio**: Built-in speaker
- **Partition Table**: `partitions_esp32s3_16mb.csv` (3MB app + 12MB storage)

### Development Tips

#### Using espbrew
```shell
# Interactive TUI with real-time build monitoring
espbrew

# Build all boards in parallel
espbrew --cli-only build

# Use generated build scripts
./support/build_esp32_p4_function_ev_board.sh
```

#### Manual ESP-IDF Commands
```shell
# Fast application-only flashing (after initial flash)
SDKCONFIG_DEFAULTS="sdkconfig.defaults.esp32_p4_function_ev_board" idf.py -B "build.esp32_p4_function_ev_board" app-flash monitor

# Clean specific build
rm -rf build.esp32_p4_function_ev_board

# Clean all builds
rm -rf build.*

# Configure project interactively (creates temporary sdkconfig)
SDKCONFIG_DEFAULTS="sdkconfig.defaults.esp32_p4_function_ev_board" idf.py menuconfig
```

#### Build Directory Structure
Each board gets its own build directory to avoid conflicts:
- `build.esp32_p4_function_ev_board/` - ESP32-P4 Function EV Board
- `build.m5stack_tab5/` - M5Stack Tab5
- `build.esp-box-3/` - ESP32-S3-BOX-3
- `build.m5stack_core_s3/` - M5Stack CoreS3

## Troubleshooting

### Common Issues

**Game not responding to keyboard input:**
- Ensure USB keyboard is properly connected to ESP32-P4
- Check that the keyboard is detected in the boot logs
- Try different USB ports or cables

**Display orientation issues:**
- For M5Stack Tab5: Ensure `CONFIG_SDL_BSP_M5STACK_TAB5=y` is set in sdkconfig
- Check that the correct sdkconfig defaults file is being used

**Touch not working on M5Stack Tab5:**
- Verify that touch is enabled in the BSP configuration
- Check for proper touch controller initialization in logs

**Build errors:**
- Ensure ESP-IDF 5.4+ is installed and properly configured
- Using espbrew: Check build logs in `./logs/` directory
- Using manual builds: Clean specific build directory `rm -rf build.{board_name}`
- Check component dependencies are properly resolved
- Verify SDKCONFIG_DEFAULTS path is correct when using manual builds

**Partition table / LittleFS errors:**
- Verify correct partition table is being used for your board architecture
- ESP32-P4 boards should use `partitions.csv`
- ESP32-S3 boards should use `partitions_esp32s3_16mb.csv`
- If changing board types, always run `idf.py fullclean` before rebuilding
- Check that `CONFIG_PARTITION_TABLE_CUSTOM=y` is set in sdkconfig

**Memory/linking errors:**
- Ensure PSRAM is properly configured for your board
- ESP32-S3-BOX-3 uses Octal PSRAM mode
- M5Stack CoreS3 uses Quad PSRAM mode
- ESP32-P4 boards use Hex PSRAM mode
- Large frame buffers should be allocated in PSRAM, not internal RAM

### Performance Optimization

- **M5Stack Tab5**: Hardware scaling provides optimal performance
- **ESP32-P4**: PPA acceleration reduces CPU load for graphics operations
- **PSRAM**: Ensure PSRAM is properly configured for frame buffers

## Keyboard Extension (Optional)

If you'd like to extend the project by using [T-Keyboard S3 Pro](https://lilygo.cc/products/t-keyboard-s3-pro), then you can use project [LilyGoT-keyboard-s3-pro-usb-boot-keyboard](https://github.com/MatheyDeo/LilyGoT-keyboard-s3-pro-usb-boot-keyboard).

## Acknowledgements

This port is based on the work of the original OpenTyrian project (https://github.com/jkirsons/OpenTyrian) and an ESP32 port by Gadget Workbench, which was initially created for ESP-WROVER and ESP-IDF 4.2. 

The current version has been significantly updated to:
- Support ESP-IDF 5.4+ with modern Board Support Packages (ESP-BSP)
- Utilize SDL3 from the [Espressif Component Registry](https://components.espressif.com/components/georgik/sdl/)
- Add M5Stack Tab5 support with optimized landscape orientation and touch input
- Implement hardware-accelerated scaling using ESP32-P4 PPA
- Include comprehensive USB HID keyboard support
- Provide game data directly from flash storage for reliability

Special thanks to:
- The OpenTyrian development team for the original open-source port
- Gadget Workbench for the initial ESP32 adaptation
- Espressif Systems for ESP-IDF, hardware platforms, and component ecosystem
- M5Stack for the Tab5 development board and BSP support

## Original Video

[![Alt text](https://img.youtube.com/vi/UL5eTUv7SZE/0.jpg)](https://www.youtube.com/watch?v=UL5eTUv7SZE)

## Credits

Special thanks to all contributors and the open-source community for making this project possible.

