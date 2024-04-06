#include "header/driver/keyboard.h"
#include "header/text/framebuffer.h"
#include "header/cpu/portio.h"
#include "header/interrupt/interrupt.h"
#include "header/stdlib/string.h"

// mendefinisikan state driver keyboard
static struct KeyboardDriverState keyboard_state = {
    .row = 0,
    .col = 0,
    .press_shift = false,
    .press_ctrl = false
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

const char keyboard_scancode_1_to_ascii_map_shift[256] = {
      0, 0x1B, '!', '@', '#', '$', '%', '^',  '&', '*', '(',  ')',  '_', '+', '\b', '\t',
    'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',  'O', 'P', '{',  '}', '\n',   0,  'A',  'S',
    'D',  'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0, '|',  'Z', 'X',  'C',  'V',
    'B',  'N', 'M', '<', '>', '?',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
};

void keyboard_isr(void){
    // membaca scancode dari port data keyboard
    uint8_t scancode = in(KEYBOARD_DATA_PORT);
    // TODO : Implement scancode processing

    //variabel lokal untuk melacak apakah tombol shift sedang ditekan atau tidak
    // static bool shift_pressed = false;

    // memeriksa apakah tombol shift ditekan atau tidak

    // jika keyboard_input_on bernilai true
    if (keyboard_state.keyboard_input_on){
        // memproses scancode yang diterima ke karakter ascii
        char ascii_char;
        if (scancode == 0x2A || scancode == 0x36){ // scancode untuk shift ditekan
            keyboard_state.press_shift = true;
        }
        else if (scancode == 0xAA || scancode == 0x86){ // scancode saat shift dilepas
            keyboard_state.press_shift = false;
        }
        else if (scancode == 0x1C){ // enter
            ascii_char = '\n';
        }
        else if (scancode == 0x0E){ // backspace
            ascii_char = '\b';
        }
        else if (scancode == 0x0F){ // tab
            ascii_char = '\t';
        }
        else if (keyboard_state.press_shift){ // jika shift ditekan
            ascii_char = keyboard_scancode_1_to_ascii_map_shift[scancode];
        }
        else{ // jika shift tidak ditekan
            ascii_char = keyboard_scancode_1_to_ascii_map[scancode];
        }

        // menyimpan karakter ascii ke dalam buffer keyboard
        keyboard_state.keyboard_buffer = ascii_char;
    } 
    // melakukan pic_ack() ke IRQ1
    pic_ack(1);
}

// mengaktifkan pembacaan input keyboard
void keyboard_state_activate(void){
    keyboard_state.keyboard_input_on = true;
}

// mematikan pembacaan input keyboard
void keyboard_state_deactivate(void){
    keyboard_state.keyboard_input_on = false;
}

// mengambil dan mengosongkan buffer keyboard
void get_keyboard_buffer(char *buf){
    //mengcopy isi buffer keyboard ke buf
    *buf = keyboard_state.keyboard_buffer;

    // mengosongkan buffer keyboard
    keyboard_state.keyboard_buffer = 0;
}