#include "header/cpu/gdt.h"
#include "header/interrupt/interrupt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            .segment_low = 0,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0,
            .non_system = 0,
            .desc_priv = 0,
            .seg_present = 0,
            .seg_limit = 0,
            .l_bit = 0,
            .default_op = 0,
            .gran_bit = 0,
            .base_high = 0
        },
        {
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0xA,
            .non_system = 1,
            .desc_priv = 0,
            .seg_present = 1,
            .seg_limit = 0xF,
            .l_bit = 0,
            .default_op = 1,
            .gran_bit = 1,
            .base_high = 0
        },
        {
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0x2,
            .non_system = 1,
            .desc_priv = 0,
            .seg_present = 1,
            .seg_limit = 0xF,
            .l_bit = 0,
            .default_op = 1,
            .gran_bit = 1,
            .base_high = 0
        },
        {
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0xA,
            .non_system = 1,
            .desc_priv = 0x3,
            .seg_present = 1,
            .seg_limit = 0xF,
            .l_bit = 0,
            .default_op = 1,
            .gran_bit = 1,
            .base_high = 0
        },
        {
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0x2,
            .non_system = 1,
            .desc_priv = 0x3,
            .seg_present = 1,
            .seg_limit = 0xF,
            .l_bit = 0,
            .default_op = 1,
            .gran_bit = 1,
            .base_high = 0
        },
        {
            .seg_limit      = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
            .segment_low       = sizeof(struct TSSEntry),
            .base_high         = 0,
            .base_mid          = 0,
            .base_low          = 0,
            .non_system        = 0,    // S bit
            .type_bit          = 0x9,
            .desc_priv         = 0,    // DPL
            .seg_present       = 1,    // P bit
            .default_op        = 1,    // D/B bit
            .l_bit             = 0,    // L bit
            .gran_bit          = 0,    // G bit
        },
        {0}
    }
};

void gdt_install_tss(void) {
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low  = base & 0xFFFF;
}

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    // TODO : Implement, this GDTR will point to global_descriptor_table. 
    //        Use sizeof operator
    .size = sizeof(global_descriptor_table) - 1,
    .address = &global_descriptor_table
};
