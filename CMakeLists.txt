cmake_minimum_required(VERSION 3.16)

set(COMPONENTS
    main
    OpenTyrian
) # "Trim" the build. Include the minimal set of components; main and anything it depends on.

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-open-tyrian)

get_filename_component(configName "${CMAKE_BINARY_DIR}" NAME)
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/components/esp_littlefs")

littlefs_create_partition_image(storage data FLASH_IN_PROJECT)
