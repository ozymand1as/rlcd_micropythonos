import logging
import time

import lvgl as lv
import machine
import mpos.ui
from mpos import InputManager

logger = logging.getLogger(__name__)

if __debug__:
    logger.debug("waveshare_rlcd_42.py initialization")

import st7305

ST7305_PIN_MOSI = 11
ST7305_PIN_SCK = 12
ST7305_PIN_DC = 5
ST7305_PIN_CS = 40
ST7305_PIN_RST = 41
ST7305_PIN_MISO = 13
ST7305_SPI_HOST = 3
ST7305_SPI_FREQ = 24 * 1000 * 1000

try:
    st7305.init(
        mosi=ST7305_PIN_MOSI,
        sck=ST7305_PIN_SCK,
        dc=ST7305_PIN_DC,
        cs=ST7305_PIN_CS,
        rst=ST7305_PIN_RST,
        miso=ST7305_PIN_MISO,
        spi_host=ST7305_SPI_HOST,
        freq=ST7305_SPI_FREQ,
    )
except Exception as e:
    logger.error("ST7305 init failed: %s" % (e,))
    time.sleep(3)
    machine.reset()

st7305.clear()

display = lv.display_create(st7305.WIDTH, st7305.HEIGHT)
lv.display_set_color_format(display, lv.COLOR_FORMAT.I1)

PARTIAL_ROWS = 40
buf_rows = PARTIAL_ROWS
buf_size = ((st7305.WIDTH + 7) // 8) * buf_rows + 8
buf1 = bytes(buf_size)
buf2 = bytes(buf_size)
lv.display_set_buffers(display, buf1, buf2, buf_size, lv.DISP_RENDER_MODE.PARTIAL)

mpos.ui.main_display = display


def flush_cb(disp, area, color_map):
    st7305.flush(buf=bytes(color_map), x0=area.x1, y0=area.y1, x1=area.x2, y1=area.y2)
    lv.display_flush_ready(disp)


lv.display_set_flush_cb(display, flush_cb)

KEY_GPIO = 18
KEY_ACTIVE_LEVEL = 0

try:
    key_pin = machine.Pin(KEY_GPIO, machine.Pin.IN, machine.Pin.PULL_UP)

    def key_read(indev_drv, data):
        state = key_pin.value()
        if state == KEY_ACTIVE_LEVEL:
            data.state = lv.INDEV_STATE.PRESSED
            data.key = lv.KEY.ENTER
        else:
            data.state = lv.INDEV_STATE.RELEASED
            data.key = lv.KEY.ENTER
        return False

    indev = lv.indev_create()
    lv.indev_set_type(indev, lv.INDEV_TYPE.BUTTON)
    lv.indev_set_read_cb(indev, key_read)

    group = lv.group_create()
    lv.indev_set_group(indev, group)
    mpos.ui.main_group = group

except Exception as e:
    logger.error("Button init failed: %s" % (e,))

I2C_SDA = 13
I2C_SCL = 14

try:
    i2c_bus = machine.I2C(0, sda=machine.Pin(I2C_SDA), scl=machine.Pin(I2C_SCL), freq=100000)

    from mpos.hardware.cardkb import CardKB

    cardkb = CardKB(i2c_bus)

    def cardkb_read(indev_drv, data):
        key = cardkb.get_key()
        if key != 0:
            lv_key, valid = cardkb.to_lvgl_key(key)
            if valid:
                data.key = lv_key
                data.state = lv.INDEV_STATE.PRESSED
                return False
        data.state = lv.INDEV_STATE.RELEASED
        return False

    cardkb_indev = lv.indev_create()
    lv.indev_set_type(cardkb_indev, lv.INDEV_TYPE.KEYPAD)
    lv.indev_set_read_cb(cardkb_indev, cardkb_read)
    if mpos.ui.main_group:
        lv.indev_set_group(cardkb_indev, mpos.ui.main_group)
    InputManager.register_indev(cardkb_indev)

    if __debug__:
        logger.debug("CardKB initialized on I2C SDA=%s SCL=%s" % (I2C_SDA, I2C_SCL))

except Exception as e:
    logger.warning("CardKB init failed (non-critical): %s" % (e,))

from mpos import BatteryManager


def adc_to_voltage(adc_value):
    return adc_value * 0.00262


BatteryManager.init_adc(3, adc_to_voltage)

if __debug__:
    logger.debug("waveshare_rlcd_42.py finished")
