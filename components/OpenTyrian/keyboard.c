/* 
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "joystick.h"
#include "keyboard.h"
#include "network.h"
#include "opentyr.h"
#include "video.h"
#include "video_scale.h"

#include "SDL3/SDL.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL_internal.h"
#include "events/SDL_keyboard_c.h"
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "usb/usb_host.h"
#include "errno.h"
#include "driver/gpio.h"

#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/hid_usage_mouse.h"

#include <pthread.h>
#include <semaphore.h>

sem_t usb_task_semaphore;
pthread_t usb_event_thread;  // Thread for handling HID events

static const char *TAG = "keyboard";

JE_boolean ESCPressed;

JE_boolean newkey, newmouse, keydown, mousedown;
SDL_Keycode lastkey_sym;
SDL_Keymod lastkey_mod;
unsigned char lastkey_char;
Uint8 lastmouse_but;
Uint16 lastmouse_x, lastmouse_y;
JE_boolean mouse_pressed[3] = {false, false, false};
Uint16 mouse_x, mouse_y;

Uint8 keysactive[SDL_SCANCODE_COUNT];

#ifdef NDEBUG
bool input_grab_enabled = true;
#else
bool input_grab_enabled = false;
#endif

QueueHandle_t app_event_queue = NULL;

#define APP_QUIT_PIN                GPIO_NUM_0

void flush_events_buffer( void )
{
}

void wait_input( JE_boolean keyboard, JE_boolean mouse, JE_boolean joystick )
{
}

void wait_noinput( JE_boolean keyboard, JE_boolean mouse, JE_boolean joystick )
{
}

typedef enum {
    APP_EVENT = 0,
    APP_EVENT_HID_HOST
} app_event_group_t;

typedef struct {
    app_event_group_t event_group;
    /* HID Host - Device related info */
    struct {
        hid_host_device_handle_t handle;
        hid_host_driver_event_t event;
        void *arg;
    } hid_host_device;
} app_event_queue_t;

static const char *hid_proto_name_str[] = {
    "NONE",
    "KEYBOARD",
    "MOUSE"
};

typedef struct {
    enum key_state {
        KEY_STATE_PRESSED = 0x00,
        KEY_STATE_RELEASED = 0x01
    } state;
    uint8_t modifier;
    uint8_t key_code;
} key_event_t;

app_event_queue_t evt_queue;

static void hid_print_new_device_report_header(hid_protocol_t proto)
{
    static hid_protocol_t prev_proto_output = -1;

    if (prev_proto_output != proto) {
        prev_proto_output = proto;
        printf("\r\n");
        if (proto == HID_PROTOCOL_MOUSE) {
            printf("Mouse\r\n");
        } else if (proto == HID_PROTOCOL_KEYBOARD) {
            printf("Keyboard\r\n");
        } else {
            printf("Generic\r\n");
        }
        fflush(stdout);
    }
}

static inline bool hid_keyboard_is_modifier_shift(uint8_t modifier)
{
    if (((modifier & HID_LEFT_SHIFT) == HID_LEFT_SHIFT) ||
            ((modifier & HID_RIGHT_SHIFT) == HID_RIGHT_SHIFT)) {
        return true;
    }
    return false;
}

#include "SDL_keyboard.h"

