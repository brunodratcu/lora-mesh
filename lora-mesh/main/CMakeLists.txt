idf_component_register(
    SRCS
        "main.c"
        "spi_driver.c"
        "sx1276.c"
        "mesh.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        freertos
        esp_common
        log
)
