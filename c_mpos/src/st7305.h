#ifndef ST7305_H
#define ST7305_H

#include "py/obj.h"

#define ST7305_WIDTH 300
#define ST7305_HEIGHT 400
#define ST7305_BUF_SIZE ((ST7305_WIDTH * ST7305_HEIGHT) / 8)

#define ST7305_PIN_MOSI 11
#define ST7305_PIN_SCK  12
#define ST7305_PIN_DC   5
#define ST7305_PIN_CS   40
#define ST7305_PIN_RST  41
#define ST7305_PIN_MISO 13
#define ST7305_SPI_HOST 3
#define ST7305_SPI_FREQ (24 * 1000 * 1000)

#define ST7305_SPI_SWAP_BUF_SIZE 4096

typedef struct {
    spi_device_handle_t spi;
    int dc_pin;
    int cs_pin;
    int rst_pin;
    uint8_t *swap_buf;
} st7305_state_t;

extern st7305_state_t st7305_state;

void st7305_hw_init(int mosi, int sck, int dc, int cs, int rst, int miso, int spi_host, int freq);
void st7305_hw_reset(void);
void st7305_send_cmd(uint8_t cmd);
void st7305_send_data(const uint8_t *data, int len);
void st7305_set_window(int x0, int y0, int x1, int y1);
void st7305_flush_i1_to_2bpp(const uint8_t *i1_buf, int x0, int y0, int x1, int y1);

#endif
