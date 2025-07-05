 #include "rc522_custom.h"
 
 #include <stdio.h>
 #include <string.h>

 #include "driver/rc522_spi.h"
 #include "rc522.h"
 #include "rc522_picc.h"

 #include "esp_log.h"
 #include "esp_lcd.h"

#define RC522_SPI_BUS_GPIO_MISO    (19)
#define RC522_SPI_BUS_GPIO_MOSI    (23)
#define RC522_SPI_BUS_GPIO_SCLK    (18)
#define RC522_SPI_SCANNER_GPIO_SDA (5)
#define RC522_SCANNER_GPIO_RST     (22)

QueueHandle_t rc522_custom_queue = NULL;

static rc522_spi_config_t driver_config = {
    .host_id = SPI3_HOST,
    .bus_config = &(spi_bus_config_t){
        .miso_io_num = RC522_SPI_BUS_GPIO_MISO,
        .mosi_io_num = RC522_SPI_BUS_GPIO_MOSI,
        .sclk_io_num = RC522_SPI_BUS_GPIO_SCLK,
    },
    .dev_config = {
        .spics_io_num = RC522_SPI_SCANNER_GPIO_SDA,
    },
    .rst_io_num = RC522_SCANNER_GPIO_RST,
};

static rc522_driver_handle_t driver;
static rc522_handle_t scanner;

static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    if (picc->state == RC522_PICC_STATE_ACTIVE) {
        lcd_scanned();
        char rc522_custom_uid_buffer[RC522_CUSTOM_UID_BUFFER_SIZE];
        rc522_picc_uid_to_str(&picc->uid, rc522_custom_uid_buffer, RC522_CUSTOM_UID_BUFFER_SIZE);
        xQueueSendFromISR(rc522_custom_queue, rc522_custom_uid_buffer, NULL);
    }
    else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE) {

    }
}

void rc522_custom_init(void)
{
    rc522_custom_queue=xQueueCreate(1, RC522_CUSTOM_UID_BUFFER_SIZE);
    rc522_spi_create(&driver_config, &driver);
    rc522_driver_install(driver);

    rc522_config_t scanner_config = {
        .driver = driver,
    };

    rc522_create(&scanner_config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED, on_picc_state_changed, NULL);
}

void rc522_custom_start(void)
{
    rc522_start(scanner);
}

void rc522_custom_pause(void)
{
    rc522_pause(scanner);
}