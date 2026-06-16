add_subdirectory(Bosch_driver)

target_sources(app PRIVATE custom_bme280_driver.c)

target_include_directories(app PRIVATE .)
