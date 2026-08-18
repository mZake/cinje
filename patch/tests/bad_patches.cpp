#define PATCHBIN_IMPLEMENTATION
#include "patchbin.hpp"

void patchbin_main()
{
    PATCH_POINTER(0x0000100, "NotASymbol", false);
    PATCH_POINTER(0x2000000, "gMoveNames", false);

    PATCH_BYTES(0x2000000, {0x01, 0x02, 0x03, 0x04});

    PATCH_HOOK(0x0000100, "NotASymbol", 0);
    PATCH_HOOK(0x2000000, "CB2_ReturnToField", 0);
    PATCH_HOOK(0x0000100, "CB2_ReturnToField", 16);
}

