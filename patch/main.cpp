#define PATCHBIN_IMPLEMENTATION
#include "patchbin.hpp"

void patchbin_main()
{
#if 1
    PATCH_POINTER(0x128, "gMoveNames", false);
    PATCH_POINTER(0x132, "gItems", true);
    PATCH_POINTER(0x800100, "CB2_ReturnToField", false);
    PATCH_BYTES(0x760, {0x00, 0x01, 0x02, 0x03, 0x04, 0x10, 0x12});
    PATCH_FUNC(0x900200, "CB2_ReturnToField", 0, false);
#else
    PATCH_POINTER("Invalid", 0x100, false);
    PATCH_POINTER("gMoveNames", 0x2000000, false);

    PATCH_BYTES(0x2000000, {0x01, 0x02, 0x03, 0x04});

    HOOK("Invalid", 0x100, 0);
    HOOK("CB2_ReturnToField", 0x2000000, 0);
    HOOK("CB2_ReturnToField", 0x100, 16);
#endif
}

