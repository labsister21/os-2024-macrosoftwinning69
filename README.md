# Tugas Besar Sistem Operasi - IF0x8B6 - 2024
> Milestone 0: Pembuatan Sistem Operasi x86 Toolchain, Kernel, GDT

> Milestone 1: Pembuatan Sistem Operasi x86 Interrupt, Driver, File System

> Milestone 2: Pembuatan Sistem Operasi x86 Paging, User Mode, Shell

> Milestone 3: Pembuatan Sistem Operasi x86 Process, Scheduler, Multitasking

## Anggota Kelompok
<table>
    <tr>
        <td colspan="3", align = "center"><center>Nama Kelompok: macrosoftwinning69</center></td>
    </tr>
    <tr>
        <td>No.</td>
        <td>Nama</td>
        <td>NIM</td>
    </tr>
    <tr>
        <td>1.</td>
        <td>Aland Mulia Pratama</td>
        <td>13522124</td>
    </tr>
    <tr>
        <td>2.</td>
        <td>Rizqika Mulia Pratama</td>
        <td>13522126</td>
    </tr>
    <tr>
        <td>3.</td>
        <td>Ikhwan Al Hakim</td>
        <td>13522147</td>
    </tr>
        <tr>
        <td>4.</td>
        <td>Muhammad Rasheed Qais Tandjung</td>
        <td>13522158</td>
    </tr>
</table>

## Table of Contents
* [Deskripsi Singkat](#deskripsi-singkat)
* [Struktur File](#struktur-file)
* [Requirements](#requirements)
* [Cara Menjalankan Program](#cara-menjalankan-program)
* [Acknowledgements](#acknowledgements)

## Deskripsi Singkat
Dalam tugas ini kami membuat sebuah program Sistem Operasi. Sistem Operasi yang dibuat akan berjalan pada arsitektur x86 32 bit dan dijalankan dengan emulator Qemu:

Milestone **_0x0_**:
1. Menyiapkan repository & tools 
    - Github classroom & repository
    - Pemasangan toolchain & vscode

2. Kernel dasar
    - Kode untuk kernel dasar
    - Linker script untuk build

3. Otomatisasi build
    - Otomatisasi kompilasi kernel
    Otomatisasi pembuatan disk image

4. Menjalankan sistem operasi
    - Menjalankan OS dengan QEMU

5. Pembuatan struktur data GDT
    - Struct GDT & Segment Descriptor

6. Load GDT
    - Assembly untuk load GDT


Milestone **_0x1_**:
1. Text Framebuffer
    - Membuat Fungsi Menulis pada Layar
    - Membuat Membersihkan Layar (clear)
    - Membuat Fungsi Menggerakan Kursor

2. Interrupt
    - Membuat IDT
    - Melakukan PIC Remapping
    - Membuat interrupt Service Routine
    - Memuat IDT pada CPU

3. Keyboard Driver
    - Menyalakan IRQ Keyboard
    - Membuat driver untuk menerima dan menerjemahkan input keyboard

4. Filesystem
    - Menambahkan hard drive pada virtual machine
    - Membuat driver untuk I/O pada hard drive
    - Membuat struktur data untuk file system FAT32
    - Membuat operasi write pada filesystem
    - Membuat operasi read pada filesystem
    - Membuat operasi delete pada filesystem

Milestone **_0x2_**:
1. Manajemen memory
    - Membuat struktur data paging
    - Memuat kernel pada alamat memori tinggi
    - Mengaktifkan paging

2. Separasi kernel-user space
    - Membuat inserter file ke file system
    - Membuat GDT entry untuk user dan Task State Segment
    - Membuat alokator memori sederhana
    - Membuat aplikasi user sederhana
    - Memasuki user mode pada aplikasi sederhana

3. Shell
    - Membuat system call
    - Mengembangkan aplikasi sederhana menjadi shell
    - Membuat perintah - perintah pada shell


Milestone **_0x3_**:

1. Menyiapkan struktur untuk proses
    - Membuat struct Process Control Block (PCB)
    - Membuat struktur data yang dibutuhkan proses

2. Membuat task scheduler & context switch
    - Membuat algoritma penjadwalan task yang ada
    - Membuat mekanisme perpindahan konteks

3. Membuat perintah shell
    - Membuat perintah shell tambahan untuk manajemen process
        - exec
        - ps
        - kill

4. Menjalankan Multitasking
    - Menjalankan program yang dapat menggambarkan konkurensi telah berjalan

## Struktur File
```bash

 ```

## Requirements
- Netwide Assembler (NASM): Kompiler untuk bahasa assembly x86 yang mendukung penulisan instruksi langsung. (https://www.nasm.us/)
- GNU C Compiler (GCC): Kompiler bahasa C yang digunakan untuk mengkompilasi kode pada berbagai sistem operasi. (https://man7.org/linux/man-pages/man1/gcc.1.html)
- GNU Linker (ld): Linker yang menggabungkan kode objek hasil kompilasi menjadi satu file eksekusi. (https://linux.die.net/man/1/ld)
- QEMU - System i386: Emulator dan virtual machine yang digunakan untuk menjalankan sistem operasi. (https://www.qemu.org/docs/master/system/target-i386.html)
- GNU Make: Alat untuk otomatisasi proses build dalam pengembangan perangkat lunak. (https://www.gnu.org/software/make/)
- genisoimage: Alat untuk membuat image disk dari sistem operasi. (https://linux.die.net/man/1/genisoimage)
- GDB (GNU Debugger): Debugger untuk melakukan debugging dinamis pada kernel. (https://man7.org/linux/man-pages/man1/gdb.1.html)

## Cara Menjalankan Program
 1. Clone repository Github [ini](repository-link).
 2. Install semua requirements yang diperlukan dengan mengeksekusi `sudo apt update` dan `sudo apt install -y nasm gcc qemu-system-x86 make genisoimage gdb` pada command prompt wsl.
 4. Jalankan program dengan mengeksekusi `make run` di terminal wsl pada root directory repository ini.


## Acknowledgements


<div align="center">
<img src="OS-S-nya-Stresss.gif" width="400" alt="OS (S nya Stress)">

<p>Cheers dulu kali udah namatin Tugas Operating Stress.</p>
<div>

<div align="center">
<img src="lolichan_anime_1.jpg" width="300" alt="OS (S nya Stress)">
<img src="lolichan_anime_2.jpg" width="300" alt="OS (S nya Stress)">
<img src="lolichan_anime_3.jpg" width="300" alt="OS (S nya Stress)">

<p>Biar seneng ditemenin dulu sama lolichan🌼.</p>
<div>