SDL_Scancode convert_hid_to_sdl_scancode(uint8_t hid_code) {
    switch (hid_code) {
        // Alphabet (A-Z)
        case HID_KEY_A: return SDL_SCANCODE_A;
        case HID_KEY_B: return SDL_SCANCODE_B;
        case HID_KEY_C: return SDL_SCANCODE_C;
        case HID_KEY_D: return SDL_SCANCODE_D;
        case HID_KEY_E: return SDL_SCANCODE_E;
        case HID_KEY_F: return SDL_SCANCODE_F;
        case HID_KEY_G: return SDL_SCANCODE_G;
        case HID_KEY_H: return SDL_SCANCODE_H;
        case HID_KEY_I: return SDL_SCANCODE_I;
        case HID_KEY_J: return SDL_SCANCODE_J;
        case HID_KEY_K: return SDL_SCANCODE_K;
        case HID_KEY_L: return SDL_SCANCODE_L;
        case HID_KEY_M: return SDL_SCANCODE_M;
        case HID_KEY_N: return SDL_SCANCODE_N;
        case HID_KEY_O: return SDL_SCANCODE_O;
        case HID_KEY_P: return SDL_SCANCODE_P;
        case HID_KEY_Q: return SDL_SCANCODE_Q;
        case HID_KEY_R: return SDL_SCANCODE_R;
        case HID_KEY_S: return SDL_SCANCODE_S;
        case HID_KEY_T: return SDL_SCANCODE_T;
        case HID_KEY_U: return SDL_SCANCODE_U;
        case HID_KEY_V: return SDL_SCANCODE_V;
        case HID_KEY_W: return SDL_SCANCODE_W;
        case HID_KEY_X: return SDL_SCANCODE_X;
        case HID_KEY_Y: return SDL_SCANCODE_Y;
        case HID_KEY_Z: return SDL_SCANCODE_Z;

        // Numbers (0-9)
        case HID_KEY_1: return SDL_SCANCODE_1;
        case HID_KEY_2: return SDL_SCANCODE_2;
        case HID_KEY_3: return SDL_SCANCODE_3;
        case HID_KEY_4: return SDL_SCANCODE_4;
        case HID_KEY_5: return SDL_SCANCODE_5;
        case HID_KEY_6: return SDL_SCANCODE_6;
        case HID_KEY_7: return SDL_SCANCODE_7;
        case HID_KEY_8: return SDL_SCANCODE_8;
        case HID_KEY_9: return SDL_SCANCODE_9;
        case HID_KEY_0: return SDL_SCANCODE_0;

        // Special keys
        case HID_KEY_ENTER: return SDL_SCANCODE_RETURN;  // Enter key
        case HID_KEY_ESC: return SDL_SCANCODE_ESCAPE;    // Escape key
        case HID_KEY_SPACE: return SDL_SCANCODE_SPACE;   // Space key
        case HID_KEY_KEYPAD_BACKSPACE: return SDL_SCANCODE_BACKSPACE;
        case HID_KEY_TAB: return SDL_SCANCODE_TAB;
        case HID_KEY_MINUS: return SDL_SCANCODE_MINUS;
        case HID_KEY_EQUAL: return SDL_SCANCODE_EQUALS;
        case HID_KEY_KEYPAD_OPEN_BRACE: return SDL_SCANCODE_LEFTBRACKET;
        case HID_KEY_KEYPAD_CLOSE_BRACE: return SDL_SCANCODE_RIGHTBRACKET;
        case HID_KEY_BACK_SLASH: return SDL_SCANCODE_BACKSLASH;
        case HID_KEY_KEYPAD_COMMA: return SDL_SCANCODE_COMMA;
        case HID_KEY_SLASH: return SDL_SCANCODE_SLASH;

        // Arrow keys
        case HID_KEY_UP: return SDL_SCANCODE_UP;
        case HID_KEY_DOWN: return SDL_SCANCODE_DOWN;
        case HID_KEY_LEFT: return SDL_SCANCODE_LEFT;
        case HID_KEY_RIGHT: return SDL_SCANCODE_RIGHT;

        // Function keys (F1-F12)
        case HID_KEY_F1: return SDL_SCANCODE_F1;
        case HID_KEY_F2: return SDL_SCANCODE_F2;
        case HID_KEY_F3: return SDL_SCANCODE_F3;
        case HID_KEY_F4: return SDL_SCANCODE_F4;
        case HID_KEY_F5: return SDL_SCANCODE_F5;
        case HID_KEY_F6: return SDL_SCANCODE_F6;
        case HID_KEY_F7: return SDL_SCANCODE_F7;
        case HID_KEY_F8: return SDL_SCANCODE_F8;
        case HID_KEY_F9: return SDL_SCANCODE_F9;
        case HID_KEY_F10: return SDL_SCANCODE_F10;
        case HID_KEY_F11: return SDL_SCANCODE_F11;
        case HID_KEY_F12: return SDL_SCANCODE_F12;

        default: return SDL_SCANCODE_UNKNOWN;
    }
}


