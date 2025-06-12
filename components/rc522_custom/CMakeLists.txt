idf_component_register(
    SRCS "rc522_custom.c"
    INCLUDE_DIRS "include"
    REQUIRES driver
    REQUIRES rc522
)