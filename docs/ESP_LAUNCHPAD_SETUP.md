# ESP Launchpad Setup for OpenTyrian

This document describes how to set up web-based flashing for OpenTyrian using ESP Launchpad.

## Overview

ESP Launchpad allows users to flash OpenTyrian firmware directly from their web browser without installing any additional software. This makes the installation process much more accessible.

## Setup Steps

### Integrated Approach (Current)

We deploy ESP Launchpad directly from this repository using GitHub Pages. The configuration is in `launchpad/config/config.toml` and automatically deploys when changes are pushed.

### Alternative: Fork ESP Launchpad

If you want to create your own ESP Launchpad instance:

1. Fork the [ESP Launchpad repository](https://github.com/espressif/esp-launchpad)
2. Clone your fork: `git clone https://github.com/YOUR_USERNAME/esp-launchpad`
3. Replace the content of `config/config.toml` with our configuration:

```toml
esp_toml_version = 1.0

# Base URL where OpenTyrian firmware binaries are hosted
firmware_images_url = "https://github.com/georgik/OpenTyrian/releases/latest/download/"

# OpenTyrian applications available
supported_apps = ["OpenTyrian"]

# Configuration documentation
config_readme_url = "https://raw.githubusercontent.com/georgik/OpenTyrian/main/docs/FLASH_INSTRUCTIONS.md"

[OpenTyrian]
# Supported ESP32 chipsets
chipsets = ["ESP32-P4"]

# Development kits
developKits.esp32-p4 = ["esp32-p4-function-ev-board"]

# Firmware images for each board
image.esp32-p4-function-ev-board = "opentyrian-latest-esp32_p4_function_ev_board.bin"

# Description
description = "OpenTyrian - Classic space shooter game ported to ESP32. Experience nostalgic arcade gaming with modern ESP32 hardware acceleration."

# Additional information
readme.text = "https://raw.githubusercontent.com/georgik/OpenTyrian/main/README.md"

# Console settings
console_baudrate = 115200
```

### 3. Enable GitHub Pages

1. Go to your forked repository settings
2. Navigate to "Pages" section
3. Set source to "GitHub Actions"
4. The existing `.github/workflows/pages.yml` will automatically deploy

### 4. Update README

Add the ESP Launchpad badge to your project README:

```markdown
## Web-based Flashing

[![Try it with ESP Launchpad](https://espressif.github.io/esp-launchpad/assets/try_with_launchpad.png)](https://YOUR_USERNAME.github.io/esp-launchpad/?flashConfigURL=https://YOUR_USERNAME.github.io/esp-launchpad/config/config.toml)
```

## How It Works

1. **Build Process**: Our CI generates both individual binaries (for advanced users) and merged binaries (for web flashing)
2. **Release Process**: Both zip packages and merged binaries are uploaded to GitHub releases
3. **ESP Launchpad**: Reads the TOML configuration and downloads firmware from GitHub releases
4. **Web Flashing**: Users can flash directly from browser using WebUSB

## Benefits

- **No software installation** required
- **Cross-platform** - works on Windows, macOS, Linux
- **User-friendly** - simple point-and-click interface
- **Always up-to-date** - automatically uses latest releases
- **Multiple board support** - automatically detects board type

## Usage

1. Connect your ESP32 board via USB
2. Open the ESP Launchpad URL
3. Click "Connect" and select your serial port
4. Choose "OpenTyrian" from the firmware list
5. Select your board type
6. Click "Flash"

## Troubleshooting

- **CORS Issues**: Make sure firmware URLs are publicly accessible
- **Board Detection**: ESP Launchpad automatically detects supported boards
- **Serial Port**: Ensure your browser supports WebUSB (Chrome/Edge recommended)
