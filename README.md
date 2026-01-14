
# OpenTyrian - ESP32 Port

![ESP32-P4 with T-Keyboard S3 Pro](docs/images/opentyrian-esp32-p4.webp)

OpenTyrian with ESP32-P4 and [T-Keyboard S3 Pro](https://github.com/MatheyDeo/LilyGoT-keyboard-s3-pro-usb-boot-keyboard).

This is a port of OpenTyrian to the ESP32 platform, originally ported to ESP32 by Gadget Workbench and updated for new hardware with support for [Board Support Packages](https://components.espressif.com/components?q=Board+Support+Package).

The port works with ESP-IDF and utilizes the latest [SDL3](https://components.espressif.com/components/georgik/sdl) available at the Espressif Component Registry. Game data can be stored either in flash memory or on SD card (ESP32-P4 only).

## Storyline

OpenTyrian is an open-source port of the DOS game Tyrian.

Tyrian is an arcade-style vertical scrolling shooter. The story is set in 20,031 where you play as Trent Hawkins, a skilled fighter-pilot employed to fight MicroSol and save the galaxy.

## Supported Hardware

- ESP-IDF 6.1 or later
- **ESP32-P4 boards (16MB Flash):**
  - [ESP32-P4 Function EV Board](https://components.espressif.com/components/espressif/esp32_p4_function_ev_board_noglib) - 1024x600 RGB display, 32MB PSRAM
  - [M5Stack Tab5](https://shop.m5stack.com/products/m5stack-tab5-esp32-p4-5-inch-1280-720-mipi-dsi-ips-display-touch-screen-development-board) - 5-inch 1280x720 MIPI-DSI display with touch, 32MB PSRAM
- **ESP32-S3 boards (16MB Flash):**
  - [ESP32-S3-BOX-3](https://components.espressif.com/components/espressif/esp-box-3) - 320x240 display, 8MB PSRAM (Octal)
  - [M5Stack-CoreS3](https://components.espressif.com/components/espressif/m5stack_core_s3) - 320x240 display, 8MB PSRAM (Quad)

## ESP32-P4 Features

The ESP32-P4 version includes hardware-accelerated graphics and audio capabilities for optimal performance.

### Pixel Processing Accelerator (PPA)

The ESP32-P4 version leverages the **Pixel Processing Accelerator (PPA)** for enhanced graphics performance. The PPA is a hardware accelerator specifically designed for image and graphics processing operations, enabling smooth scaling of the original game graphics to full display resolution with minimal CPU overhead.

The [PPA peripheral](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ppa.html) provides hardware acceleration for:
- Image scaling and rotation
- Color format conversion
- Blending operations
- Real-time graphics transformations

This hardware acceleration ensures OpenTyrian runs smoothly at full display resolution while maintaining responsive gameplay.

### Audio System

OpenTyrian on ESP32-P4 includes a complete audio implementation supporting all original game sound effects and music.

**Audio Hardware:**
- ES8311 audio codec with I2S interface
- 22.05kHz sample rate (downsampled from 44.1kHz for memory efficiency)
- Mono output optimized for embedded systems
- TX-only I2S configuration to minimize DMA memory usage

**Audio Features:**
- All original sound effects (explosions, shooting, pickups, UI sounds)
- OPL/AdLib emulator for music playback
- Real-time audio mixing with proper volume control
- Memory-efficient design using PSRAM for buffers

**Technical Implementation:**
The audio system uses a custom SDL backend that integrates with the ESP-IDF audio codec framework. Sound samples are loaded from the game data files and mixed in real-time by the audio callback. The system handles:
- Automatic volume initialization (75% music, 100% SFX)
- Codec mute/unmute during initialization to prevent startup artifacts
- NULL pointer safety for completed sounds
- Efficient memory usage with TX-only I2S mode and mono output

## Game Data Storage

OpenTyrian supports two methods for storing game data:

### Storage Options Overview

| Method | Boards Supported | Capacity | Setup |
|--------|-----------------|----------|-------|
| **Internal Flash** | All boards | 11-12 MB | Automatic (embedded in firmware) |
| **SD Card** | ESP32-P4 only | Up to SD card size | Manual (copy data to SD card) |

### ESP32-P4 Automatic Fallback

On ESP32-P4 boards, the game automatically detects and uses available game data in this priority order:

1. **SD Card** (if present with Tyrian data at `/sdcard/tyrian/data/`)
2. **Internal Flash** (fallback if SD card unavailable or missing data)

This fallback system is completely automatic—no configuration needed. Simply insert an SD card with game data and the game will use it; otherwise, it uses the internal flash storage.

### Internal Flash Storage

All boards support game data stored directly in flash memory using LittleFS filesystem.

#### ESP32-P4 Boards (16MB Flash)
- **Partition Table**: `partitions.csv`
- **Factory App**: 3MB (main application)
- **LittleFS Storage**: 11MB (game data and assets)
- **Mount Point**: `/flash/`

#### ESP32-S3 Boards (16MB Flash)
- **Partition Table**: `partitions_esp32s3_16mb.csv`
- **Factory App**: 3MB (main application)
- **LittleFS Storage**: 12MB (game data and assets)
- **Mount Point**: `/flash/`

Flash storage is embedded during the build process—no additional setup required.

### SD Card Storage (ESP32-P4 Only)

ESP32-P4 Function EV Board supports loading game data from SD card, which offers several advantages:

- **Larger Capacity**: Store additional game mods or custom assets
- **Easy Updates**: Change game data without reflashing firmware
- **Development**: Test new assets quickly during development

#### Supported Hardware
- ESP32-P4 Function EV Board (SDMMC interface)
- SD card interface pins: GPIO 39-44 (D0-D3, CMD, CLK)

#### SD Card Setup

1. **Format SD Card** (if needed):
   - Filesystem: FAT32
   - Cluster size: Default or 4KB

2. **Create Directory Structure**:
   ```
   /sdcard/
   └── tyrian/
       └── data/
           ├── tyrian1.lvl
           ├── tyrian.hdt
           ├── voices.snd
           └── [all other game files]
   ```

3. **Copy Game Data**:
   - Download Tyrian data files from the official OpenTyrian project
   - Extract and copy all files to `/sdcard/tyrian/data/` on the SD card
   - Ensure `tyrian1.lvl` exists in the data directory (used for detection)

4. **Insert SD Card**:
   - Power off the board
   - Insert SD card into the slot
   - Power on—the game will automatically detect and use SD card data

#### SD Card Behavior

- **Hot Plugging**: Not supported. Insert SD card before powering on.
- **Card Removal**: Game may crash if SD card is removed during play.
- **Empty/Missing Data**: Falls back to internal flash automatically.
- **Corrupted Card**: Falls back to internal flash with error message.

#### Verification

When the game boots, watch the serial output for confirmation:

```
OpenTyrian File System Initialization
==============================================

[1/2] Attempting to mount SD card...
SUCCESS: SD card mounted at /sdcard
Found Tyrian data on SD card at /sdcard/tyrian/data

>>> Using SD card for game data <<<
```

If SD card is not used:
```
[2/2] Attempting to mount internal flash LittleFS...
SUCCESS: Internal flash LittleFS mounted at /flash

>>> Using internal flash for game data <<<
```

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
- ESP-IDF 6.1 installed and configured
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
- **Audio**: ES8311 codec with I2S, 22.05kHz mono output, all SFX and music working
- **RGB Display**: Direct RGB interface for minimal latency
- **USB HID**: Full keyboard and mouse support
- **SD Card**: SDMMC interface with automatic fallback to flash storage
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

#### Fast Development Workflow
```shell
# Fast application-only flashing (after initial full flash)
# Using espbrew generated scripts:
./support/build_esp32_p4_function_ev_board.sh --app-flash

# Using manual ESP-IDF:
SDKCONFIG_DEFAULTS="sdkconfig.defaults.esp32_p4_function_ev_board" idf.py -B "build.esp32_p4_function_ev_board" app-flash monitor
```

#### Project Configuration
```shell
# Interactive configuration (creates temporary sdkconfig)
SDKCONFIG_DEFAULTS="sdkconfig.defaults.esp32_p4_function_ev_board" idf.py menuconfig

# View partition table layout
idf.py partition-table

# Monitor serial output without flashing
idf.py monitor -p /dev/cu.usbserial-*
```

#### Debugging and Analysis
```shell
# Check component sizes
idf.py size-components

# Analyze memory usage
idf.py size

# View build logs (when using espbrew)
ls ./logs/
tail -f ./logs/esp32_p4_function_ev_board.log
```

#### Clean Builds
```shell
# Clean specific board build
rm -rf build.{board_name}

# Clean all board builds
rm -rf build.*

# Clean generated espbrew scripts
rm -rf support/ logs/
```

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
- Ensure ESP-IDF 6.1 is installed and properly configured
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

**SD card issues (ESP32-P4):**
- Game not detecting SD card: Verify `tyrian1.lvl` exists at `/sdcard/tyrian/data/`
- SD card mounting fails: Check SD card is formatted as FAT32
- Automatic fallback not working: Check boot logs for filesystem initialization messages
- Using wrong mount point: ESP32-P4 uses `/sdcard`, not `/sd` (changed from earlier versions)

**Audio issues (ESP32-P4):**
- No sound effects: Verify audio codec is initialized in boot logs (look for "ES8311" and "I2S_IF" messages)
- Sound but no music: Music files may not be loaded; check for "audio loaded" message after sound files load
- Distorted audio: Check that I2S_MCLK_MULTIPLE_384 is configured for ES8311 codec at 22.05kHz
- Audio crashes: Ensure audio task stack size is sufficient (12KB recommended for OPL emulator)
- Startup sounds: Audio codec is muted during initialization to prevent artifacts; this is normal behavior

### Performance Optimization

- **M5Stack Tab5**: Hardware scaling provides optimal performance
- **ESP32-P4**: PPA acceleration reduces CPU load for graphics operations
- **PSRAM**: Ensure PSRAM is properly configured for frame buffers

## Keyboard Extension (Optional)

If you'd like to extend the project by using [T-Keyboard S3 Pro](https://lilygo.cc/products/t-keyboard-s3-pro), then you can use project [LilyGoT-keyboard-s3-pro-usb-boot-keyboard](https://github.com/MatheyDeo/LilyGoT-keyboard-s3-pro-usb-boot-keyboard).

## Acknowledgements

This port is based on the work of the original OpenTyrian project (https://github.com/jkirsons/OpenTyrian) and an ESP32 port by Gadget Workbench, which was initially created for ESP-WROVER and ESP-IDF 4.2.

The current version has been significantly updated to:
- Support ESP-IDF 6.1 with modern Board Support Packages (ESP-BSP)
- Implement complete audio system with ES8311 codec (all SFX and music working)
- Utilize SDL3 from the [Espressif Component Registry](https://components.espressif.com/components/georgik/sdl/)
- Add M5Stack Tab5 support with optimized landscape orientation and touch input
- Implement hardware-accelerated scaling using ESP32-P4 PPA
- Include comprehensive USB HID keyboard support
- Provide flexible game data storage (flash and SD card with automatic fallback)
- Optimize memory usage for DMA-constrained ESP32-P4 architecture

Special thanks to:
- The OpenTyrian development team for the original open-source port
- Gadget Workbench for the initial ESP32 adaptation
- Espressif Systems for ESP-IDF, hardware platforms, and component ecosystem
- M5Stack for the Tab5 development board and BSP support

## Original Video

[![Alt text](https://img.youtube.com/vi/UL5eTUv7SZE/0.jpg)](https://www.youtube.com/watch?v=UL5eTUv7SZE)

## Credits

Special thanks to all contributors and the open-source community for making this project possible.

