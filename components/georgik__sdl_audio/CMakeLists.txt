idf_component_register(
    SRCS "src/SDL_audio_esp32.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_codec_dev espressif__esp32_p4_function_ev_board_noglib esp_lcd
)