char convert_sdl_scancode_to_ascii(SDL_Scancode scancode, bool shift_pressed) {
    switch (scancode) {
        // Alphabet (A-Z)
        case SDL_SCANCODE_A: return shift_pressed ? 'A' : 'a';
        case SDL_SCANCODE_B: return shift_pressed ? 'B' : 'b';
        case SDL_SCANCODE_C: return shift_pressed ? 'C' : 'c';
        case SDL_SCANCODE_D: return shift_pressed ? 'D' : 'd';
        case SDL_SCANCODE_E: return shift_pressed ? 'E' : 'e';
        case SDL_SCANCODE_F: return shift_pressed ? 'F' : 'f';
        case SDL_SCANCODE_G: return shift_pressed ? 'G' : 'g';
        case SDL_SCANCODE_H: return shift_pressed ? 'H' : 'h';
        case SDL_SCANCODE_I: return shift_pressed ? 'I' : 'i';
        case SDL_SCANCODE_J: return shift_pressed ? 'J' : 'j';
        case SDL_SCANCODE_K: return shift_pressed ? 'K' : 'k';
        case SDL_SCANCODE_L: return shift_pressed ? 'L' : 'l';
        case SDL_SCANCODE_M: return shift_pressed ? 'M' : 'm';
        case SDL_SCANCODE_N: return shift_pressed ? 'N' : 'n';
        case SDL_SCANCODE_O: return shift_pressed ? 'O' : 'o';
        case SDL_SCANCODE_P: return shift_pressed ? 'P' : 'p';
        case SDL_SCANCODE_Q: return shift_pressed ? 'Q' : 'q';
        case SDL_SCANCODE_R: return shift_pressed ? 'R' : 'r';
        case SDL_SCANCODE_S: return shift_pressed ? 'S' : 's';
        case SDL_SCANCODE_T: return shift_pressed ? 'T' : 't';
        case SDL_SCANCODE_U: return shift_pressed ? 'U' : 'u';
        case SDL_SCANCODE_V: return shift_pressed ? 'V' : 'v';
        case SDL_SCANCODE_W: return shift_pressed ? 'W' : 'w';
        case SDL_SCANCODE_X: return shift_pressed ? 'X' : 'x';
        case SDL_SCANCODE_Y: return shift_pressed ? 'Y' : 'y';
        case SDL_SCANCODE_Z: return shift_pressed ? 'Z' : 'z';

        // Numbers (0-9)
        case SDL_SCANCODE_1: return shift_pressed ? '!' : '1';
        case SDL_SCANCODE_2: return shift_pressed ? '@' : '2';
        case SDL_SCANCODE_3: return shift_pressed ? '#' : '3';
        case SDL_SCANCODE_4: return shift_pressed ? '$' : '4';
        case SDL_SCANCODE_5: return shift_pressed ? '%' : '5';
        case SDL_SCANCODE_6: return shift_pressed ? '^' : '6';
        case SDL_SCANCODE_7: return shift_pressed ? '&' : '7';
        case SDL_SCANCODE_8: return shift_pressed ? '*' : '8';
        case SDL_SCANCODE_9: return shift_pressed ? '(' : '9';
        case SDL_SCANCODE_0: return shift_pressed ? ')' : '0';

        // Special keys (these don't have ASCII equivalents, but return placeholders)
        case SDL_SCANCODE_RETURN: return '\n';
        case SDL_SCANCODE_ESCAPE: return 27;  // ASCII ESC
        case SDL_SCANCODE_SPACE: return ' ';
        case SDL_SCANCODE_BACKSPACE: return '\b';
        case SDL_SCANCODE_TAB: return '\t';
        case SDL_SCANCODE_MINUS: return shift_pressed ? '_' : '-';
        case SDL_SCANCODE_EQUALS: return shift_pressed ? '+' : '=';
        case SDL_SCANCODE_LEFTBRACKET: return shift_pressed ? '{' : '[';
        case SDL_SCANCODE_RIGHTBRACKET: return shift_pressed ? '}' : ']';
        case SDL_SCANCODE_BACKSLASH: return shift_pressed ? '|' : '\\';
        case SDL_SCANCODE_COMMA: return shift_pressed ? '<' : ',';
        case SDL_SCANCODE_SLASH: return shift_pressed ? '?' : '/';

        // Arrow keys (returning non-ASCII codes)
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_RIGHT:
            return 0;

        // Function keys (returning non-ASCII codes)
        case SDL_SCANCODE_F1:
        case SDL_SCANCODE_F2:
        case SDL_SCANCODE_F3:
        case SDL_SCANCODE_F4:
        case SDL_SCANCODE_F5:
        case SDL_SCANCODE_F6:
        case SDL_SCANCODE_F7:
        case SDL_SCANCODE_F8:
        case SDL_SCANCODE_F9:
        case SDL_SCANCODE_F10:
        case SDL_SCANCODE_F11:
        case SDL_SCANCODE_F12:
            return 0;

        default: return 0;
    }
}

