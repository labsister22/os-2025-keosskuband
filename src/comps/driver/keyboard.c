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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\n', 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
  
  if (scancode == KEYBOARD_SCANCODE_EXTENDED) {
      keyboard_state.extended_mode = true;
      pic_ack(IRQ_KEYBOARD);
      return;
  }
  
  if (scancode & 0x80) {
      handle_key_release(scancode & 0x7F);
      pic_ack(IRQ_KEYBOARD);
      return;
  }
  
  if (scancode == KEYBOARD_SCANCODE_LSHIFT || scancode == KEYBOARD_SCANCODE_RSHIFT) {
      keyboard_state.shift_pressed = true;
  }
  else if (scancode == KEYBOARD_SCANCODE_CAPS_LOCK) {
      keyboard_state.caps_lock_on = !keyboard_state.caps_lock_on;
  }
  else if (keyboard_state.keyboard_input_on) {
      char mapped_key;
      
      if (keyboard_state.extended_mode) {
          mapped_key = keyboard_scancode_extended_map[scancode];
      } 
      else {
          bool use_shift_mapping = keyboard_state.shift_pressed;
          
          if (keyboard_state.caps_lock_on) {
              char non_shifted = keyboard_scancode_1_to_ascii_map[scancode];
              if ((non_shifted >= 'a' && non_shifted <= 'z')) {
                  use_shift_mapping = use_shift_mapping != keyboard_state.caps_lock_on;
              }
          }
          
          if (use_shift_mapping) {
              mapped_key = keyboard_scancode_1_to_ascii_shifted_map[scancode];
          } else {
              mapped_key = keyboard_scancode_1_to_ascii_map[scancode];
          }
      }

      if (mapped_key != 0) {
          keyboard_state.keyboard_buffer = mapped_key;
      }
  }
  
  if (keyboard_state.extended_mode) {
      keyboard_state.extended_mode = false;
  }
  
  pic_ack(IRQ_KEYBOARD);
}