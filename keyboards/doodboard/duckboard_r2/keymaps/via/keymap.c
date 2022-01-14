/* Copyright 2020-2021 doodboard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H

#define _LAYER0 0
#define _LAYER1 1
#define _LAYER2 2

volatile uint8_t led_numlock = false;
volatile uint8_t led_capslock = false;
volatile uint8_t led_scrolllock = false;

volatile uint8_t last_caps;
volatile uint8_t last_num;
volatile uint8_t last_scr;

volatile bool enc_controller = true;

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
                 KC_ESC,   KC_PSLS, KC_PAST, KC_PMNS,
                 KC_7,    KC_8,    KC_9,    KC_PPLS,
                 KC_4,    KC_5,    KC_6,    KC_BSPC,
        KC_MUTE, KC_1,    KC_2,    KC_3,    KC_ENT,
        MO(1), KC_F14,    KC_0,    KC_DOT,  KC_ENT),

    [1] = LAYOUT(
                 KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
                 RGB_HUI, RGB_SAI, RGB_VAI, RGB_MOD,
                 RGB_HUD, RGB_SAD, RGB_VAD, RGB_RMOD,
        RESET,   RGB_SPI,  RGB_SPD, KC_TRNS, KC_TRNS,
        KC_TRNS,   TG(2), KC_TRNS,  KC_TRNS,  RGB_TOG),

    [2] = LAYOUT(
                 KC_TRNS, KC_F3, KC_F8, KC_F9,
                 KC_F1, KC_F2, KC_TRNS, MACRO00,
                 KC_A, KC_D, KC_F, KC_TRNS,
        RESET,   KC_Z, KC_X, KC_C, KC_TRNS,
        TG(2),   KC_TRNS,   KC_TRNS, KC_TRNS, KC_TRNS),

    [3] = LAYOUT(
                 KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
                 KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
                 KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS),
};

bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index == 0 && enc_controller) { /* First encoder */
        if (clockwise) {
            tap_code(KC_VOLU);
        } else {
            tap_code(KC_VOLD);
        }
    }
    enc_controller = !enc_controller;
    return true;
}


#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) { return OLED_ROTATION_270; }

static uint16_t oled_timer;

// WPM-responsive animation stuff here
#define IDLE_FRAMES 2
#define IDLE_SPEED 40 // below this wpm value your animation will idle

#define ANIM_FRAME_DURATION 200 // how long each frame lasts in ms
// #define SLEEP_TIMER 60000 // should sleep after this period of 0 wpm, needs fixing
#define ANIM_SIZE 636 // number of bytes in array, minimize for adequate firmware size, max is 1024
#define TIMEOUT_OLED 5000

uint32_t anim_timer = 0;
uint32_t anim_sleep = 0;
uint8_t current_idle_frame = 0;

// Credit to u/Pop-X- for the initial code. You can find his commit here https://github.com/qmk/qmk_firmware/pull/9264/files#diff-303f6e3a7a5ee54be0a9a13630842956R196-R333.
static void render_anim(void) {
    static const char PROGMEM idle[IDLE_FRAMES][ANIM_SIZE] = {
        {0,  0,192,192,192,192,192,192,192,248,248, 30, 30,254,254,248,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  3,  3,  3,  3,255,255,255,255,255,255,255,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127,127,255,255,255,255,255,159,159,135,135,129,129,129, 97, 97, 25, 25,  7,  7,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1, 97, 97,127,  1,  1, 97, 97,127,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0},
        {0,  0,128,128,128,128,128,128,128,240,240, 60, 60,252,252,240,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  7,  7,  7,  7,  7,255,255,254,254,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255,255,255,255,255, 63, 63, 15, 15,  3,  3,  3,195,195, 51, 51, 15, 15,  3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  3, 99, 99,127,  3,  3, 99, 99,127,  3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0}
    };

    //assumes 1 frame prep stage
    void animation_phase(void) {
            current_idle_frame = (current_idle_frame + 1) % IDLE_FRAMES;
            oled_write_raw_P(idle[abs((IDLE_FRAMES-1)-current_idle_frame)], ANIM_SIZE);
    }

    if(timer_elapsed32(anim_timer) > ANIM_FRAME_DURATION) {
        anim_timer = timer_read32();
        animation_phase();
    }
}

void read_host_led_state(void) {
    uint8_t leds = host_keyboard_leds();
    if (leds & (1 << USB_LED_NUM_LOCK)) {
        led_numlock = led_numlock ? led_numlock : !led_numlock;
    } else {
        led_numlock = led_numlock ? !led_numlock : led_numlock;
    }

    if (leds & (1 << USB_LED_CAPS_LOCK)) {
        led_capslock = led_capslock ? led_capslock : !led_capslock;
    } else {
        led_capslock = led_capslock ? !led_capslock : led_capslock;
    }

    if (leds & (1 << USB_LED_SCROLL_LOCK)) {
        led_scrolllock = led_scrolllock ? led_scrolllock : !led_scrolllock;
    } else {
        led_scrolllock = led_scrolllock ? !led_scrolllock : led_scrolllock;
    }
}

bool oled_task_user(void) {
    read_host_led_state();
    if (last_caps != led_capslock) {
        last_caps = led_capslock;
        oled_timer = timer_read();
    }
    if (last_num != led_numlock) {
        last_num = led_numlock;
        oled_timer = timer_read();
    }
    if (last_scr != led_scrolllock) {
        last_scr = led_scrolllock;
        oled_timer = timer_read();
    }

	if (timer_elapsed(oled_timer) > TIMEOUT_OLED) {
        oled_off();
    } else {
        oled_on();
    render_anim();
    

    oled_set_cursor(0,6);
    // oled_write_P(PSTR("DUCK\nBOARD\n"), false);
    // oled_write_P(PSTR("-----\n"), false);
    // // Host Keyboard Layer Status

    oled_write_P(PSTR("LAYER\n"), false);

    
    switch (get_highest_layer(layer_state)) {
        case 0:
            oled_write_P(PSTR("  0\n\n"), false);
            break;
        case 1:
            oled_write_P(PSTR("  1\n\n"), false);
            break;
        case 2:
            oled_write_P(PSTR(" osu\n\n"), false);
            break;
    }

    if (led_capslock) {
        oled_write_P(PSTR("CAP <\n"), false);
    } else {
        oled_write_P(PSTR("CAP  \n"), false);
    }
    if (led_scrolllock) {
        oled_write_P(PSTR("SCR <\n"), false);
    } else {
        oled_write_P(PSTR("SCR  \n"), false);
    }
    if (led_numlock) {
        oled_write_P(PSTR("NUM <\n"), false);
    } else {
        oled_write_P(PSTR("NUM  \n"), false);
    }
    }
    return false;
}
#endif

void keyboard_post_init_user(void) {
  //Customise these values to debug
  debug_enable=true;
  debug_matrix=true;
  //debug_keyboard=true;
  //debug_mouse=true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        oled_timer = timer_read();
    }
    return true;
}