#include "../src/events/SDL_keyboard_c.h"

static void key_event_callback(key_event_t *key_event)
{
    unsigned char key_char;
    SDL_Scancode scancode;

    hid_print_new_device_report_header(HID_PROTOCOL_KEYBOARD);

    // Get the keyboard ID (assuming there's only one for simplicity)
    int num_keyboards;
    SDL_KeyboardID *keyboard_ids = SDL_GetKeyboards(&num_keyboards);

    if (num_keyboards == 0) {
        printf("No SDL_keyboard registered, please add at least virtual keayboad by SDL_AddKeyboard\n");
        return;
    }

    SDL_KeyboardID keyboardID = keyboard_ids[0];  // Use the first available keyboard ID

    if (KEY_STATE_PRESSED == key_event->state) {
        // Get ASCII character from keycode2ascii array
        // uint8_t mod = (hid_keyboard_is_modifier_shift(key_event->modifier)) ? 1 : 0;
        // if (key_event->key_code < sizeof(keycode2ascii) / sizeof(keycode2ascii[0])) {
        //     key_char = keycode2ascii[key_event->key_code][mod];
        //     if (key_char) {
        //         printf("%c\n", key_char);  // Print to console for testing
        //     }
        // }

        // Get corresponding SDL scancode
        scancode = convert_hid_to_sdl_scancode(key_event->key_code);
        if (scancode != SDL_SCANCODE_UNKNOWN) {
            // Send key press event to SDL
            SDL_SendKeyboardKey(SDL_GetTicks(), keyboardID, key_event->key_code, scancode, 1);
        }
    }

    if (KEY_STATE_RELEASED == key_event->state) {
        scancode = convert_hid_to_sdl_scancode(key_event->key_code);
        if (scancode != SDL_SCANCODE_UNKNOWN) {
            // Send key release event to SDL
            SDL_SendKeyboardKey(SDL_GetTicks(), keyboardID, key_event->key_code, scancode, 0);
        }
    }

}

static inline bool key_found(const uint8_t *const src,
                             uint8_t key,
                             unsigned int length)
{
    for (unsigned int i = 0; i < length; i++) {
        if (src[i] == key) {
            return true;
        }
    }
    return false;
}

static void hid_host_keyboard_report_callback(const uint8_t *const data, const int length)
{
    hid_keyboard_input_report_boot_t *kb_report = (hid_keyboard_input_report_boot_t *)data;

    if (length < sizeof(hid_keyboard_input_report_boot_t)) {
        return;
    }

    static uint8_t prev_keys[HID_KEYBOARD_KEY_MAX] = { 0 };
    key_event_t key_event;

    for (int i = 0; i < HID_KEYBOARD_KEY_MAX; i++) {

        // key has been released verification
        if (prev_keys[i] > HID_KEY_ERROR_UNDEFINED &&
                !key_found(kb_report->key, prev_keys[i], HID_KEYBOARD_KEY_MAX)) {
            key_event.key_code = prev_keys[i];
            key_event.modifier = 0;
            key_event.state = KEY_STATE_RELEASED;
            key_event_callback(&key_event);
        }

        // key has been pressed verification
        if (kb_report->key[i] > HID_KEY_ERROR_UNDEFINED &&
                !key_found(prev_keys, kb_report->key[i], HID_KEYBOARD_KEY_MAX)) {
            key_event.key_code = kb_report->key[i];
            key_event.modifier = kb_report->modifier.val;
            key_event.state = KEY_STATE_PRESSED;
            key_event_callback(&key_event);
        }
    }

    memcpy(prev_keys, &kb_report->key, HID_KEYBOARD_KEY_MAX);
}


static void hid_host_mouse_report_callback(const uint8_t *const data, const int length)
{
    hid_mouse_input_report_boot_t *mouse_report = (hid_mouse_input_report_boot_t *)data;

    if (length < sizeof(hid_mouse_input_report_boot_t)) {
        return;
    }

    static int x_pos = 0;
    static int y_pos = 0;

    // Calculate absolute position from displacement
    x_pos += mouse_report->x_displacement;
    y_pos += mouse_report->y_displacement;

    hid_print_new_device_report_header(HID_PROTOCOL_MOUSE);

    printf("X: %06d\tY: %06d\t|%c|%c|\r",
           x_pos, y_pos,
           (mouse_report->buttons.button1 ? 'o' : ' '),
           (mouse_report->buttons.button2 ? 'o' : ' '));
    fflush(stdout);
}

