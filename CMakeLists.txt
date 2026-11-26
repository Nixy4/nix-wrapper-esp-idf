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
    "src/misc/*.cpp"
  "src/misc/console/*.cpp"
)

# Exclude display-dsi.cpp and ili9881c.cpp from common sources
list(FILTER SOURCES EXCLUDE REGEX ".*display-dsi\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX ".*ili9881c\\.cpp$")
# list(FILTER SOURCES EXCLUDE REGEX ".*elf_loader\\.cpp$")

# u8g2 HAL 源码保留在 src/misc/ 中，但不参与编译（不引入 u8g2 组件依赖）。
# 如需启用，请在本行注释掉后自行在 components/ 下提供 u8g2 组件。
list(FILTER SOURCES EXCLUDE REGEX ".*u8g2-hal-esp-idf\\.cpp$")

# board 源码已拆分到 nix-board-esp-idf，wrapper 不再编译任何 board 源码。
# p4 专用 local/device 源码仍需按目标加入。
if(IDF_TARGET STREQUAL "esp32p4")
    list(APPEND SOURCES "src/local/display-dsi.cpp")
    list(APPEND SOURCES "src/device/ili9881c.cpp")
endif()

idf_component_register(
  SRCS ${SOURCES}
  INCLUDE_DIRS ${INCLUDE_DIRS}
  REQUIRES ${REQUIRES}
  PRIV_REQUIRES ${PRIV_REQUIRES}
  EMBED_FILES ${EMBED_FILES}
)
