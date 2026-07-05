#include <cstdio>

#include "script.hpp"

static const char* s_source = R"(
    import sym "test1.sym"
    import sym "test2.sym"

    pointer data 904030 Foo
    pointer func 804020 Bar
)";

static void print_node(const ImportNode& node)
{
    std::printf("Import:\n");
    std::printf("- Type: %d\n", (int)node.type);
    std::printf("- File: %s\n", node.filepath.c_str());
}

static void print_node(const PointerNode& node)
{
    std::printf("Pointer:\n");
    std::printf("- Type: %d\n", (int)node.type);
    std::printf("- Offset: %.06X\n", node.offset);
    std::printf("- Symbol: %s\n", node.symbol.c_str());
}

static void print_node(const WrapperNode& node)
{
    std::printf("Wrapper:\n");
    std::printf("- Offset: %.06X\n", node.offset);
    std::printf("- Argument Count: %d\n", node.argument_count);
    std::printf("- Returns: %b\n", node.returns);
    std::printf("- Symbol: %s\n", node.symbol.c_str());
}

static void print_node(const HookNode& node)
{
    std::printf("Hook:\n");
    std::printf("- Offset: %.06X\n", node.offset);
    std::printf("- Argument Count: %d\n", node.argument_count);
    std::printf("- Symbol: %s\n", node.symbol.c_str());
}

int main()
{
    auto tokens = tokenize_script(s_source);
    auto nodes = parse_script(tokens);

    for (const auto& node : nodes)
        std::visit([](auto&& node) { print_node(node); }, node);
}

