# OpenTyrian - ESP32 Port

This is a port of OpenTyrian to the ESP32 platform, originally ported to ESP32 by Gadget Workbench and updated for new hardware with support for [Board Support Packages](https://components.espressif.com/components?q=Board+Support+Package).

The fork was updated to use all the game data directly from flash, as SD cards have a tendency to be unreliable. It now works with ESP-IDF 5.3 and utilizes the latest [SDL3](https://components.espressif.com/components/georgik/sdl/versions/3.1.2~2) available at the Espressif Component Registry.

## Storyline

OpenTyrian is an open-source port of the DOS game Tyrian.

Tyrian is an arcade-style vertical scrolling shooter. The story is set in 20,031 where you play as Trent Hawkins, a skilled fighter-pilot employed to fight MicroSol and save the galaxy.

## Requirements

- ESP-IDF 5.0 or later
- [ESP32-S3-BOX](https://components.espressif.com/components/espressif/esp-box-3)
- [ESP32-S3-BOX-3](https://components.espressif.com/components/espressif/esp-box-3)
- [ESP32-P4](https://components.espressif.com/components/espressif/esp32_p4_function_ev_board_noglib)
- [M5Stack-CoreS3](https://components.espressif.com/components/espressif/m5stack_core_s3)

## Installation

1. Clone the repository and prepare the data:
   ```
   ./prepare-data.sh
   ```

2. Configure the build environment for your board:

   For ESP32-S3-BOX-3:
   ```
   idf.py @boards/esp-box-3.cfg set-target esp32-s3
   idf.py @boards/esp-box-3.cfg build
   ```

   For ESP32-S3-BOX (prior to Dec. 2023):
   ```
   idf.py @boards/esp-box.cfg reconfigure
   ```

   For ESP32-P4:
   ```
   idf.py @boards/esp32_p4_function_ev_board.cfg reconfigure
   ```

   For M5Stack-CoreS3:
   ```
   idf.py @boards/m5stack_core_s3.cfg reconfigure
   ```

3. Build and flash the firmware:
   ```
   idf.py flash monitor
   ```

## Building for Different Boards

You can use the `@board/` parameter with `idf.py` to switch between different board configurations:

- ESP32-S3-BOX-3:
  ```
  idf.py @boards/esp-box-3.cfg reconfigure
  ```

- ESP32-S3-BOX (prior Dec. 2023):
  ```
  idf.py @boards/esp-box.cfg reconfigure
  ```

- ESP32-P4:
  ```
  idf.py @boards/esp32_p4_function_ev_board.cfg reconfigure
  ```

- M5Stack-CoreS3:
  ```
  idf.py @boards/m5stack_core_s3.cfg reconfigure
  ```

## Acknowledgements

This port is based on the work of the original OpenTyrian project (https://github.com/jkirsons/OpenTyrian) and an ESP32 port by Gadget Workbench, which was initially created for ESP-WROVER and ESP-IDF 4.2. The current version is compatible with ESP-IDF 5.3 and leverages the ESP-BSP for board support. The updated port uses SDL3, available on the [Espressif Component Registry](https://components.espressif.com/components/georgik/sdl/).

## Original Video

[![Alt text](https://img.youtube.com/vi/UL5eTUv7SZE/0.jpg)](https://www.youtube.com/watch?v=UL5eTUv7SZE)

## Credits

Special thanks to all contributors and the open-source community for making this project possible.
