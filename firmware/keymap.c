// ============================================================
// STASIS MACROPAD - keymap.c
// 3 Modos: SYSTEM / MEDIA / VIRTUALDJ
// Hardware: Pro Micro ATmega32U4
// ============================================================

#include QMK_KEYBOARD_H
#include "oled_display.h"
#include <string.h>

// ============================================================
// ESTADO GLOBAL
// ============================================================

uint8_t  current_mode         = 0;      // 0=SYSTEM, 1=MEDIA, 2=VIRTUALDJ
bool     mic_muted            = true;   // Começa como mudo
int8_t   encoder_left_val     = 50;     // Valores 0-100
int8_t   encoder_center_val   = 50;
int8_t   encoder_right_val    = 50;
uint32_t encoder_feedback_timer = 0;
uint8_t  encoder_feedback_id  = 0;

// ============================================================
// DEFINIÇÃO DE KEYCODES CUSTOMIZADOS
// ============================================================

enum custom_keycodes {
    KC_MODE_SWITCH = SAFE_RANGE,  // D1/TX: troca de modo cíclica
    KC_PTT_LARGE,                 // D10/A10: Push-to-Talk / DJ Play
};

// ============================================================
// LAYOUT — Direct Pins (1 linha, 8 colunas)
//
// Index: [ 0          1          2          3
//           4          5          6          7      ]
// Pino:  [ D1/TX      D4/A6      D5         D6/A7
//           D7         D8/A8      D9/A9      D10/A10]
// Func:  [ ModeSwitch SW1        SW2        SW3
//           SW4        SW5        SW6        PTT    ]
//
// Encoders geridos por encoder_update_user() — fora da matriz
// ============================================================

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    // --------------------------------------------------------
    // LAYER 0: SYSTEM / Produtividade
    // --------------------------------------------------------
    [0] = LAYOUT(
     // ModeSwitch   SW1            SW2            SW3
        KC_MODE_SWITCH, LCTL(KC_C), LCTL(KC_V),    LCTL(KC_X),
     // SW4             SW5         SW6             PTT
        KC_PSCR,        LALT(KC_F4),LCTL(LSFT(KC_ESC)), KC_PTT_LARGE
    ),

    // --------------------------------------------------------
    // LAYER 1: MEDIA & Navi
    // --------------------------------------------------------
    [1] = LAYOUT(
     // ModeSwitch   SW1       SW2        SW3
        KC_MODE_SWITCH, KC_MPRV, KC_MPLY,  KC_MNXT,
     // SW4             SW5      SW6       PTT
        KC_CALC,        KC_WBAK, KC_WFWD,  KC_PTT_LARGE
    ),

    // --------------------------------------------------------
    // LAYER 2: VirtualDJ
    // --------------------------------------------------------
    [2] = LAYOUT(
     // ModeSwitch   SW1    SW2    SW3
        KC_MODE_SWITCH, KC_F1, KC_F2, KC_F3,
     // SW4             SW5    SW6    PTT
        KC_F4,          KC_F5, KC_F6, KC_PTT_LARGE
    ),
};

// ============================================================
// CORES RGB POR MODO
// ============================================================

typedef struct {
    uint8_t r, g, b;
} rgb_color_t;

// Cores dos 6 LEDs para cada modo
static const rgb_color_t PROGMEM mode_colors[3][6] = {
    // SYSTEM: Branco/Ciano alternado
    [0] = {
        {200, 200, 200}, {0, 200, 200},  {200, 200, 200},
        {0,   200, 200}, {200, 200, 200},{0,   200, 200},
    },
    // MEDIA: Roxo/Rosa alternado
    [1] = {
        {180, 0, 200},   {200, 0, 120},  {180, 0, 200},
        {200, 0, 120},   {180, 0, 200},  {200, 0, 120},
    },
    // VIRTUALDJ: 3 Azuis / 3 Vermelhos
    [2] = {
        {0,   0,   200}, {0,   0,   200}, {0,   0,   200},
        {200, 0,   0},   {200, 0,   0},   {200, 0,   0},
    },
};

static void apply_rgb_mode(uint8_t mode) {
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t r = pgm_read_byte(&mode_colors[mode][i].r);
        uint8_t g = pgm_read_byte(&mode_colors[mode][i].g);
        uint8_t b = pgm_read_byte(&mode_colors[mode][i].b);
        rgblight_setrgb_at(r, g, b, i);
    }
}

// ============================================================
// INICIALIZAÇÃO
// ============================================================

void keyboard_post_init_user(void) {
    // Ativar layer 0
    layer_clear();
    layer_on(0);

    // RGB: modo breathing no início
    rgblight_enable();
    rgblight_mode(RGBLIGHT_MODE_BREATHING + 2);  // breathing médio
    apply_rgb_mode(0);
}

// ============================================================
// TROCA DE MODO
// ============================================================

static void switch_mode(void) {
    current_mode = (current_mode + 1) % 3;

    // Trocar layer
    layer_clear();
    layer_on(current_mode);

    // Aplicar LEDs do novo modo
    apply_rgb_mode(current_mode);

    // No modo SYSTEM e MEDIA: breathing; no DJ: estático
    if (current_mode == 2) {
        rgblight_mode(RGBLIGHT_MODE_STATIC_LIGHT);
    } else {
        rgblight_mode(RGBLIGHT_MODE_BREATHING + 2);
    }
}

