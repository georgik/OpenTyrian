cmake_minimum_required(VERSION 3.16)

# ESP-BSP SDL Abstraction Layer Component
# Kconfig-based BSP selection to avoid header conflicts

# Conditional source files based on board selection to avoid compilation errors
set(COMPONENT_SRCS "src/esp_bsp_sdl_common.c")

# Add board-specific sources based on Kconfig selection
if(CONFIG_SDL_BSP_M5_ATOM_S3)
    list(APPEND COMPONENT_SRCS "src/boards/esp_bsp_sdl_m5_atom_s3.c")
    message(STATUS "ESP-BSP SDL: Including M5 Atom S3 source files")
elseif(CONFIG_SDL_BSP_ESP_BOX_3)
    list(APPEND COMPONENT_SRCS "src/boards/esp_bsp_sdl_esp_box_3.c")
    message(STATUS "ESP-BSP SDL: Including ESP32-S3-BOX-3 source files")
elseif(CONFIG_SDL_BSP_M5STACK_CORE_S3)
    list(APPEND COMPONENT_SRCS "src/boards/esp_bsp_sdl_m5stack_core_s3.c")
    message(STATUS "ESP-BSP SDL: Including M5Stack Core S3 source files")
elseif(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV)
    list(APPEND COMPONENT_SRCS "src/boards/esp_bsp_sdl_esp32_p4_function_ev.c")
    message(STATUS "ESP-BSP SDL: Including ESP32-P4 Function EV Board source files")
elseif(CONFIG_SDL_BSP_ESP32_S3_LCD_EV_BOARD)
    list(APPEND COMPONENT_SRCS "src/boards/esp_bsp_sdl_esp32_s3_lcd_ev_board.c")
    message(STATUS "ESP-BSP SDL: Including ESP32-S3-LCD-EV-Board source files")
elseif(CONFIG_SDL_BSP_M5STACK_TAB5)
    list(APPEND COMPONENT_SRCS "src/boards/esp_bsp_sdl_m5stack_tab5.c")
    message(STATUS "ESP-BSP SDL: Including M5Stack Tab5 source files")
else()
    message(WARNING "ESP-BSP SDL: No board selected in menuconfig!")
endif()

# Include only the required BSP dependencies - this is the minimum required
set(COMPONENT_REQUIRES "esp_lcd")
set(COMPONENT_PRIV_REQUIRES "espressif__esp_lcd_touch")

# Conditional BSP selection to avoid symbol conflicts
# Each board BSP is included separately to prevent function name conflicts
if(CONFIG_SDL_BSP_M5_ATOM_S3)
    list(APPEND COMPONENT_PRIV_REQUIRES "espressif__m5_atom_s3_noglib")
    message(STATUS "ESP-BSP SDL: Including M5 Atom S3 BSP")
elseif(CONFIG_SDL_BSP_ESP_BOX_3)
    list(APPEND COMPONENT_PRIV_REQUIRES "espressif__esp-box-3_noglib")
    message(STATUS "ESP-BSP SDL: Including ESP32-S3-BOX-3 BSP")
elseif(CONFIG_SDL_BSP_M5STACK_CORE_S3)
    list(APPEND COMPONENT_PRIV_REQUIRES "espressif__m5stack_core_s3_noglib")
    list(APPEND COMPONENT_PRIV_REQUIRES "espressif__esp_lcd_touch")
    message(STATUS "ESP-BSP SDL: Including M5Stack Core S3 BSP")
elseif(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV)
    list(APPEND COMPONENT_PRIV_REQUIRES "espressif__esp32_p4_function_ev_board_noglib")
    list(APPEND COMPONENT_PRIV_REQUIRES "espressif__esp_lcd_touch")
    message(STATUS "ESP-BSP SDL: Including ESP32-P4 Function EV Board BSP")
elseif(CONFIG_SDL_BSP_ESP32_S3_LCD_EV_BOARD)
    list(APPEND COMPONENT_PRIV_REQUIRES "espressif__esp32_s3_lcd_ev_board_noglib")
    list(APPEND COMPONENT_PRIV_REQUIRES "espressif__esp_lcd_touch")
    message(STATUS "ESP-BSP SDL: Including ESP32-S3-LCD-EV-Board BSP")
elseif(CONFIG_SDL_BSP_M5STACK_TAB5)
    list(APPEND COMPONENT_PRIV_REQUIRES "georgik__m5stack_tab5")
    message(STATUS "ESP-BSP SDL: Including M5Stack Tab5 BSP")
else()
    message(WARNING "ESP-BSP SDL: No BSP dependency selected!")
endif()

# This approach ensures:
# 1. All BSP headers are available during compilation
# 2. No dependency resolution timing issues
# 3. Board selection works purely through Kconfig + conditional compilation
# 4. Simple addition of new boards by adding to the list above

# Register the component
idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES ${COMPONENT_REQUIRES}
    PRIV_REQUIRES ${COMPONENT_PRIV_REQUIRES}
)

# Add board selection messages during build (after CONFIG variables are available)
if(CONFIG_SDL_BSP_M5_ATOM_S3)
    message(STATUS "ESP-BSP SDL: Building for M5 Atom S3 (128x128, no PSRAM)")
elseif(CONFIG_SDL_BSP_M5STACK_CORE_S3)
    message(STATUS "ESP-BSP SDL: Building for M5Stack CoreS3 (320x240, QUAD PSRAM)")
elseif(CONFIG_SDL_BSP_ESP_BOX_3)
    message(STATUS "ESP-BSP SDL: Building for ESP32-S3-BOX-3 (320x240, OCTAL PSRAM)")
elseif(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV)
    message(STATUS "ESP-BSP SDL: Building for ESP32-P4 Function EV Board (1280x800, 32MB PSRAM)")
elseif(CONFIG_SDL_BSP_ESP32_S3_LCD_EV_BOARD)
    message(STATUS "ESP-BSP SDL: Building for ESP32-S3-LCD-EV-Board (800x480, OCTAL PSRAM)")
elseif(CONFIG_SDL_BSP_M5STACK_TAB5)
    message(STATUS "ESP-BSP SDL: Building for M5Stack Tab5 (1280x720, 32MB PSRAM, MIPI-DSI)")
else()
    message(WARNING "ESP-BSP SDL: No specific board detected in configuration. Check menuconfig.")
endif()
