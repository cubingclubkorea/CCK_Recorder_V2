idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")

set(image ../data)
# fatfs_create_spiflash_image(storage ${image} FLASH_IN_PROJECT PRESERVE_TIME)