#include "header/driver/keyboard.h"
#include "header/cpu/portio.h"
#include "header/stdlib/string.h"
#include "header/interrupt/interrupt.h"

#define KEYBOARD_SCANCODE_LSHIFT        0x2A
#define KEYBOARD_SCANCODE_RSHIFT        0x36
#define KEYBOARD_SCANCODE_CAPS_LOCK     0x3A
#define KEYBOARD_SCANCODE_EXTENDED      0xE0

static struct KeyboardDriverState keyboard_state = {
    .keyboard_input_on = false,
    .keyboard_buffer = 0,
    .shift_pressed = false,
    .caps_lock_on = false,
    .extended_mode = false
};

const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

const char keyboard_scancode_1_to_ascii_shifted_map[256] = {
      0, 0x1B, '!', '@', '#', '$', '%', '^',  '&', '*', '(',  ')',  '_', '+', '\b', '\t',
    'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',  'O', 'P', '{',  '}', '\n',   0,  'A',  'S',
    'D',  'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',   0,  '|',  'Z', 'X',  'C',  'V',
    'B',  'N', 'M', '<', '>', '?',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '_',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

const char keyboard_scancode_extended_map[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0x00-0x0F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\n', 0, 0, 0,  // 0x10-0x1F (0x1C = Enter)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0x20-0x2F
    0, 0, 0, 0, 0, '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 0x30-0x3F (0x35 = numpad /)
    0, 0, 0, 0, 0, 0, 0, 0, 0x10, 0, 0, 0x11, 0, 0x12, 0, 0, // 0x40-0x4F (0x48=UP=0x10, 0x4B=LEFT=0x11, 0x4D=RIGHT=0x12)
    0x13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x50-0x5F (0x50=DOWN=0x13)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0x60-0x6F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0x70-0x7F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0x80-0x8F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0x90-0x9F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0xA0-0xAF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0xB0-0xBF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0xC0-0xCF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0xD0-0xDF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0xE0-0xEF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0      // 0xF0-0xFF
};

void keyboard_state_activate(void) {
    if (!keyboard_state.keyboard_input_on) {
        keyboard_state.keyboard_input_on = true;
    }
}

void keyboard_state_deactivate(void) {
    if (keyboard_state.keyboard_input_on) {
        keyboard_state.keyboard_input_on = false;
    }
}

void get_keyboard_buffer(char *buf) {
    *buf = keyboard_state.keyboard_buffer;
    keyboard_state.keyboard_buffer = 0x0;
}

void handle_key_release(uint8_t scancode) {
    if (scancode == KEYBOARD_SCANCODE_LSHIFT || scancode == KEYBOARD_SCANCODE_RSHIFT) {
        keyboard_state.shift_pressed = false;
    }

    if (keyboard_state.extended_mode) {
        keyboard_state.extended_mode = false;
    }
}

void keyboard_isr(void) {
    uint8_t scancode = in(KEYBOARD_DATA_PORT);
    
    // Handle extended scancode prefix
    if (scancode == KEYBOARD_SCANCODE_EXTENDED) {
        keyboard_state.extended_mode = true;
        pic_ack(IRQ_KEYBOARD);
        return;
    }
    
    // Handle key release (MSB set)
    if (scancode & 0x80) {
        handle_key_release(scancode & 0x7F);
        pic_ack(IRQ_KEYBOARD);
        return;
    }
    
    // Handle shift keys
    if (scancode == KEYBOARD_SCANCODE_LSHIFT || scancode == KEYBOARD_SCANCODE_RSHIFT) {
        keyboard_state.shift_pressed = true;
    }
    // Handle caps lock
    else if (scancode == KEYBOARD_SCANCODE_CAPS_LOCK) {
        keyboard_state.caps_lock_on = !keyboard_state.caps_lock_on;
    }
    // Handle normal keys if keyboard input is active
    else if (keyboard_state.keyboard_input_on) {
        char mapped_key;
        
        if (keyboard_state.extended_mode) {
            // Use extended scancode mapping for arrow keys
            mapped_key = keyboard_scancode_extended_map[scancode];
        } 
        else {
            // Determine which mapping to use based on shift/caps lock state
            bool use_shift_mapping = keyboard_state.shift_pressed;
            
            // Handle caps lock for alphabetic characters
            if (keyboard_state.caps_lock_on) {
                char non_shifted = keyboard_scancode_1_to_ascii_map[scancode];
                if ((non_shifted >= 'a' && non_shifted <= 'z')) {
                    // For letters, caps lock inverts the shift state
                    use_shift_mapping = !use_shift_mapping;
                }
            }
            
            // Select appropriate mapping
            if (use_shift_mapping) {
                mapped_key = keyboard_scancode_1_to_ascii_shifted_map[scancode];
            } else {
                mapped_key = keyboard_scancode_1_to_ascii_map[scancode];
            }
        }

        // Store the mapped character if it's valid (non-zero)
        if (mapped_key != 0) {
            keyboard_state.keyboard_buffer = mapped_key;
        }
    }
    
    // Reset extended mode after processing
    if (keyboard_state.extended_mode) {
        keyboard_state.extended_mode = false;
    }
    
    pic_ack(IRQ_KEYBOARD);
}