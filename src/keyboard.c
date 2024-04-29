#include "header/driver/keyboard.h"
#include "header/text/framebuffer.h"
#include "header/cpu/portio.h"
#include "header/interrupt/interrupt.h"
#include "header/stdlib/string.h"

// mendefinisikan state driver keyboard
struct KeyboardDriverState keyboard_state = {
    .row = 0,
    .col = 0,
    .press_shift = false,
    .press_ctrl = false,
    .last_non_space_col = {0},
    .keyboard_buffer_ext = EXT_BUFFER_NONE,

    .up_limit = 0,
    .down_limit = FRAMEBUFFER_HEIGHT - 1,
    .left_limit = 0,
    .right_limit = FRAMEBUFFER_WIDTH - 1
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

    // jika keyboard_input_on bernilai true
    if (keyboard_state.keyboard_input_on){

        // Process extended scancodes
        if (scancode == EXTENDED_SCANCODE_BYTE) {
            keyboard_state.read_extended_mode = true;
            return;
        } else if (keyboard_state.read_extended_mode) {
            if (scancode == EXT_SCANCODE_UP) keyboard_state.keyboard_buffer_ext = EXT_BUFFER_UP;
            else if (scancode == EXT_SCANCODE_DOWN) keyboard_state.keyboard_buffer_ext = EXT_BUFFER_DOWN;
            else if (scancode == EXT_SCANCODE_LEFT) keyboard_state.keyboard_buffer_ext = EXT_BUFFER_LEFT;
            else if (scancode == EXT_SCANCODE_RIGHT) keyboard_state.keyboard_buffer_ext = EXT_BUFFER_RIGHT;
            else keyboard_state.keyboard_buffer_ext = EXT_BUFFER_NONE;
        }

        // memproses scancode yang diterima ke karakter ascii
        if (scancode == 0x2A || scancode == 0x36){ // scancode untuk shift ditekan
            keyboard_state.press_shift = true;
        }
        else if (scancode == 0xAA || scancode == 0xB6){ // scancode saat shift dilepas
            keyboard_state.press_shift = false;
        }
        else if (scancode == 0x1D) { // ctrl
            keyboard_state.press_ctrl = true;
        }
        else if (scancode == 0x9D) { // ctrl
            keyboard_state.press_ctrl = false;
        }
        else if (scancode == 0x1C){ // enter
            keyboard_state.keyboard_buffer = '\n';
        }
        else if (scancode == 0x0E){ // backspace
            keyboard_state.keyboard_buffer = '\b';
        }
        else if (scancode == 0x0F){ // tab
            keyboard_state.keyboard_buffer = '\t';
        }
        else {
            if (keyboard_state.press_shift){ // jika shift ditekan
                keyboard_state.keyboard_buffer = keyboard_scancode_1_to_ascii_map_shift[scancode];
            }
            else{ // jika shift tidak ditekan
                keyboard_state.keyboard_buffer = keyboard_scancode_1_to_ascii_map[scancode];
            }
        }
    } 
    // melakukan pic_ack() ke IRQ1
    pic_ack(1);
}

// void keyboard_isr(void){
//     // membaca scancode dari port data keyboard
//     uint8_t scancode = in(KEYBOARD_DATA_PORT);
//     // variabel lokal untuk menyimpan karakter ascii yang akan diproses
//     char ascii_char = 0;

//     // jika keyboard_input_on bernilai true
//     if (keyboard_state.keyboard_input_on){
//         // memproses scancode yang diterima ke karakter ascii
//         if (scancode == 0x2A || scancode == 0x36){ // scancode untuk shift ditekan
//             keyboard_state.press_shift = true;
//         }
//         else if (scancode == 0xAA || scancode == 0xB6){ // scancode saat shift dilepas
//             keyboard_state.press_shift = false;
//         }
//         else if (scancode == 0x1D) { // ctrl
//             keyboard_state.press_ctrl = true;
//         }
//         else if (scancode == 0x9D) { // ctrl
//             keyboard_state.press_ctrl = false;
//         }
//         else if (scancode == 0x1C){ // enter
//             // maju ke baris berikutnya
//             keyboard_state.row++;
//             keyboard_state.col = 0;
            
//             // update posisi cursor
//             framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
//         }
//         else if (scancode == 0x0E){ // backspace
//             // hapus karakter sebelumnya jika buffer tidak kosong
//             if (keyboard_state.col > 0) {
//                 keyboard_state.col--;
//                 framebuffer_write(keyboard_state.row, keyboard_state.col, ' ', 0x07, 0x00);
//             } else if (keyboard_state.row > 0) { // jika posisi kolom adalah 0
//                 // kmbali ke baris sebelumnya dan ke kolom terakhir yang berisi karakter non-spasi
//                 keyboard_state.row--;
//                 keyboard_state.col = keyboard_state.last_non_space_col[keyboard_state.row] + 1;
//             }
//             // update posisi cursor
//             framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
//         }
//         else if (scancode == 0x0F){ // tab
//             // maju ke kolom berikutnya yang merupakan kelipatan 4
//             keyboard_state.col = (keyboard_state.col + 4) & ~3;

//             // update posisi cursor
//             framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
//         }
//         else if (keyboard_state.press_shift){ // jika shift ditekan
//             ascii_char = keyboard_scancode_1_to_ascii_map_shift[scancode];
//         }
//         else{ // jika shift tidak ditekan
//             ascii_char = keyboard_scancode_1_to_ascii_map[scancode];
//         }

//         // jika ascii_char bukan 0, berarti bukan tombol khusus yang sudah ditangani
//         if (ascii_char != 0) {
//             // menyimpan karakter ascii ke dalam framebuffer
//             framebuffer_write(keyboard_state.row, keyboard_state.col, ascii_char, 0x07, 0x00);
//             // jika karakter bukan spasi, perbarui posisi kolom terakhir yang berisi karakter non-spasi
//             if (ascii_char != ' ') {
//                 keyboard_state.last_non_space_col[keyboard_state.row] = keyboard_state.col;
//             }
//             // maju ke kolom berikutnya
//             keyboard_state.col++;
//             // update posisi kursor
//             framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
//         }
//     } 
//     // melakukan pic_ack() ke IRQ1
//     pic_ack(1);
// }

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

void get_keyboard_buffer_ext(char *buf){
    //mengcopy isi buffer keyboard ke buf
    *buf = keyboard_state.keyboard_buffer_ext;

    // mengosongkan buffer keyboard
    keyboard_state.keyboard_buffer_ext = EXT_BUFFER_NONE;
}