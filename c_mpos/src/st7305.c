#include "st7305.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "st7305";

st7305_state_t st7305_state = {0};

void st7305_hw_init(int mosi, int sck, int dc, int cs, int rst, int miso, int spi_host, int freq) {
    st7305_state.dc_pin = dc;
    st7305_state.cs_pin = cs;
    st7305_state.rst_pin = rst;

    spi_bus_config_t buscfg = {
        .miso_io_num = miso,
        .mosi_io_num = mosi,
        .sclk_io_num = sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ST7305_WIDTH * ST7305_HEIGHT / 8,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(spi_host, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = freq,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(spi_host, &devcfg, &st7305_state.spi));

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << dc) | (1ULL << cs) | (1ULL << rst),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_level(st7305_state.cs_pin, 1);

    st7305_state.swap_buf = heap_caps_malloc(ST7305_SPI_SWAP_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

    ESP_LOGI(TAG, "SPI host=%d mosi=%d sck=%d dc=%d cs=%d rst=%d freq=%d",
             spi_host, mosi, sck, dc, cs, rst, freq);
}

void st7305_hw_reset(void) {
    gpio_set_level(st7305_state.rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(st7305_state.rst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(st7305_state.rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

void st7305_send_cmd(uint8_t cmd) {
    gpio_set_level(st7305_state.dc_pin, 0);
    gpio_set_level(st7305_state.cs_pin, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(st7305_state.spi, &t);
    gpio_set_level(st7305_state.cs_pin, 1);
}

void st7305_send_data(const uint8_t *data, int len) {
    if (len == 0) return;
    gpio_set_level(st7305_state.dc_pin, 1);
    gpio_set_level(st7305_state.cs_pin, 0);

    while (len > 0) {
        int chunk = len > ST7305_SPI_SWAP_BUF_SIZE ? ST7305_SPI_SWAP_BUF_SIZE : len;
        if (data != st7305_state.swap_buf) {
            memcpy(st7305_state.swap_buf, data, chunk);
        }
        spi_transaction_t t = {
            .length = chunk * 8,
            .tx_buffer = st7305_state.swap_buf,
        };
        spi_device_polling_transmit(st7305_state.spi, &t);
        data += chunk;
        len -= chunk;
    }

    gpio_set_level(st7305_state.cs_pin, 1);
}

void st7305_init_registers(void) {
    st7305_hw_reset();

    st7305_send_cmd(0xD6);
    { uint8_t d[] = {0x17, 0x02}; st7305_send_data(d, 2); }

    st7305_send_cmd(0xD1);
    { uint8_t d[] = {0x01}; st7305_send_data(d, 1); }

    st7305_send_cmd(0xC0);
    { uint8_t d[] = {0x11, 0x04}; st7305_send_data(d, 2); }

    st7305_send_cmd(0xC1);
    { uint8_t d[] = {0x69, 0x69, 0x69, 0x69}; st7305_send_data(d, 4); }

    st7305_send_cmd(0xC2);
    { uint8_t d[] = {0x19, 0x19, 0x19, 0x19}; st7305_send_data(d, 4); }

    st7305_send_cmd(0xC4);
    { uint8_t d[] = {0x4B, 0x4B, 0x4B, 0x4B}; st7305_send_data(d, 4); }

    st7305_send_cmd(0xC5);
    { uint8_t d[] = {0x19, 0x19, 0x19, 0x19}; st7305_send_data(d, 4); }

    st7305_send_cmd(0xD8);
    { uint8_t d[] = {0x80, 0xE9}; st7305_send_data(d, 2); }

    st7305_send_cmd(0xB2);
    { uint8_t d[] = {0x02}; st7305_send_data(d, 1); }

    st7305_send_cmd(0xB3);
    { uint8_t d[] = {0xE5, 0xF6, 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45}; st7305_send_data(d, 10); }

    st7305_send_cmd(0xB4);
    { uint8_t d[] = {0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45}; st7305_send_data(d, 8); }

    st7305_send_cmd(0x62);
    { uint8_t d[] = {0x32, 0x03, 0x1F}; st7305_send_data(d, 3); }

    st7305_send_cmd(0xB7);
    { uint8_t d[] = {0x13}; st7305_send_data(d, 1); }

    st7305_send_cmd(0xB0);
    { uint8_t d[] = {0x64}; st7305_send_data(d, 1); }

    st7305_send_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(200));

    st7305_send_cmd(0xC9);
    { uint8_t d[] = {0x00}; st7305_send_data(d, 1); }

    st7305_send_cmd(0x36);
    { uint8_t d[] = {0x48}; st7305_send_data(d, 1); }

    st7305_send_cmd(0x3A);
    { uint8_t d[] = {0x11}; st7305_send_data(d, 1); }

    st7305_send_cmd(0xB9);
    { uint8_t d[] = {0x20}; st7305_send_data(d, 1); }

    st7305_send_cmd(0xB8);
    { uint8_t d[] = {0x29}; st7305_send_data(d, 1); }

    st7305_send_cmd(0x21);

    st7305_send_cmd(0x2A);
    { uint8_t d[] = {0x00, 0x00, 0x01, 0x2B}; st7305_send_data(d, 4); }

    st7305_send_cmd(0x2B);
    { uint8_t d[] = {0x00, 0x00, 0x01, 0x8F}; st7305_send_data(d, 4); }

    st7305_send_cmd(0x35);
    { uint8_t d[] = {0x00}; st7305_send_data(d, 1); }

    st7305_send_cmd(0xD0);
    { uint8_t d[] = {0xFF}; st7305_send_data(d, 1); }

    st7305_send_cmd(0x38);
    st7305_send_cmd(0x29);

    ESP_LOGI(TAG, "ST7305 init registers done");
}

void st7305_set_window(int x0, int y0, int x1, int y1) {
    st7305_send_cmd(0x2A);
    { uint8_t d[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF}; st7305_send_data(d, 4); }

    st7305_send_cmd(0x2B);
    { uint8_t d[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF}; st7305_send_data(d, 4); }
}

static inline uint8_t expand_i1_to_2bpp(uint8_t i1_byte) {
    uint8_t out = 0;
    for (int i = 7; i >= 0; i--) {
        out <<= 2;
        if (i1_byte & (1 << i)) {
            out |= 0x02;
        }
    }
    return out;
}

void st7305_flush_i1_to_2bpp(const uint8_t *i1_buf, int x0, int y0, int x1, int y1) {
    int w = x1 - x0 + 1;
    int h = y1 - y0 + 1;
    int i1_row_bytes = (w + 7) / 8;
    int out_row_bytes = w / 4;

    st7305_set_window(x0, y0, x1, y1);
    st7305_send_cmd(0x2C);

    gpio_set_level(st7305_state.dc_pin, 1);
    gpio_set_level(st7305_state.cs_pin, 0);

    for (int y = 0; y < h; y++) {
        const uint8_t *row = i1_buf + y * i1_row_bytes;
        for (int x = 0; x < out_row_bytes; x++) {
            uint8_t i1_byte = row[x];
            uint8_t out = expand_i1_to_2bpp(i1_byte);

            if (st7305_state.swap_buf && i1_byte != st7305_state.swap_buf[0]) {
                st7305_state.swap_buf[0] = out;
            }
            spi_transaction_t t = {
                .length = 8,
                .tx_buffer = &out,
            };
            spi_device_polling_transmit(st7305_state.spi, &t);
        }
    }

    gpio_set_level(st7305_state.cs_pin, 1);
}

static mp_obj_t st7305_py_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_mosi, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = ST7305_PIN_MOSI}},
        {MP_QSTR_sck, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = ST7305_PIN_SCK}},
        {MP_QSTR_dc, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = ST7305_PIN_DC}},
        {MP_QSTR_cs, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = ST7305_PIN_CS}},
        {MP_QSTR_rst, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = ST7305_PIN_RST}},
        {MP_QSTR_miso, MP_ARG_INT, {.u_int = ST7305_PIN_MISO}},
        {MP_QSTR_spi_host, MP_ARG_INT, {.u_int = ST7305_SPI_HOST}},
        {MP_QSTR_freq, MP_ARG_INT, {.u_int = ST7305_SPI_FREQ}},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int mosi = args[0].u_int;
    int sck = args[1].u_int;
    int dc = args[2].u_int;
    int cs = args[3].u_int;
    int rst = args[4].u_int;
    int miso = args[5].u_int;
    int spi_host = args[6].u_int;
    int freq = args[7].u_int;

    st7305_hw_init(mosi, sck, dc, cs, rst, miso, spi_host, freq);
    st7305_init_registers();

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(st7305_init_obj, 0, st7305_py_init);

