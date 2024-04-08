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
    .press_ctrl = false,
    .last_non_space_col = {0}
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
    // variabel lokal untuk menyimpan karakter ascii yang akan diproses
    char ascii_char = 0;

    // jika keyboard_input_on bernilai true
    if (keyboard_state.keyboard_input_on){
        // memproses scancode yang diterima ke karakter ascii
        if (scancode == 0x2A || scancode == 0x36){ // scancode untuk shift ditekan
            keyboard_state.press_shift = true;
        }
        else if (scancode == 0xAA || scancode == 0x86){ // scancode saat shift dilepas
            keyboard_state.press_shift = false;
        }
        else if (scancode == 0x1C){ // enter
            // maju ke baris berikutnya
            keyboard_state.row++;
            keyboard_state.col = 0;
        }
        else if (scancode == 0x0E){ // backspace
            // Hapus karakter sebelumnya jika buffer tidak kosong
            if (keyboard_state.col > 0) {
                keyboard_state.col--;
                framebuffer_write(keyboard_state.row, keyboard_state.col, ' ', 0x07, 0x00);
            } else if (keyboard_state.row > 0) { // Jika posisi kolom adalah 0
                // Kembali ke baris sebelumnya dan ke kolom terakhir yang berisi karakter non-spasi
                keyboard_state.row--;
                keyboard_state.col = keyboard_state.last_non_space_col[keyboard_state.row] + 1;
            }
            // Update cursor position
            framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
        }
        else if (scancode == 0x0F){ // tab
            // maju ke kolom berikutnya yang merupakan kelipatan 4
            keyboard_state.col = (keyboard_state.col + 4) & ~3;
        }
        else if (keyboard_state.press_shift){ // jika shift ditekan
            ascii_char = keyboard_scancode_1_to_ascii_map_shift[scancode];
        }
        else{ // jika shift tidak ditekan
            ascii_char = keyboard_scancode_1_to_ascii_map[scancode];
        }

        // jika ascii_char bukan 0, berarti bukan tombol khusus yang sudah ditangani
        if (ascii_char != 0) {
            // menyimpan karakter ascii ke dalam framebuffer
            framebuffer_write(keyboard_state.row, keyboard_state.col, ascii_char, 0x07, 0x00);
            // Jika karakter bukan spasi, perbarui posisi kolom terakhir yang berisi karakter non-spasi
            if (ascii_char != ' ') {
                keyboard_state.last_non_space_col[keyboard_state.row] = keyboard_state.col;
            }
            // Maju ke kolom berikutnya
            keyboard_state.col++;
            // Update cursor position
            framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
        }
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