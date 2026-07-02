#include <cstdio>

#include "script.hpp"

static const char* s_source = R"(
    import sym "test1.sym"
    import sym "test2.sym"

    pointer data 904030 Foo
    pointer func 804020 Bar
)";

static void print_patch(const Patch& patch)
{
    std::printf("  %.06X", patch.offset);
    for (uint8_t byte : patch.bytes)
        std::printf(" %.02X", byte);
    std::printf("\n");
}

int main()
{
    auto tokens = tokenize_script(s_source);
    auto patches = parse_script(tokens);

    std::printf("Generated patches:\n");
    for (const auto& patch : patches)
        print_patch(patch);
}