// ============================================================
// KEYCODES CUSTOMIZADOS
// ============================================================

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {

        // ---- Troca de modo cíclica (SW11) ----
        case KC_MODE_SWITCH:
            if (record->event.pressed) {
                switch_mode();
            }
            return false;

        // ---- PTT / Switch Grande (SW7) ----
        case KC_PTT_LARGE:
            if (current_mode == 0 || current_mode == 1) {
                // Modos 0 e 1: Discord Push-to-Talk
                // Pressionar E soltar ambos enviam Ctrl+Shift+M
                if (record->event.pressed) {
                    mic_muted = !mic_muted;  // toggle visual
                    tap_code16(LCTL(LSFT(KC_M)));
                } else {
                    tap_code16(LCTL(LSFT(KC_M)));
                }
            } else {
                // Modo 2 (VirtualDJ): Play/Pause = Espaço
                if (record->event.pressed) {
                    tap_code(KC_SPC);
                }
            }
            return false;

        default:
            return true;
    }
}

// ============================================================
// ENCODERS
// Mapeamento:
//   Encoder 0 = Esquerdo  (SW8)
//   Encoder 1 = Central   (SW9)
//   Encoder 2 = Direito   (SW10)
// ============================================================

static void clamp_val(int8_t *val, int8_t delta) {
    *val += delta;
    if (*val > 100) *val = 100;
    if (*val < 0)   *val = 0;
}

static void encoder_feedback(uint8_t id) {
    encoder_feedback_id    = id;
    encoder_feedback_timer = timer_read32();
}

bool encoder_update_user(uint8_t index, bool clockwise) {
    int8_t dir = clockwise ? 5 : -5;  // Passo de 5%

    switch (current_mode) {

        // ---- MODO 0: SYSTEM ----
        case 0:
            if (index == 0) {
                // Encoder Esq: Scroll do rato (cima/baixo)
                if (clockwise) {
                    tap_code(KC_WH_D);
                } else {
                    tap_code(KC_WH_U);
                }
                // Não mostra feedback de valor (scroll não tem valor absoluto)
            } else if (index == 1) {
                // Encoder Central: Volume Sistema
                if (clockwise) {
                    tap_code(KC_VOLU);
                    clamp_val(&encoder_center_val, 5);
                } else {
                    tap_code(KC_VOLD);
                    clamp_val(&encoder_center_val, -5);
                }
                encoder_feedback(1);
            } else if (index == 2) {
                // Encoder Dir: Scroll horizontal
                if (clockwise) {
                    tap_code(KC_WH_R);
                } else {
                    tap_code(KC_WH_L);
                }
            }
            break;

        // ---- MODO 1: MEDIA ----
        case 1:
            if (index == 0) {
                // Encoder Esq: Zoom (Ctrl+Plus/Minus)
                if (clockwise) {
                    tap_code16(LCTL(KC_EQL));
                    clamp_val(&encoder_left_val, 5);
                } else {
                    tap_code16(LCTL(KC_MINS));
                    clamp_val(&encoder_left_val, -5);
                }
                encoder_feedback(0);
            } else if (index == 1) {
                // Encoder Central: Volume Sistema
                if (clockwise) {
                    tap_code(KC_VOLU);
                    clamp_val(&encoder_center_val, 5);
                } else {
                    tap_code(KC_VOLD);
                    clamp_val(&encoder_center_val, -5);
                }
                encoder_feedback(1);
            } else if (index == 2) {
                // Encoder Dir: Volume Aplicação (não há keycode nativo - usar Vol pois
                // maioria das apps responde ao volume do sistema)
                if (clockwise) {
                    tap_code(KC_VOLU);
                    clamp_val(&encoder_right_val, 5);
                } else {
                    tap_code(KC_VOLD);
                    clamp_val(&encoder_right_val, -5);
                }
                encoder_feedback(2);
            }
            break;

        // ---- MODO 2: VIRTUALDJ ----
        case 2:
            // No VirtualDJ, os encoders enviam MIDI CC ou atalhos de teclado.
            // VirtualDJ mapeia por padrão:
            //   Deck A Volume: não tem atalho padrão de teclado - usamos MIDI se disponível
            //   Por agora: usar as setas + modificadores para simular
            if (index == 0) {
                // Deck A: Volume (VirtualDJ: sem atalho padrão - atalho customizável)
                // Usar Page Up/Down como placeholder - mapeia no VirtualDJ
                if (clockwise) {
                    tap_code(KC_PGUP);
                    clamp_val(&encoder_left_val, 5);
                } else {
                    tap_code(KC_PGDN);
                    clamp_val(&encoder_left_val, -5);
                }
                encoder_feedback(0);
            } else if (index == 1) {
                // Master Volume: Volume do sistema
                if (clockwise) {
                    tap_code(KC_VOLU);
                    clamp_val(&encoder_center_val, 5);
                } else {
                    tap_code(KC_VOLD);
                    clamp_val(&encoder_center_val, -5);
                }
                encoder_feedback(1);
            } else if (index == 2) {
                // Deck B: Volume (usar Home/End como placeholder)
                if (clockwise) {
                    tap_code16(LSFT(KC_PGUP));
                    clamp_val(&encoder_right_val, 5);
                } else {
                    tap_code16(LSFT(KC_PGDN));
                    clamp_val(&encoder_right_val, -5);
                }
                encoder_feedback(2);
            }
            break;
    }
    return false;
}

// ============================================================
// NOTA: Para VirtualDJ com MIDI real, seria necessário:
// 1. Ativar MIDI_ENABLE = yes no rules.mk
// 2. Usar midi_send_cc() com os CC numbers corretos
// Exemplo Deck A Volume = CC 7, Channel 1
// Exemplo Deck B Volume = CC 7, Channel 2
// Isso requer configurar o VirtualDJ para receber MIDI do macropad
// ============================================================