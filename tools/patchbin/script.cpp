#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <fstream>
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
            break;

        if (current == '\n')
        {
            advance_line(state);
            continue;
        }

        if (current == '#')
        {
            while (current != '\0' && current != '\n')
                current = advance_column(state);

            advance_line(state);
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

std::vector<Patch> compile_script(std::string_view file_path)
{
    return {};
}

