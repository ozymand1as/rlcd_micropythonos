import logging

import lvgl as lv

logger = logging.getLogger(__name__)

CARDKB_ADDR = 0x5F

_SPECIAL_KEYS = {
    13: lv.KEY.ENTER,
    8: lv.KEY.BACKSPACE,
    9: lv.KEY.TAB,
    27: lv.KEY.ESC,
    127: lv.KEY.DEL,
    32: lv.KEY.SPACE,
    180: lv.KEY.LEFT,
    181: lv.KEY.UP,
    182: lv.KEY.DOWN,
    183: lv.KEY.RIGHT,
}


class CardKB:
    def __init__(self, i2c, addr=CARDKB_ADDR):
        self._i2c = i2c
        self._addr = addr
        self._last_key = 0

    def get_key(self):
        try:
            data = self._i2c.readfrom(self._addr, 1)
            key = data[0]
            self._last_key = key
            return key
        except Exception:
            return 0

    def get_string(self):
        key = self.get_key()
        if key == 0:
            return ""
        return chr(key)

    def is_pressed(self):
        return self.get_key() != 0

    def to_lvgl_key(self, ascii_val):
        if ascii_val == 0:
            return 0, False
        if ascii_val in _SPECIAL_KEYS:
            return _SPECIAL_KEYS[ascii_val], True
        if 32 <= ascii_val <= 126:
            return ascii_val, True
        return 0, False