static void hid_host_generic_report_callback(const uint8_t *const data, const int length)
{
	ESP_LOGI(TAG, "HID Host Generic Report Callback");
    hid_print_new_device_report_header(HID_PROTOCOL_NONE);
    for (int i = 0; i < length; i++) {
        printf("%02X", data[i]);
    }
    putchar('\r');
}


void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                                 const hid_host_interface_event_t event,
                                 void *arg)
{
    uint8_t data[64] = { 0 };
    size_t data_length = 0;
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
        ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(hid_device_handle,
                                                                  data,
                                                                  64,
                                                                  &data_length));

        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                hid_host_keyboard_report_callback(data, data_length);
            } else if (HID_PROTOCOL_MOUSE == dev_params.proto) {
                hid_host_mouse_report_callback(data, data_length);
            }
        } else {
            hid_host_generic_report_callback(data, data_length);
        }

        break;
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HID Device, protocol '%s' DISCONNECTED",
                 hid_proto_name_str[dev_params.proto]);
        ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGI(TAG, "HID Device, protocol '%s' TRANSFER_ERROR",
                 hid_proto_name_str[dev_params.proto]);
        break;
	default:
        ESP_LOGE(TAG, "HID Device, protocol '%s' Unhandled event",
                 hid_proto_name_str[dev_params.proto]);
        break;
    }
}

void hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                           const hid_host_driver_event_t event,
                           void *arg)
{
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_DRIVER_EVENT_CONNECTED:
        ESP_LOGI(TAG, "HID Device, protocol '%s' CONNECTED",
                 hid_proto_name_str[dev_params.proto]);

        const hid_host_device_config_t dev_config = {
            .callback = hid_host_interface_callback,
            .callback_arg = NULL
        };

        ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            ESP_ERROR_CHECK(hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT));
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                ESP_ERROR_CHECK(hid_class_request_set_idle(hid_device_handle, 0, 0));
            }
        }
        ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));
        break;
    default:
        break;
    }
}

void* usb_lib_thread(void *arg)
{
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));

    // Notify the main thread that USB host initialization is complete
    sem_post(&usb_task_semaphore);

    ESP_LOGI(TAG, "USB main loop");
    while (true) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        // In this example, there is only one client registered
        // So, once we deregister the client, this call must succeed with ESP_OK
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_ERROR_CHECK(usb_host_device_free_all());
            break;
        }
    }

    ESP_LOGI(TAG, "USB shutdown");
    // Clean up USB Host
    usleep(10 * 1000);  // Short delay to allow clients clean-up
    ESP_ERROR_CHECK(usb_host_uninstall());

    pthread_exit(NULL);  // Terminate the thread
}

void process_keyboard()
{
	if (xQueueReceive(app_event_queue, &evt_queue, portMAX_DELAY)) {
            if (APP_EVENT == evt_queue.event_group) {
                // User pressed button
                usb_host_lib_info_t lib_info;
                ESP_ERROR_CHECK(usb_host_lib_info(&lib_info));
                if (lib_info.num_devices == 0) {
                    // End while cycle
                    return;
                } else {
                    ESP_LOGW(TAG, "To shutdown example, remove all USB devices and press button again.");
                    // Keep polling
                }
            }

            if (APP_EVENT_HID_HOST ==  evt_queue.event_group) {
                hid_host_device_event(evt_queue.hid_host_device.handle,
                                      evt_queue.hid_host_device.event,
                                      evt_queue.hid_host_device.arg);
            }
        }
}

static void gpio_isr_cb(void *arg)
{
    BaseType_t xTaskWoken = pdFALSE;
    const app_event_queue_t evt_queue = {
        .event_group = APP_EVENT,
    };

    if (app_event_queue) {
        xQueueSendFromISR(app_event_queue, &evt_queue, &xTaskWoken);
    }

    if (xTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}


void hid_host_device_callback(hid_host_device_handle_t hid_device_handle,
                              const hid_host_driver_event_t event,
                              void *arg)
{
    const app_event_queue_t evt_queue = {
        .event_group = APP_EVENT_HID_HOST,
        // HID Host Device related info
        .hid_host_device.handle = hid_device_handle,
        .hid_host_device.event = event,
        .hid_host_device.arg = arg
    };

    if (app_event_queue) {
        xQueueSend(app_event_queue, &evt_queue, 0);
    }
}

void* usb_event_handler_thread(void* arg)
{
    ESP_LOGI(TAG, "USB HID event handler started");

    while (true) {
        // Handle USB HID events
        esp_err_t ret = hid_host_handle_events(portMAX_DELAY);  // Wait indefinitely for events
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error handling HID events: %d", ret);
            break;
        }
    }

    ESP_LOGI(TAG, "USB HID event handler shutting down");
    pthread_exit(NULL);
}

