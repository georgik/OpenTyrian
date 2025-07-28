# OpenTyrian Flashing Instructions

## Web-based Flashing (Recommended)

The easiest way to flash OpenTyrian is using our web-based installer:

1. **Connect your ESP32 board** via USB cable
2. **Open the web installer** (Chrome/Edge browsers recommended)
3. **Click "Connect"** and select your board's serial port
4. **Choose your board type** from the dropdown
5. **Click "Flash"** and wait for completion

## Supported Boards

- **M5Stack Core S3**: Portable gaming with integrated controls
- **ESP32-P4 Function EV Board**: High-performance gaming with PPA acceleration

## Manual Flashing

For advanced users, download the zip package and use the included scripts:

### Linux/macOS
```bash
./flash.sh
```

### Windows Command Prompt
```cmd
flash.bat
```

### Windows PowerShell
```powershell
flash.ps1
```

## Game Controls

- **USB Keyboard**: Connect any USB keyboard for full game control
- **T-Keyboard S3 Pro**: Enhanced gaming experience with programmable keys

## Features

- **Hardware Acceleration**: ESP32-P4 uses PPA for smooth graphics scaling
- **Full Game Data**: All assets stored in flash memory for reliability
- **Multiple Display Support**: Optimized for different screen sizes
- **Audio Support**: Full game soundtrack and sound effects

## Troubleshooting

- **Connection Issues**: Try a different USB cable or port
- **Browser Compatibility**: Use Chrome or Edge for best web flashing support
- **Board Detection**: Make sure drivers are installed for your ESP32 board
