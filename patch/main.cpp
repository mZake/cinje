#define PATCHBIN_IMPLEMENTATION
#include "patchbin.hpp"

void patchbin_main()
{
#if 1
    REPOINT("gMoveNames", 0x128, false);
    REPOINT("gItems", 0x132, true);
    REPOINT("CB2_ReturnToField", 0x800100, false);
    REPLACE(0x760, {0x00, 0x01, 0x02, 0x03, 0x04, 0x10, 0x12});
    REWRITE("CB2_ReturnToField", 0x900200, 0, false);
#else
    REPOINT("Invalid", 0x100, false);
    REPOINT("gMoveNames", 0x2000000, false);

    REPLACE(0x2000000, {0x01, 0x02, 0x03, 0x04});

    HOOK("Invalid", 0x100, 0);
    HOOK("CB2_ReturnToField", 0x2000000, 0);
    HOOK("CB2_ReturnToField", 0x100, 16);
#endif
}

