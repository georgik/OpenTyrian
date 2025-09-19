
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
- **ESP32-P4 boards:**
  - [ESP32-P4 Function EV Board](https://components.espressif.com/components/espressif/esp32_p4_function_ev_board_noglib) - 1024x600 RGB display
  - [M5Stack Tab5](https://shop.m5stack.com/products/m5stack-tab5-esp32-p4-5-inch-1280-720-mipi-dsi-ips-display-touch-screen-development-board) - 5-inch 1280x720 MIPI-DSI display with touch
- **ESP32-S3 boards:**
  - [ESP32-S3-BOX-3](https://components.espressif.com/components/espressif/esp-box-3) - 320x240 display
  - [M5Stack-CoreS3](https://components.espressif.com/components/espressif/m5stack_core_s3) - 320x240 display

## ESP32-P4 Features

The ESP32-P4 version leverages the **Pixel Processing Accelerator (PPA)** for enhanced graphics performance. The PPA is a hardware accelerator specifically designed for image and graphics processing operations, enabling smooth scaling of the original game graphics to full display resolution with minimal CPU overhead.

The [PPA peripheral](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ppa.html) provides hardware acceleration for:
- Image scaling and rotation
- Color format conversion
- Blending operations
- Real-time graphics transformations

This hardware acceleration ensures OpenTyrian runs smoothly at full display resolution while maintaining responsive gameplay.

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

### Quick Build with espbrew (Recommended)

[espbrew](https://github.com/georgik/espbrew) is a convenient build tool that simplifies ESP-IDF project management:

```shell
# Install espbrew (if not already installed)
curl -L https://georgik.github.io/espbrew/install.sh | bash

# Clone and build OpenTyrian
git clone https://github.com/georgik/OpenTyrian.git
cd OpenTyrian

# Build and flash for different boards
espbrew build --board esp32_p4_function_ev_board  # ESP32-P4 Function EV Board
espbrew build --board m5stack_tab5                # M5Stack Tab5
espbrew build --board esp-box-3                   # ESP32-S3-BOX-3
espbrew build --board m5stack_core_s3             # M5Stack CoreS3

# Flash the firmware
espbrew flash
```

### Manual Build with ESP-IDF

Alternatively, use board-specific sdkconfig files for building:

```shell
git clone https://github.com/georgik/OpenTyrian.git
cd OpenTyrian

# Set up ESP-IDF environment
source $IDF_PATH/export.sh  # or export.bat on Windows

# Build for specific boards using sdkconfig defaults

# ESP32-P4 Function EV Board (1024x600 RGB display)
cp sdkconfig.defaults.esp32_p4_function_ev_board sdkconfig.defaults
idf.py build flash monitor

# M5Stack Tab5 (1280x720 MIPI-DSI display with touch)
cp sdkconfig.defaults.m5stack_tab5 sdkconfig.defaults
idf.py build flash monitor

# ESP32-S3-BOX-3
cp sdkconfig.defaults.esp-box-3 sdkconfig.defaults
idf.py build flash monitor

# M5Stack CoreS3
cp sdkconfig.defaults.m5stack_core_s3 sdkconfig.defaults
idf.py build flash monitor
```

### Board-Specific Features

#### M5Stack Tab5 Optimizations
- **Native Landscape Orientation**: Display configured for 1280x720 landscape mode
- **Touch Input**: Full capacitive touch support with coordinate transformation
- **Hardware Scaling**: PPA scaling disabled in favor of efficient display driver scaling
- **PSRAM Integration**: Utilizes 32MB PSRAM for optimal performance

#### ESP32-P4 Function EV Board
- **PPA Acceleration**: Hardware-accelerated 3x scaling (320x200 → 960x600)
- **RGB Display**: Direct RGB interface for minimal latency
- **USB HID**: Full keyboard and mouse support

### Development Tips

```shell
# Fast application-only flashing (after initial flash)
idf.py app-flash monitor

# Clean build
idf.py fullclean

# Configure project interactively
idf.py menuconfig
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
- Ensure ESP-IDF 5.4+ is installed and properly configured
- Run `idf.py fullclean` and rebuild
- Check component dependencies are properly resolved

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

