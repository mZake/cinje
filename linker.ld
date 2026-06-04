MEMORY
{
    ROM : ORIGIN = 0x8900000, LENGTH = 32M
}

SECTIONS
{
    .text   : { *(.text) } >ROM
    .rodata : { *(.rodata) } >ROM
    .data   : { *(.data) } >ROM
    .bss    : { *(.bss) *(COMMON) } >ROM
    _blob_end = .;
}
