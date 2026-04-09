MCU = atmega32u4
BOOTLOADER = caterina

BOOTMAGIC_ENABLE = yes      # Hold top-left on plug = bootloader
MOUSEKEY_ENABLE = yes       # Mouse keys (scroll)
EXTRAKEY_ENABLE = yes       # Audio control, system keys
CONSOLE_ENABLE = no
COMMAND_ENABLE = no
NKRO_ENABLE = no

ENCODER_ENABLE = yes        # 3x EC11
OLED_ENABLE = yes           # 128x64 I2C
OLED_DRIVER = ssd1306
RGBLIGHT_ENABLE = yes       # WS2812B
WS2812_DRIVER = avr

SRC += oled_display.c

LTO_ENABLE = yes