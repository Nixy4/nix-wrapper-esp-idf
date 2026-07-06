set(INCLUDE_DIRS "src")
set(REQUIRES_ESP_IDF
  "log"
  "console"
  "efuse"
  "nvs_flash"
  "esp_event"
  "esp_wifi"
  "esp_timer"
  "esp_lcd"
  "esp_driver_gpio"
  "esp_driver_i2c"
  "esp_driver_i2s"
  "esp_driver_spi"
  "esp_driver_uart"
  "esp_driver_ledc"
  "esp_driver_usb_serial_jtag"
  "fatfs"
  "wear_levelling"
  "spiffs"
  "esp_driver_sdmmc"
  "esp_driver_sdspi"
  "sdmmc"
  "spi_flash"
  "bootloader_support"
  "app_update"
  "esp_partition"
  "esp_rom"
  "esp_https_ota"
  "esp_http_client"
  "esp_app_format"
)
set(REQUIRES_ESP_REGISTRY "")
set(REQUIRES_PROJECT "")

set(REQUIRES ${REQUIRES_ESP_IDF} ${REQUIRES_ESP_REGISTRY} ${REQUIRES_PROJECT})
set(PRIV_REQUIRES "")

set(EMBED_FILES "")

# Common sources
file(GLOB_RECURSE SOURCES 
    "src/local/*.cpp" 
    "src/registry/*.cpp"
    "src/device/*.cpp" 
    "src/app/*.cpp"
)

# Exclude display-dsi.cpp and ili9881c.cpp from common sources
list(FILTER SOURCES EXCLUDE REGEX ".*display-dsi\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX ".*ili9881c\\.cpp$")
# list(FILTER SOURCES EXCLUDE REGEX ".*elf_loader\\.cpp$")

# Add PowerHub source
list(APPEND SOURCES "src/board/m5stack/powerhub.cpp")

# Board specific sources
if(IDF_TARGET STREQUAL "esp32s3")
  if( CONFIG_WRAPPER_ESP32_BOARD_M5STACK_CORE_S3 )
    list(APPEND SOURCES "src/board/m5stack/core-s3.cpp")
  elseif( CONFIG_WRAPPER_ESP32_BOARD_M5STACK_CARDPUTER )
    list(APPEND SOURCES "src/board/m5stack/cardputer.cpp")
  elseif( CONFIG_WRAPPER_ESP32_BOARD_LILYGO_T_LORA_PAGER )
    list(APPEND SOURCES "src/board/lilygo/t-lora-pager.cpp")
  endif()
elseif(IDF_TARGET STREQUAL "esp32p4")
    list(APPEND SOURCES "src/local/display-dsi.cpp")
    list(APPEND SOURCES "src/device/ili9881c.cpp")
  if( CONFIG_WRAPPER_ESP32_BOARD_M5STACK_TAB5 )
    list(APPEND SOURCES "src/board/m5stack/tab5.cpp")
  endif()
endif()

idf_component_register(
  SRCS ${SOURCES}
  INCLUDE_DIRS ${INCLUDE_DIRS}
  REQUIRES ${REQUIRES}
  PRIV_REQUIRES ${PRIV_REQUIRES}
  EMBED_FILES ${EMBED_FILES}
)
