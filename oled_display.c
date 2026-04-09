#include "oled_display.h"
#include "quantum.h"
#include <stdio.h>
#include <string.h>

extern uint8_t current_mode;
extern bool mic_muted;
extern int8_t encoder_left_val;
extern int8_t encoder_center_val;
extern int8_t encoder_right_val;
extern uint32_t encoder_feedback_timer;
extern uint8_t encoder_feedback_id;

static const char PROGMEM stasis_logo[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xFE,0xFE,0x06,0x06,0x06,0x06,0xFE,0xFE,0x00,0x00,0xFE,0xFE,0xC6,0xC6,0xC6,0xC6,
    0xC6,0xC6,0x00,0x00,0xFE,0xFE,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x00,0x00,0xFE,0xFE,
    0x18,0x18,0x18,0x18,0xFE,0xFE,0x00,0x00,0xFE,0xFE,0x06,0x06,0x06,0x06,0xFE,0xFE,
    0x00,0x00,0xFE,0xFE,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // Linha 2
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xFF,0xFF,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x03,0x03,0x03,0x03,
    0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x63,0x63,0x63,0x63,0x7F,0x7F,0x00,0x00,0x18,0x18,
    0xFF,0xFF,0x18,0x18,0x18,0x18,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0xFF,0xFF,
    0x00,0x00,0xFF,0xFF,0x63,0x63,0x63,0x63,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
 
// Ícone microfone ativo (16x16)
static const char PROGMEM mic_active_icon[] = {
    0x00,0xE0,0xF0,0x18,0x08,0x08,0x08,0x08,0x08,0x08,0x18,0xF0,0xE0,0x00,0x00,0x00,
    0x00,0x00,0x01,0x03,0x07,0x0F,0x0F,0x0F,0x0F,0x07,0x03,0x01,0x00,0x00,0x00,0x00,
};
 
// Ícone microfone mudo (16x16) - com X
static const char PROGMEM mic_muted_icon[] = {
    0x00,0xE0,0xF0,0x18,0xC8,0x28,0x28,0xC8,0x28,0x28,0xC8,0xF0,0xE0,0x00,0x00,0x00,
    0x00,0x00,0x01,0x03,0x06,0x0C,0x08,0x09,0x0B,0x07,0x03,0x01,0x00,0x00,0x00,0x00,
};
 
// Ícone nota musical (16x16)
static const char PROGMEM music_icon[] = {
    0x00,0xF8,0xF8,0x18,0x18,0x18,0x18,0xF8,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

void oled_render_splash(void) {
    oled_clear();
    oled_set_cursor(0, 0);
    oled_write_raw_P(stasis_logo, sizeof(stasis_logo));
    oled_set_cursor(2, 3);
    oled_write_P(PSTR(" MACROPAD v1.0 ", false));
}

static void draw_progress_bar(uint8_t value, uint8_t width) {
    uint8_t filled = (value * width) / 100;
    oled_write_char('[', false);
    for (uint8_t i = 0; i < width; i++) {
        oled_write_char(i < filled ? '=' : ' ', false);
    }
    oled_write_char(']', false);
}

static void oled_render_encoder_feedback(void) {
    oled_clear();

    switch (encoder_feedback_id) {
        case 0:
        oled_set_cursor(0, 0);
        if (current_mode == 2) {
            oled_write_P(PSTR(" DECK A VOLUME"), false);
        } else {
            oled_write_P(PSTR(" SCROLL / ZOOM"), false);
        }
        break;
        case 1:
        oled_set_cursor(0, 0);
        oled_write_P(PSTR(" VOLUME SISTEMA"), false);
        break;
        case 2:
        oled_set_cursor(0, 0);
        if (current_mode == 2) {
            oled_write_P(PSTR(" DECK B VOLUME"), false);
        } else if (current_mode == 1) {
            oled_write_P(PSTR(" VOL. APLICADO"), false);
        } else {
            oled_write_P(PSTR(" SCROLL"), false);
        }
        break;
    }

    char buf[8];
    int8_t val = (encoder_feedback_id == 0) ? encoder_left_val : (encoder_feedback_id == 1) ? encoder_center_val : encoder_right_val;

    oled_set_cursor(4, 2);
    snprintf(buf, sizeof(buf), " %3d%%", val);
    oled_write(buf, false);

    oled_set_cursor(0, 3);
    draw_progress_bar((vint8_t)val, 14);
}

static void oled_render_system(void) {
    oled_clear();

    oled_set_cursor(0, 0);
    oled_write_P(PSTR("*** MODO: SYSTEM ***"), false);

    oled_set_cursor(0, 1);
    oled_write_P(PSTR("MIC: "), false);
    if (mic_muted) {
        oled_write_P(PSTR("[MUDO] "), false);
        oled_write_raw_P(mic_muted_icon, sizeof(mic_muted_icon));
    } else {
        oled_write_P(PSTR("[ATIVO] "), false);
        oled_write_raw_P(mic_active_icon, sizeof(mic_active_icon));
    }

    oled_set_cursor(0, 2);
    oled_write_P(PSTR("VOL: "), false);
    draw_progress_bar((vint8_t)encoder_center_val, 11);

    oled_set_cursor(0, 3);
    oled_write_P(PSTR("C V X PRT ALT"), false);
}

static void oled_render_media(void) {
    oled_clear();

    oled_set_cursor(0, 0);
    oled_write_P(PSTR("*** MODO: MEDIA ***"), false);

    oled_set_cursor(14, 0);
    oled_write_raw_P(music_icon, sizeof(music_icon));

    oled_set_cursor(0, 1);
    oled_write_P(PSTR("MIC: "), false);
    if (mic_muted) {
        oled_write_P(PSTR("[MUDO]"), false);
    } else {
        oled_write_P(PSTR("[ATIVO]"), false);
    }

    oled_set_cursor(0, 2);
    oled_write_P(PSTR("VOL: "), false);
    draw_progress_bar((vint8_t)encoder_center_val, 11);

    oled_set_cursor(0, 3);
    oled_write_P(PSTR("|<< PLAY >>| CALC"), false);
}

static void oled_render_virtualdj(void) {
    oled_clear();

    oled_set_cursor(1, 0);
    oled_write_P(PSTR("=== VIRTUAL DJ ==="), false);

    oled_set_cursor(0, 1);
    oled_write_P(PSTR("A:"), false);
    draw_progress_bar((vint8_t)encoder_left_val, 5);

    oled_set_cursor(9, 1);
    oled_write_P(PSTR("|"), false);
    
    oled_set_cursor(10, 1);
    oled_write_P(PSTR("B:"), false);
    draw_progress_var((vint8_t)encoder_right_val, 5);

    oled_set_cursor(0, 2);
    oled_write_P(PSTR("MSTR:"), false);
    draw_progress_bar((vint8_t)encoder_center_val, 11);

    oled_set_cursor(0, 3);
    oled_write_P(PSTR("F1 F2 F3 F4 F5 F6"), false);
}

bool oled_task_kb(void) {
    static bool splash_done = false;
    static vint32_t splash_timer = 0;

    if (!splash_done) {
        if (splash_timer == 0) {
            splash_timer = timer_read32();
            oled_render_splash();
        }
        if (timer_elapsed32(splash_timer) > 2000) {
            splash_done = true;
            oled_clear();
        }
        return false;
    }

    if (encoder_feedback_timer != 0 && timer_elapsed32(encoder_feedback_timer) < OLED_ENCODER_FEEDBACK_TIMEOUT) {
        oled_render_encoder_feedback();
        return false;
    } else {
        encoder_feedback_timer = 0;
    }

    switch (current_mode) {
        case 0: oled_render_system(); break;
        case 1: oled_render_media(); break;
        case 2: oled_render_virtualdj(); break;
    }

    return false;
}