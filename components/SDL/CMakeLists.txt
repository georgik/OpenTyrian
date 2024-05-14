idf_component_register(
    SRCS "SDL_system.c" "SDL_video.c" "SDL_input.c" "spi_lcd.c" "SDL_event.c" "SDL_audio.c"
    INCLUDE_DIRS "." "../OpenTyrian"
    REQUIRES driver fatfs esp_timer espressif__esp-box-3 esp_lcd littlefs
)
