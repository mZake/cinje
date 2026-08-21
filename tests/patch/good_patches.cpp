#define PATCHBIN_IMPLEMENTATION
#include "patchbin.hpp"

void patchbin_main()
{
    PATCH_POINTER(0x000100, "gMoveNames", false);
    PATCH_POINTER(0x000200, "gItems", true);
    PATCH_POINTER(0x800100, "CB2_ReturnToField", false);

    PATCH_BYTES(0x760, {0x00, 0x01, 0x02, 0x03, 0x04, 0x10, 0x12});

    PATCH_FUNC(0x900200, "CB2_ReturnToField", 0, false);
}

