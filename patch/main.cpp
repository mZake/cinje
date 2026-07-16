#include "patchbin.hpp"

int main(int argc, char** argv)
{
    REPOINT("gMoveNames", 0x128, false);
    REPOINT("gItems", 0x132, true);
    REPOINT("CB2_ReturnToField", 0x800100, false);
    HOOK("CB2_ReturnToField", 0x900200, 0);

    REPLACE(0x760, {0x00, 0x01, 0x02, 0x03, 0x04, 0x10, 0x12});

    patch(argc, argv);
}

