#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <span>
#include <string_view>
#include <string>
#include <unordered_map>

#include <stddef.h>
#include <stdint.h>

#include "common.hpp"
#include "script.hpp"

/* Available commands:
 *   import   <type> <file>
 *   pointer  <type> <offset> <symbol>
 *   wrapper  <offset> <symbol>
 *   hook     <offset> <symbol>
 */

enum class ImportType
{
    None, Symbol
};

enum class PointerType
{
    None, Function, Data
};

struct LexerState
{
    std::vector<Token> tokens;
    std::string_view source;
    size_t index;
    size_t line;
    size_t column;
};

struct ParserState
{
    std::vector<Patch> patches;
    std::unordered_map<std::string, uint32_t> symbols;
    std::span<Token> tokens;
    size_t index;
};

static std::unordered_map<std::string_view, TokenType> s_keyword_table =
{
    {"import",  TokenType::Import},
    {"pointer", TokenType::Pointer},
    {"wrapper", TokenType::Wrapper},
    {"hook",    TokenType::Hook},
};

// The behavior of all functions from <cctype> is undefined if the argument's value is
// neither representable as unsigned char nor equal to EOF. To use these functions safely
// with plain chars (or signed chars), the argument should first be converted to unsigned char.

static bool is_space_char(char c)
{
    auto uc = static_cast<unsigned char>(c);
    return std::isspace(uc) && uc != '\n';
}

static bool is_identifier_start(char c)
{
    auto uc = static_cast<unsigned char>(c);
    return std::isalpha(uc) || uc == '_';
}

static bool is_identifier_char(char c)
{
    auto uc = static_cast<unsigned char>(c);
    return std::isalnum(uc) || uc == '_';
}

static bool is_digit_char(char c)
{
    auto uc = static_cast<unsigned char>(c);
    return std::isxdigit(uc);
}

static std::string read_text_file(std::string_view file)
{
    std::ifstream stream{std::string(file)};
    if (!stream.is_open())
        log_fatal("cannot open %.*s for reading", SV_ARG(file));

    stream.seekg(0, std::ios::end);
    std::streampos size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    std::string buffer;
    buffer.resize(size);
    stream.read(buffer.data(), size);

    return buffer;
}

static std::string_view token_type_name(TokenType type)
{
    switch (type)
    {
        case TokenType::Import: return "import";
        case TokenType::Pointer: return "pointer";
        case TokenType::Wrapper: return "wrapper";
        case TokenType::Hook: return "hook";
        case TokenType::Identifier: return "identifier";
        case TokenType::String: return "string";
        case TokenType::Number: return "number";
        case TokenType::Newline: return "newline";
        case TokenType::Eof: return "end-of-file";
        default: return "UNKNOWN";
    }
}

static void add_token(LexerState& state, TokenType type)
{
    Token token = {};
    token.type = type;
    token.line = state.line;
    token.column = state.column;
    state.tokens.push_back(token);
}

static void add_token(LexerState& state, TokenType type, std::string_view text)
{
    Token token = {};
    token.type = type;
    token.line = state.line;
    token.column = state.column;
    token.text = std::string(text);
    state.tokens.push_back(token);
}

static void add_token(LexerState& state, TokenType type, uint32_t number)
{
    Token token = {};
    token.type = type;
    token.line = state.line;
    token.column = state.column;
    token.number = number;
    state.tokens.push_back(token);
}

static char advance_column(LexerState& state)
{
    ++state.column;
    return state.source[++state.index];
}

static char advance_line(LexerState& state)
{
    add_token(state, TokenType::Newline);
    ++state.line;
    state.column = 1;
    return state.source[++state.index];
}

static char current_char(const LexerState& state)
{
    return state.source[state.index];
}

static std::string_view extract_lexeme(const LexerState& state, size_t first)
{
    std::string_view source_sv = state.source;
    size_t count = state.index - first;
    return source_sv.substr(first, count);
}

std::vector<Token> tokenize_script(std::string_view source)
{
    LexerState state = {};
    state.source = source;
    state.line = 1;
    state.column = 1;

    while (true)
    {
        char current = current_char(state);
        while (is_space_char(current))
            current = advance_column(state);

        if (current == '\0')
        {
            advance_line(state);
            add_token(state, TokenType::Eof);
            break;
        }

        if (current == '\n')
        {
            advance_line(state);
            continue;
        }

        if (current == '#')
        {
            while (current != '\0' && current != '\n')
                current = advance_column(state);
            continue;
        }

        if (is_identifier_start(current))
        {
            size_t first = state.index;

            while (is_identifier_char(current))
                current = advance_column(state);

            std::string_view lexeme = extract_lexeme(state, first);

            auto it = s_keyword_table.find(lexeme);
            if (it != s_keyword_table.end())
                add_token(state, it->second);
            else
                add_token(state, TokenType::Identifier, lexeme);
        }
        else if (is_digit_char(current))
        {
            size_t first = state.index;

            while (is_digit_char(current))
                current = advance_column(state);

            std::string_view lexeme = extract_lexeme(state, first);

            uint32_t value = 0;
            auto result = std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), value, 16);
            if (result.ec == std::errc::result_out_of_range)
                std::exit(EXIT_FAILURE); // TODO: Logging

            add_token(state, TokenType::Number, value);
        }
        else if (current == '\"')
        {
            size_t first = state.index;

            current = advance_column(state);
            while (current != '\0' && current != '\n' && current != '\"')
                current = advance_column(state);

            if (current == '\0' || current == '\n')
                std::exit(EXIT_FAILURE); // TODO: Logging

            std::string_view lexeme = extract_lexeme(state, first + 1);

            add_token(state, TokenType::String, lexeme);
            advance_column(state);
        }
    }

    return state.tokens;
}

