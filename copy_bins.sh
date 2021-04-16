#!/usr/bin/env bash

copy_binary () {
  cp .pio/build/${1}/firmware.bin bin/${1}_firmware.bin
  cp .pio/build/${1}/partitions.bin bin/${1}_partitions.bin
  cp .pio/build/${1}/spiffs.bin bin/${1}_spiffs.bin
}

copy_binary "d32_pro_tft"
copy_binary "lcd_ssd1306"
copy_binary "tft_espi"
copy_binary "m5stickc"
