#include <cstdio>

#include "script.hpp"

static const char* s_source = R"(
    # Import symbols from blob.sym
    import sym "blob.sym"

    # Create pointers to Foo and Bar
    pointer data 800400 Foo
    pointer func 800500 Bar

    # Create function wrapper
    wrapper 900000 Foo

    # Create hook
    hook 900200 Bar
)";

static const char* token_name(TokenType type)
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

int main()
{
    auto tokens = tokenize_script(s_source);
    for (const auto& token : tokens)
    {
        if (token.type == TokenType::Number)
            std::printf("%s(%X)", token_name(token.type), token.number);
        else
            std::printf("%s(%s)", token_name(token.type), token.text.c_str());

        if (token.type == TokenType::Newline)
            std::putchar('\n');
        else
            std::putchar(' ');
    }
}

