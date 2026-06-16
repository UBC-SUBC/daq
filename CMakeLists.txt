cmake_minimum_required(VERSION 3.20.0)
set(DTC_OVERLAY_FILE app.overlay)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(daq)

add_subdirectory(driver/sd_card)
add_subdirectory(driver/BME280)
add_subdirectory(driver/bar30)

target_sources(app PRIVATE src/main.c)
