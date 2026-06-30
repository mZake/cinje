#ifndef SCRIPT_HPP_
#define SCRIPT_HPP_

#include <string_view>
#include <string>
#include <vector>

#include <stddef.h>
#include <stdint.h>

enum class TokenType
{
    Import,
    Pointer,
    Wrapper,
    Hook,
    Identifier,
    String,
    Number,
    Newline,
    Eof
};

struct Token
{
    TokenType type;
    size_t line;
    size_t column;
    uint32_t number;
    std::string text;
};

struct Patch
{
    uint32_t offset;
    std::vector<uint8_t> bytes;
};

std::vector<Token> tokenize_script(std::string_view source);

std::vector<Patch> compile_script(std::string_view file_path);

#endif // SCRIPT_HPP_