void init_keyboard(void)
{
    SDL_AddKeyboard(1, "Virtual Keyboard", true);

    newkey = newmouse = false;
    keydown = mousedown = false;

    ESP_LOGI(TAG, "HID Keyboard");

    // Init BOOT button: Pressing the button simulates app request to exit
    const gpio_config_t input_pin = {
        .pin_bit_mask = BIT64(APP_QUIT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&input_pin));
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1));
    ESP_ERROR_CHECK(gpio_isr_handler_add(APP_QUIT_PIN, gpio_isr_cb, NULL));

    // Initialize semaphore for synchronization
    sem_init(&usb_task_semaphore, 0, 0);

    // Create usb_lib_task thread
    pthread_t usb_thread;
    pthread_attr_t usb_thread_attr;
    pthread_attr_init(&usb_thread_attr);
    pthread_attr_setstacksize(&usb_thread_attr, 8912);

    int ret = pthread_create(&usb_thread, &usb_thread_attr, usb_lib_thread, NULL);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to create USB thread: %d", ret);
        return;
    }

    // Wait for notification from usb_lib_thread to proceed (using semaphore)
    sem_wait(&usb_task_semaphore);

    // HID host driver configuration
    const hid_host_driver_config_t hid_host_driver_config = {
        .create_background_task = false,  // No background task, handled manually
        .task_priority = 5,               // Priority doesn't apply to pthreads
        .stack_size = 8912,               // Stack size for thread (set manually if needed)
        .core_id = 0,
        .callback = hid_host_device_callback,
        .callback_arg = NULL
    };

    ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

    // Create queue (keeping xQueue for event handling if necessary)
    app_event_queue = xQueueCreate(10, sizeof(app_event_queue_t));

    ESP_LOGI(TAG, "Waiting for HID Device to be connected");

    // Start the HID event handler thread
    ret = pthread_create(&usb_event_thread, NULL, usb_event_handler_thread, NULL);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to create HID event handler thread: %d", ret);
        return;
    }
    pthread_detach(usb_event_thread);
}

void input_grab( bool enable )
{

}

JE_word JE_mousePosition( JE_word *mouseX, JE_word *mouseY )
{
	*mouseX = mouse_x;
	*mouseY = mouse_y;
	return mousedown ? lastmouse_but : 0;
}

void set_mouse_position( int x, int y )
{
	if (input_grab_enabled)
	{
		mouse_x = x;
		mouse_y = y;
	}
}

void service_SDL_events(JE_boolean clear_new)
{
    // Clear flags if requested
    if (clear_new) {
        newkey = newmouse = false;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // Keyboard events
            case SDL_EVENT_KEY_DOWN: {
                lastkey_sym = event.key.scancode;   // Record the last key symbol
                lastkey_mod = SDL_GetModState();   // Record the last key modifiers
                lastkey_char = convert_sdl_scancode_to_ascii(lastkey_sym, lastkey_mod & SDL_KMOD_SHIFT);
                printf("Key: %i, Scan code: %i\n", lastkey_char, event.key.scancode);

                keydown = true;    // Key is pressed
                newkey = true;     // Mark new key event
                keysactive[event.key.key] = true;  // Update key state
                break;
            }
            case SDL_EVENT_KEY_UP: {
                lastkey_sym = event.key.scancode;   // Record the last key symbol
                lastkey_mod = SDL_GetModState();   // Record the last key modifiers
                lastkey_char = convert_sdl_scancode_to_ascii(lastkey_sym, lastkey_mod & SDL_KMOD_SHIFT);
                printf("Key: %i, Scan code: %i\n", lastkey_char, event.key.scancode);

                keydown = false;   // Key is released
                newkey = false;     // Mark new key event
                keysactive[event.key.key] = false;  // Update key state
                break;
            }
            case SDL_EVENT_QUIT:
                // Handle quit event (if needed)
                break;

            default:
                break;
        }
    }
}


void JE_clearKeyboard( void )
{
	// /!\ Doesn't seems important. I think. D:
}

