# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)



set(PROJECT_VER "1.0")
set(PROJECT_VER_NUMBER 1)

set(ESP_MATTER_PATH $ENV{ESP_MATTER_PATH})
set(MATTER_SDK_PATH ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip)





set(EXTRA_COMPONENT_DIRS
    # "${ESP_MATTER_PATH}/examples/common/"
    "${MATTER_SDK_PATH}/config/esp32/components"
    "${ESP_MATTER_PATH}/components"
    "hal"
    ${extra_components_dirs_append})



# each transport variant uses its own generated sdkconfig file : SDKCONFIG_DEFAULTS is only
# applied when the sdkconfig does not exist yet, so sharing one sdkconfig between variants
# would silently keep the previous transport
if(DEFINED SDKCONFIG_DEFAULTS)
    if("${SDKCONFIG_DEFAULTS}" MATCHES "c6_wifi")
        set(SDKCONFIG "${CMAKE_CURRENT_SOURCE_DIR}/sdkconfig.c6_wifi")
    elseif("${SDKCONFIG_DEFAULTS}" MATCHES "c6_thread")
        set(SDKCONFIG "${CMAKE_CURRENT_SOURCE_DIR}/sdkconfig.c6_thread")
    endif()
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(matter_min_onoff_led)





idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H;-Wno-overloaded-virtual" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)
# For RISCV chips, project_include.cmake sets -Wno-format, but does not clear various
# flags that depend on -Wformat
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security" APPEND)


# put the file mfg.bin in the partition 'fctry'; contains certifs (DAC, pairing id, ...)
# the file mfg.bin is generated with `python3 generate_mfg_bin.py`, which reads
# mfg_config.csv and names its output folder after the device type and serial
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/mfg_config.csv" MFG_CONFIG_LINES)
foreach(line ${MFG_CONFIG_LINES})
    if(line MATCHES "^(device-type|serial-num),[ \t]*([^,]+)")
        set(key "${CMAKE_MATCH_1}")  # string(REGEX ...) below overwrites CMAKE_MATCH_*
        # same slug rule as generate_mfg_bin.py: lowercase, non-alphanumerics to '_'
        string(TOLOWER "${CMAKE_MATCH_2}" slug)
        string(REGEX REPLACE "[^a-z0-9]+" "_" slug "${slug}")
        string(REGEX REPLACE "^_+|_+$" "" slug "${slug}")
        if(key STREQUAL "device-type")
            set(MFG_DEVICE_TYPE "${slug}")
        else()
            set(MFG_SERIAL_NUM "${slug}")
        endif()
    endif()
endforeach()
set(MFG_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/out_mfg_bin_somef_${MFG_DEVICE_TYPE}_${MFG_SERIAL_NUM}")
spiffs_create_partition_image(fctry ${MFG_BIN_DIR}/mfg.bin FLASH_IN_PROJECT)