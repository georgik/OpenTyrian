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

# Create flash scripts with board-specific flags
ESPFLASH_FLAGS=""
if [ "$BOARD" = "esp32_p4_function_ev_board" ]; then
    ESPFLASH_FLAGS="--chip esp32p4 --no-stub"
fi

cat << EOF > $PACKAGE_DIR/flash.sh
#!/bin/bash
echo "Flashing OpenTyrian for ${BOARD}..."
espflash write-bin ${ESPFLASH_FLAGS} 0x0 bootloader-${BOARD}.bin 0x8000 partition-table-${BOARD}.bin 0x10000 opentyrian.bin
EOF
chmod +x $PACKAGE_DIR/flash.sh

cat << EOF > $PACKAGE_DIR/flash.bat
@echo off
echo Flashing OpenTyrian for ${BOARD}...
espflash write-bin ${ESPFLASH_FLAGS} 0x0 bootloader-${BOARD}.bin 0x8000 partition-table-${BOARD}.bin 0x10000 opentyrian.bin
EOF

cat << EOF > $PACKAGE_DIR/flash.ps1
Write-Host "Flashing OpenTyrian for ${BOARD}..."
espflash write-bin ${ESPFLASH_FLAGS} 0x0 bootloader-${BOARD}.bin 0x8000 partition-table-${BOARD}.bin 0x10000 opentyrian.bin
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
offset = "0x0"
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

# Flash settings
[flash_settings]
mode = "dio"          # Flash mode (dio/qio/dout/qout)
frequency = "80m"     # Flash frequency
size = "detect"       # Flash size (detect automatically)

# Tool configurations
[tools.espflash]
command = "espflash write-bin ${ESPFLASH_FLAGS} 0x0 {bootloader} 0x8000 {partition_table} 0x10000 {application}"
flags = "${ESPFLASH_FLAGS}"

[tools.espflash.components]
bootloader = "bootloader-${BOARD}.bin"
partition_table = "partition-table-${BOARD}.bin"
application = "opentyrian.bin"

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

