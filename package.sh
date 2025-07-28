#!/bin/bash

VERSION=$1
BOARD=$2

# Create package directory
PACKAGE_DIR="opentyrian-${VERSION}-${BOARD}"
mkdir -p $PACKAGE_DIR

# Copy binaries to package directory
cp ./bootloader-${BOARD}.bin $PACKAGE_DIR/
cp ./partition-table-${BOARD}.bin $PACKAGE_DIR/
cp ./opentyrian.bin $PACKAGE_DIR/
cp ./storage.bin $PACKAGE_DIR/

# Create flash scripts with board-specific flags
ESPFLASH_FLAGS=""
BOOTLOADER_OFFSET="0x0"
if [ "$BOARD" = "esp32_p4_function_ev_board" ]; then
    ESPFLASH_FLAGS="--chip esp32p4 --no-stub"
    BOOTLOADER_OFFSET="0x2000"
fi

cat << EOF > $PACKAGE_DIR/flash.sh
#!/bin/bash
echo "Flashing OpenTyrian for ${BOARD}..."
echo "Flashing bootloader..."
espflash write-bin ${ESPFLASH_FLAGS} ${BOOTLOADER_OFFSET} bootloader-${BOARD}.bin
echo "Flashing partition table..."
espflash write-bin ${ESPFLASH_FLAGS} 0x8000 partition-table-${BOARD}.bin
echo "Flashing application..."
espflash write-bin ${ESPFLASH_FLAGS} 0x10000 opentyrian.bin
echo "Flashing storage data..."
espflash write-bin ${ESPFLASH_FLAGS} 0x310000 storage.bin
echo "Done!"
EOF
chmod +x $PACKAGE_DIR/flash.sh

cat << EOF > $PACKAGE_DIR/flash.bat
@echo off
echo Flashing OpenTyrian for ${BOARD}...
echo Flashing bootloader...
espflash write-bin ${ESPFLASH_FLAGS} ${BOOTLOADER_OFFSET} bootloader-${BOARD}.bin
echo Flashing partition table...
espflash write-bin ${ESPFLASH_FLAGS} 0x8000 partition-table-${BOARD}.bin
echo Flashing application...
espflash write-bin ${ESPFLASH_FLAGS} 0x10000 opentyrian.bin
echo Flashing storage data...
espflash write-bin ${ESPFLASH_FLAGS} 0x310000 storage.bin
echo Done!
EOF

cat << EOF > $PACKAGE_DIR/flash.ps1
Write-Host "Flashing OpenTyrian for ${BOARD}..."
Write-Host "Flashing bootloader..."
espflash write-bin ${ESPFLASH_FLAGS} ${BOOTLOADER_OFFSET} bootloader-${BOARD}.bin
Write-Host "Flashing partition table..."
espflash write-bin ${ESPFLASH_FLAGS} 0x8000 partition-table-${BOARD}.bin
Write-Host "Flashing application..."
espflash write-bin ${ESPFLASH_FLAGS} 0x10000 opentyrian.bin
Write-Host "Flashing storage data..."
espflash write-bin ${ESPFLASH_FLAGS} 0x310000 storage.bin
Write-Host "Done!"
EOF

# Create comprehensive TOML manifest
cat << EOF > $PACKAGE_DIR/manifest.toml
# OpenTyrian Firmware Package Manifest
# This file describes the firmware components and flashing instructions

[package]
name = "OpenTyrian"
version = "$VERSION"
board = "$BOARD"
target = "$(case $BOARD in
  esp-box-3|m5stack_core_s3) echo 'esp32s3';;
  esp32_p4_function_ev_board) echo 'esp32p4';;
  *) echo 'unknown';;
esac)"
created_at = "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
description = "OpenTyrian game firmware for ESP32 boards"

# Flash memory layout and components
[[flash_components]]
name = "bootloader"
file = "bootloader-${BOARD}.bin"
offset = "${BOOTLOADER_OFFSET}"
required = true
description = "ESP-IDF bootloader - handles initial boot sequence"

[[flash_components]]
name = "partition_table"
file = "partition-table-${BOARD}.bin"
offset = "0x8000"
required = true
description = "Partition table - defines flash memory layout"

[[flash_components]]
name = "application"
file = "opentyrian.bin"
offset = "0x10000"
required = true
description = "Main OpenTyrian application binary"

[[flash_components]]
name = "storage"
file = "storage.bin"
offset = "0x310000"
required = true
description = "Game assets and data storage (LittleFS)"

# Flash settings
[flash_settings]
mode = "dio"          # Flash mode (dio/qio/dout/qout)
frequency = "80m"     # Flash frequency
size = "detect"       # Flash size (detect automatically)

# Tool configurations
[tools.espflash]
flags = "${ESPFLASH_FLAGS}"

[tools.espflash.commands]
bootloader = "espflash write-bin ${ESPFLASH_FLAGS} ${BOOTLOADER_OFFSET} bootloader-${BOARD}.bin"
partition_table = "espflash write-bin ${ESPFLASH_FLAGS} 0x8000 partition-table-${BOARD}.bin"
application = "espflash write-bin ${ESPFLASH_FLAGS} 0x10000 opentyrian.bin"
storage = "espflash write-bin ${ESPFLASH_FLAGS} 0x310000 storage.bin"

[tools.espflash.components]
bootloader = "bootloader-${BOARD}.bin"
partition_table = "partition-table-${BOARD}.bin"
application = "opentyrian.bin"
storage = "storage.bin"

[tools.esptool]
erase_command = "esptool.py --chip auto erase_flash"
flash_command = "esptool.py --chip auto write_flash --flash_mode {mode} --flash_size {size} --flash_freq {frequency} {components}"

# Available flashing scripts
[scripts]
linux = "flash.sh"
macos = "flash.sh"
windows_cmd = "flash.bat"
windows_powershell = "flash.ps1"
EOF

# Zip the package
zip -r ${PACKAGE_DIR}.zip $PACKAGE_DIR

# Clean up
rm -r $PACKAGE_DIR

