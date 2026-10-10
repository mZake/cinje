MEMORY
{
    ROM : ORIGIN = 0x8000000 + OFFSET_TO_INSERT, LENGTH = 32M
}

SECTIONS
{
    .text   : { *(.text) } >ROM
    .rodata : { *(.rodata) } >ROM
    .data   : { *(.data) } >ROM
    .bss    : { *(.bss) *(COMMON) } >ROM
    BLOB_END = .;
}