static mp_obj_t st7305_py_flush(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_buf, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_x0, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0}},
        {MP_QSTR_y0, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0}},
        {MP_QSTR_x1, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = ST7305_WIDTH - 1}},
        {MP_QSTR_y1, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = ST7305_HEIGHT - 1}},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_buffer_info_t buf_info;
    mp_get_buffer(args[0].u_obj, &buf_info, MP_BUFFER_READ);

    int x0 = args[1].u_int;
    int y0 = args[2].u_int;
    int x1 = args[3].u_int;
    int y1 = args[4].u_int;

    if (x0 < 0 || y0 < 0 || x1 >= ST7305_WIDTH || y1 >= ST7305_HEIGHT) {
        mp_raise_ValueError(MP_ERROR_TEXT("coordinates out of range"));
    }

    st7305_flush_i1_to_2bpp(buf_info.buf, x0, y0, x1, y1);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(st7305_flush_obj, 0, st7305_py_flush);

static mp_obj_t st7305_py_clear(void) {
    int total = ST7305_BUF_SIZE;
    uint8_t *buf = heap_caps_malloc(total, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) {
        buf = heap_caps_malloc(total, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    }
    if (!buf) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("failed to allocate clear buffer"));
    }
    memset(buf, 0x00, total);

    st7305_set_window(0, 0, ST7305_WIDTH - 1, ST7305_HEIGHT - 1);
    st7305_send_cmd(0x2C);

    gpio_set_level(st7305_state.dc_pin, 1);
    gpio_set_level(st7305_state.cs_pin, 0);

    int sent = 0;
    while (sent < total) {
        int chunk = total - sent;
        if (chunk > ST7305_SPI_SWAP_BUF_SIZE) chunk = ST7305_SPI_SWAP_BUF_SIZE;
        memcpy(st7305_state.swap_buf, buf + sent, chunk);
        spi_transaction_t t = {
            .length = chunk * 8,
            .tx_buffer = st7305_state.swap_buf,
        };
        spi_device_polling_transmit(st7305_state.spi, &t);
        sent += chunk;
    }

    gpio_set_level(st7305_state.cs_pin, 1);
    heap_caps_free(buf);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(st7305_clear_obj, st7305_py_clear);

static const mp_rom_map_elem_t st7305_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&st7305_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&st7305_flush_obj)},
    {MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&st7305_clear_obj)},
    {MP_ROM_QSTR(MP_QSTR_WIDTH), MP_ROM_INT(ST7305_WIDTH)},
    {MP_ROM_QSTR(MP_QSTR_HEIGHT), MP_ROM_INT(ST7305_HEIGHT)},
    {MP_ROM_QSTR(MP_QSTR_BUF_SIZE), MP_ROM_INT(ST7305_BUF_SIZE)},
};
static MP_DEFINE_CONST_DICT(st7305_module_globals, st7305_module_globals_table);

const mp_obj_module_t st7305_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&st7305_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_st7305, st7305_module);
