# Partition Tables Configuration

OpenTyrian uses custom partition tables optimized for each supported board architecture. This document explains the partition table setup and how to configure it for different boards.

## Overview

The project includes architecture-specific partition tables to maximize available storage for game assets while providing sufficient space for the main application:

- **ESP32-P4 boards**: Use `partitions.csv` (3MB app + 11MB storage)
- **ESP32-S3 boards**: Use `partitions_esp32s3_16mb.csv` (3MB app + 12MB storage)

## Partition Table Files

### ESP32-P4 Partition Table (`partitions.csv`)

Used by ESP32-P4 based boards with 16MB flash:

```csv
# Name,      Type,  SubType,  Offset,    Size,   Flags
nvs,         data,  nvs,      0x9000,    0x5000,
phy_init,    data,  phy,      0xe000,    0x2000,
factory,     app,   factory,  0x10000,   3M,
storage,     data,  littlefs, ,          11M,
```

**Layout:**
- **NVS (Non-Volatile Storage)**: 20KB for system configuration
- **PHY Init**: 8KB for RF calibration data
- **Factory App**: 3MB for the main OpenTyrian application
- **LittleFS Storage**: 11MB for game assets and data

### ESP32-S3 Partition Table (`partitions_esp32s3_16mb.csv`)

Used by ESP32-S3 based boards with 16MB flash:

```csv
# Name,      Type,  SubType,  Offset,    Size,   Flags
nvs,         data,  nvs,      0x9000,    0x5000,
phy_init,    data,  phy,      0xe000,    0x2000,
factory,     app,   factory,  0x10000,   3M,
storage,     data,  littlefs, ,          12M,
```

**Layout:**
- **NVS (Non-Volatile Storage)**: 20KB for system configuration
- **PHY Init**: 8KB for RF calibration data  
- **Factory App**: 3MB for the main OpenTyrian application
- **LittleFS Storage**: 12MB for game assets and data

## Board Configuration

### ESP32-P4 Function EV Board
- **Flash Size**: 16MB
- **Partition Table**: `partitions.csv`
- **sdkconfig setting**: `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"`

### M5Stack Tab5 (ESP32-P4)
- **Flash Size**: 16MB
- **Partition Table**: `partitions.csv`
- **sdkconfig setting**: `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"`

### ESP32-S3-BOX-3
- **Flash Size**: 16MB
- **Partition Table**: `partitions_esp32s3_16mb.csv`
- **sdkconfig setting**: `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions_esp32s3_16mb.csv"`

### M5Stack CoreS3
- **Flash Size**: 16MB
- **Partition Table**: `partitions_esp32s3_16mb.csv`
- **sdkconfig setting**: `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions_esp32s3_16mb.csv"`

## Configuration Files

Each board has a corresponding `sdkconfig.defaults.*` file with the correct partition table configuration:

### ESP32-P4 Boards
```bash
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```

### ESP32-S3 Boards
```bash
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions_esp32s3_16mb.csv"
```

## Storage Architecture

### LittleFS Filesystem

The storage partition uses LittleFS filesystem containing:

- **Game Assets**: Original Tyrian sprites, levels, sounds, and music
- **Configuration**: Game settings and preferences
- **Save Data**: Player progress and saved games
- **Build-time Integration**: All assets embedded during build process

### Build Process

During compilation:

1. Game assets are copied from `tyrian/` directory
2. LittleFS image is created with all assets
3. Image is embedded in the storage partition
4. No runtime file system mounting required

## Troubleshooting

### Common Issues

**Partition table not found:**
```
Could not find the input data file partitions.csv
```
**Solution:** Ensure the correct partition table file exists and the filename in sdkconfig matches.

**LittleFS creation failed:**
```
Failed to create LittleFS image for storage partition
```
**Solution:** Check that the storage partition size is sufficient for all game assets.

**Default partition table used:**
```
Using partition table: partitions_singleapp.csv
```
**Solution:** Verify `CONFIG_PARTITION_TABLE_CUSTOM=y` is set and the custom filename is correct.

### Debugging Steps

1. **Verify partition table configuration:**
   ```bash
   grep PARTITION_TABLE sdkconfig
   ```

2. **Check build output for partition table:**
   ```bash
   idf.py partition-table
   ```

3. **Clean build if changing board types:**
   ```bash
   idf.py fullclean
   ```

4. **Verify partition table file exists:**
   ```bash
   ls -la partitions*.csv
   ```

## Memory Layout

### Flash Usage (16MB total)
- **Bootloader**: ~52KB (0x0000 - 0xCFFF)
- **Partition Table**: 4KB (0x8000 - 0x8FFF) 
- **NVS**: 20KB (0x9000 - 0xDFFF)
- **PHY Init**: 8KB (0xE000 - 0xFFFF)
- **Factory App**: 3MB (0x10000 - 0x30FFFF)
- **Storage (P4)**: 11MB (0x310000 - 0xFFFFFF)
- **Storage (S3)**: 12MB (0x310000 - 0xFFFFFF)

### PSRAM Usage
- **Frame Buffers**: Allocated in PSRAM to preserve internal RAM
- **Game Assets**: Loaded from flash to PSRAM when needed
- **Stack/Heap**: Mixed internal RAM and PSRAM based on allocation

## Custom Modifications

To modify partition tables for custom requirements:

1. **Edit partition CSV file** with new sizes
2. **Update corresponding sdkconfig.defaults file**
3. **Ensure total size doesn't exceed flash capacity**
4. **Test with `idf.py partition-table`**
5. **Perform full clean build**

### Example: Larger App Partition

To increase app partition to 4MB and reduce storage:

```csv
# ESP32-P4 custom (4MB app + 10MB storage)
nvs,         data,  nvs,      0x9000,    0x5000,
phy_init,    data,  phy,      0xe000,    0x2000,
factory,     app,   factory,  0x10000,   4M,
storage,     data,  littlefs, ,          10M,
```

Remember to verify that all game assets still fit in the reduced storage partition.