#ifndef SCRIPT_HPP_
#define SCRIPT_HPP_

#include <span>
#include <string_view>
#include <string>
#include <variant>
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

enum class ImportType
{
    None, Symbol
};

enum class PointerType
{
    None, Function, Data
};

struct Token
{
    TokenType type;
    size_t line;
    size_t column;
    uint32_t number;
    std::string text;
};

struct ImportNode
{
    ImportType type = ImportType::None;
    std::string filepath;
};

struct PointerNode
{
    PointerType type = PointerType::None;
    uint32_t offset = 0;
    std::string symbol;
};

struct WrapperNode
{
    uint32_t offset = 0;
    uint32_t argument_count = 0;
    bool returns = false;
    std::string symbol;
};

struct HookNode
{
    uint32_t offset = 0;
    uint32_t argument_count = 0;
    std::string symbol;
};

struct Patch
{
    uint32_t offset;
    std::vector<uint8_t> bytes;
};

using Node = std::variant<ImportNode, PointerNode, WrapperNode, HookNode>;

std::vector<Token> tokenize_script(std::string_view source);

std::vector<Node> parse_script(std::span<Token> tokens);

std::vector<Patch> compile_script(std::string_view file_path);

#endif // SCRIPT_HPP_