#include <sstream>

static std::unordered_map<std::string, uint32_t> read_symbol_table(std::string_view filepath)
{
    // Temporary implementation

    std::unordered_map<std::string, uint32_t> symbols;

    std::string source = read_text_file(filepath);
    std::istringstream stream{source};
    stream >> std::hex;

    while (stream)
    {
        uint32_t address = 0;
        char section = 0;
        std::string name;

        stream >> address >> section >> name;
        symbols[name] = address;
    }

    return symbols;
}

static Token shift_tokens(ParserState& state)
{
    return state.tokens[state.index++];
}

static void type_check(TokenType actual_type, TokenType expected_type)
{
    if (actual_type == expected_type)
        return;

    auto actual_name = token_type_name(actual_type);
    auto expected_name = token_type_name(expected_type);

    log_fatal("expected %.*s, got %.*s", SV_ARG(actual_name), SV_ARG(expected_name));
}

static ImportType parse_import_type(std::string_view text)
{
    if (text == "sym")
        return ImportType::Symbol;

    return ImportType::None;
}

static void parse_import(ParserState& state)
{
    Token type_token = shift_tokens(state);
    Token file_token = shift_tokens(state);

    type_check(type_token.type, TokenType::Identifier);
    type_check(file_token.type, TokenType::String);

    ImportType import_type = parse_import_type(type_token.text);
    if (import_type == ImportType::None)
        log_fatal("%.*s is not a valid import type", SV_ARG(type_token.text));

    if (import_type == ImportType::Symbol)
    {
        auto new_symbols = read_symbol_table(file_token.text);
        state.symbols.merge(new_symbols);
    }
}

static PointerType parse_pointer_type(std::string_view text)
{
    if (text == "data")
        return PointerType::Data;
    if (text == "func")
        return PointerType::Function;

    return PointerType::None;
}

#if defined(__linux__)
    #include <endian.h>
    #define HTOLE32(x) htole32(x)
#elif defined(_WIN32) // Windows is always little-endian
    #define HTOLE32(x) (x)
#else
    #error Unsupported platform
#endif

static void parse_pointer(ParserState& state)
{
    Token type_token = shift_tokens(state);
    Token offset_token = shift_tokens(state);
    Token symbol_token = shift_tokens(state);

    type_check(type_token.type, TokenType::Identifier);
    type_check(offset_token.type, TokenType::Number);
    type_check(symbol_token.type, TokenType::Identifier);

    PointerType pointer_type = parse_pointer_type(type_token.text);
    if (pointer_type == PointerType::None)
        log_fatal("%.*s is not a valid pointer type", SV_ARG(type_token.text));

    auto it = state.symbols.find(symbol_token.text);
    if (it == state.symbols.end())
        log_fatal("symbol %.*s not found", SV_ARG(symbol_token.text));

    uint32_t address = it->second;
    if (pointer_type == PointerType::Data)
        address = HTOLE32(it->second);
    else if (pointer_type == PointerType::Function)
        address = HTOLE32(it->second | 1);

    std::vector<uint8_t> bytes;
    bytes.resize(sizeof(uint32_t));
    std::memcpy(bytes.data(), &address, sizeof(uint32_t));

    Patch patch = {};
    patch.offset = offset_token.number;
    patch.bytes = std::move(bytes);
    state.patches.push_back(patch);
}

std::vector<Patch> parse_script(std::span<Token> tokens)
{
    ParserState state = {};
    state.tokens = tokens;

    while (true)
    {
        Token token = shift_tokens(state);
        if (token.type == TokenType::Eof)
            break;

        switch (token.type)
        {
            case TokenType::Newline:
                break;
            case TokenType::Import:
                parse_import(state);
                break;
            case TokenType::Pointer:
                parse_pointer(state);
                break;
            default:
                log_fatal("missing parser implementation");
                break;
        }
    }

    for (const auto& [name, address] : state.symbols)
        std::printf("%s = %.08X\n", name.c_str(), address);

    return state.patches;
}

std::vector<Patch> compile_script(std::string_view file_path)
{
    return {};
}

