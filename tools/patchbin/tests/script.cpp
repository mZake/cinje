#include <cctype>
#include <cstdio>
#include <string_view>
#include <vector>

#include "common.hpp"
#include "script.hpp"

static std::string_view s_raw_source = R"(
import sym "blob.sym"

pointer data 100 Foo
pointer func 200 Bar

wrapper 300 2 0 Baz
hook 400 1 Fizz
)";

static std::string_view strip_source(std::string_view raw_src)
{
    auto cur_it = raw_src.begin();
    while (*cur_it && std::isspace(*cur_it))
        ++cur_it;

    auto first_it = cur_it;
    auto last_it = cur_it;

    while (*cur_it)
    {
        if (!std::isspace(*cur_it))
            last_it = cur_it;
        cur_it++;
    }

    return std::string_view(first_it, last_it + 1);
}

static const char* token_type_name(TokenType type)
{
    switch (type)
    {
        case TokenType::Import: return "Import";
        case TokenType::Pointer: return "Pointer";
        case TokenType::Wrapper: return "Wrapper";
        case TokenType::Hook: return "Hook";
        case TokenType::Identifier: return "Identifier";
        case TokenType::String: return "String";
        case TokenType::Number: return "Number";
        case TokenType::Newline: return "Newline";
        case TokenType::Eof: return "EOF";
        default: return "Unknown";
    }
}

static void print_node(const ImportNode& node)
{
    std::printf("Import: Type=%d, File=%s\n",
                static_cast<int>(node.type), node.filepath.c_str());
}

static void print_node(const PointerNode& node)
{
    std::printf("Pointer: Type=%d, Offset=%.06X, Symbol=%s\n",
                static_cast<int>(node.type), node.offset, node.symbol.c_str());
}

static void print_node(const WrapperNode& node)
{
    std::printf("Wrapper: Offset=%.06X, Argument-Count=%d, Returns=%b, Symbol=%s\n",
                node.offset, node.argument_count, node.returns, node.symbol.c_str());
}

static void print_node(const HookNode& node)
{
    std::printf("Hook: Offset=%.06X, Argument-Count=%d, Symbol=%s\n",
                node.offset, node.argument_count, node.symbol.c_str());
}

static void print_source(std::string_view src)
{
    std::printf("Source:\n");
    std::printf("--------------------------------\n");
    std::printf("%.*s\n", SV_ARG(src));
    std::printf("--------------------------------\n");
}

static void print_tokens(const std::vector<Token>& toks)
{
    std::printf("Tokens:\n");
    std::printf("--------------------------------\n");

    for (const auto& tok : toks)
    {
        if (tok.type == TokenType::Number)
            std::printf("%s(%X)", token_type_name(tok.type), tok.number);
        else
            std::printf("%s(%s)", token_type_name(tok.type), tok.text.c_str());

        if (tok.type == TokenType::Newline || tok.type == TokenType::Eof)
            std::putchar('\n');
        else
            std::putchar(' ');
    }

    std::printf("--------------------------------\n");
}

static void print_nodes(const std::vector<Node>& nodes)
{
    std::printf("Nodes:\n");
    std::printf("--------------------------------\n");

    for (const auto& node : nodes)
    {
        std::visit([](auto&& node) {
            print_node(node);
        }, node);
    }

    std::printf("--------------------------------\n");
}

int main()
{
    auto src = strip_source(s_raw_source);
    auto toks = tokenize_script(src);
    auto nodes = parse_script(toks);

    print_source(src);
    print_tokens(toks);
    print_nodes(nodes);
}

