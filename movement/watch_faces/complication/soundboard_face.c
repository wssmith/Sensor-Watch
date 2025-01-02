/*
 * MIT License
 *
 * Copyright (c) 2024 <#author_name#>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "movement.h"
#include "soundboard_tunes.h"
#include "soundboard_face.h"

typedef struct {
    char name[7];
	int8_t* notes;
} tune_entry;

static const uint8_t tune_count = 12;

static tune_entry tunes[] = {
    { " Ghost", friendly_ghost_tune },
    { "Airflo", airflow_tune },
    { " Fairy", zelda_fairy_tune },
    { "Thomas", thomas_theme_tune },
	{ "Secret", zelda_secret_tune },
    { " Mario", mario_theme_tune },
    { " MGS  ", mgs_codec_tune },
    { " Pssbl", kim_possible_tune },
    { "Ranger", power_rangers_tune },
    { " Layla", layla_tune },
    { "Harry ", harry_potter_tune },
    { "Harry2", harry_potter_long_tune }
};

static const char* face_name = "SB";
static const WatchIndicatorSegment tune_playing_indicator = WATCH_INDICATOR_SIGNAL;

static volatile bool tune_playing = false;
static volatile bool tune_looping = false;
static tune_entry* last_tune_played = NULL;

static void soundboard_face_update_lcd(soundboard_state_t* state);
static void play_tune(soundboard_state_t* state);
static void stop_tune(void);
static void on_tune_end(void);
static tune_entry* get_selected_tune(uint8_t tune_index);

void soundboard_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(soundboard_state_t));
        memset(*context_ptr, 0, sizeof(soundboard_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void soundboard_face_activate(movement_settings_t *settings, void *context) {
    soundboard_state_t *state = (soundboard_state_t *)context;
    (void)settings;

    // Handle any tasks related to your watch face coming on screen.
    stop_tune();
	state->tune_index = 0;
    tune_looping = false;
}

bool soundboard_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    soundboard_state_t *state = (soundboard_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE: {
            soundboard_face_update_lcd(state);
            break;
        }
            
        case EVENT_TICK:
            // If needed, update your display here.
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            // Suppress light
            break;

        case EVENT_LIGHT_BUTTON_UP: {
            ++state->tune_index;
            if (state->tune_index == tune_count) {
                state->tune_index = 0;
            }
            soundboard_face_update_lcd(state);
            break;
        }
            
        case EVENT_ALARM_BUTTON_UP: {
            if (!tune_playing) {
                play_tune(state);
            }
            else {
                stop_tune();
            }
            soundboard_face_update_lcd(state);
            break;
        }

        case EVENT_ALARM_LONG_PRESS: {
			tune_looping = !tune_looping;
            soundboard_face_update_lcd(state);
            break;
        }
           
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            // movement_move_to_face(0);
            break;

        case EVENT_LOW_ENERGY_UPDATE:
            // If you did not resign in EVENT_TIMEOUT, you can use this event to update the display once a minute.
            // Avoid displaying fast-updating values like seconds, since the display won't update again for 60 seconds.
            // You should also consider starting the tick animation, to show the wearer that this is sleep mode:
            // watch_start_tick_animation(500);
            break;

        default:
            // Movement's default loop handler will step in for any cases you don't handle above:
            // * EVENT_LIGHT_BUTTON_DOWN lights the LED
            // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
            // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the secondary watch face, if configured)
            // You can override any of these behaviors by adding a case for these events to this switch statement.
            return movement_default_loop_handler(event, settings);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    // Exceptions:
    //  * If you are displaying a color using the low-level watch_set_led_color function, you should return false.
    //  * If you are sounding the buzzer using the low-level watch_set_buzzer_on function, you should return false.
    // Note that if you are driving the LED or buzzer using Movement functions like movement_illuminate_led or
    // movement_play_alarm, you can still return true. This guidance only applies to the low-level watch_ functions.
    return true;
}

void soundboard_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
    stop_tune();
}

static void soundboard_face_update_lcd(soundboard_state_t* state) {
    tune_entry* entry = get_selected_tune(state->tune_index);
    
    char buf[11];
    sprintf(buf, "%s %c%s", face_name, tune_looping ? 'L' : ' ', entry->name);
    watch_display_string(buf, 0);

	if (tune_playing) {
		watch_set_indicator(tune_playing_indicator);
	} else {
		watch_clear_indicator(tune_playing_indicator);
	}
}

static void play_tune(soundboard_state_t* state) {
    tune_playing = true;
    tune_entry* entry = get_selected_tune(state->tune_index);
    last_tune_played = entry;
    movement_play_tune(entry->notes, on_tune_end);
}

static void stop_tune() {
    watch_buzzer_abort_sequence();
    tune_playing = false;
}

static void on_tune_end() {
    if (tune_looping) {
        movement_play_tune(last_tune_played->notes, on_tune_end);
    } else {
        if (watch_is_buzzer_or_led_enabled()) {
            movement_end_buzzing();
        } else {
            movement_end_buzzing_and_disable_buzzer();
        }

        tune_playing = false;
        watch_clear_indicator(tune_playing_indicator);
    }
}

static tune_entry* get_selected_tune(uint8_t tune_index) {
    return &tunes[tune_index];
}
