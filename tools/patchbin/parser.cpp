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
#include "parser.hpp"

/* Available commands:
 *   import   <type> <file>
 *   pointer  <type> <offset> <symbol>
 *   wrapper  <offset> <symbol>
 *   hook     <offset> <symbol>
 */

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

struct LexerState
{
    std::vector<Token> tokens;
    std::string file_path;
    std::string source;
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

static std::vector<Token> tokenize(std::string_view source, std::string_view file_path)
{
    LexerState state = {};
    state.file_path = std::string(file_path);
    state.source = std::string(source);
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

/*
static std::vector<Token> tokenize(std::string_view source, std::string_view file)
{
    std::vector<Token> tokens;

    auto current_it = source.begin();
    auto eof_it = source.end();
    size_t line = 1;

    while (true)
    {
         while (current_it != eof_it && is_space_char(*current_it))
            ++current_it;

         if (current_it == eof_it)
             break;

        if (*current_it == '\n')
        {
            auto token = make_token(TokenType::Newline, line);
            tokens.push_back(token);
            ++current_it;
            ++line;
            continue;
        }

        if (*current_it == '#')
        {
            while (current_it != eof_it && *current_it != '\n')
                ++current_it;
            continue;
        }

        if (is_identifier_start(*current_it))
        {
            auto first_it = current_it;
            while (current_it != eof_it && is_identifier_char(*current_it))
                ++current_it;

            std::string_view text_value{first_it, current_it};

            auto table_it = s_keyword_table.find(text_value);
            if (table_it != s_keyword_table.end())
            {
                auto token = make_token(table_it->second, line);
                tokens.push_back(token);
            }
            else
            {
                auto token = make_token(TokenType::Identifier, line, text_value);
                tokens.push_back(token);
            }
        }
        else if (is_xdigit_char(*current_it))
        {
            auto first_it = current_it;
            while (current_it != eof_it && is_xdigit_char(*current_it))
                ++current_it;

            std::string_view text_value{first_it, current_it};
            uint32_t int_value = 0;

            auto fc_result = std::from_chars(text_value.data(), text_value.data() + text_value.size(), int_value, 16);
            if (fc_result.ec == std::errc::result_out_of_range)
                source_error({file, line}, "{} cannot be stored in a 32-bit integer", text_value);

            auto token = make_token(TokenType::Number, line, int_value);
            tokens.push_back(token);
        }
        else if (*current_it == '\"')
        {
            auto first_it = ++current_it;
            if (current_it == eof_it)
                source_error({file, line}, "unterminated string");

            while (*current_it != '\"')
            {
                if (current_it == eof_it || *current_it == '\n')
                    source_error({file, line}, "unterminated string");
                ++current_it;
            }

            std::string_view text_value{first_it, current_it++};
            auto token = make_token(TokenType::String, line, text_value);
            tokens.push_back(token);
        }
    }

    Token eof_token = make_token(TokenType::EndOfFile, line);
    tokens.push_back(eof_token);

    return tokens;
}

static std::vector<Patch> parse(std::span<Token> tokens, std::string_view file)
{
    PatchJobs jobs;

    size_t token_index = 0;
    auto next_token = [&]() -> Token
    {
        return tokens[token_index++];
    };

    auto next_argument = [&](std::string_view name, TokenType type) -> Token
    {
        auto token = next_token();
        if (token.type != type)
            source_error({file, token.line}, "expected {} for <{}>, got {}",
                         token_type_name(type), name, token_type_name(token.type));
        return token;
    };

    while (true)
    {
        auto cmd_token = next_token();
        while (cmd_token.type == TokenType::Newline)
            cmd_token = next_token();

        if (cmd_token.type == TokenType::EndOfFile)
            break;

        if (cmd_token.type == TokenType::Data)
        {
            auto off_arg = next_argument("offset", TokenType::Number);
            auto sym_arg = next_argument("symbol", TokenType::Identifier);
            jobs.pointers.emplace_back(off_arg.int_value, sym_arg.text_value);
        }
        else if (cmd_token.type == TokenType::Func)
        {
            auto off_arg = next_argument("offset", TokenType::Number);
            auto sym_arg = next_argument("symbol", TokenType::Identifier);
            jobs.pointers.emplace_back(off_arg.int_value | 1, sym_arg.text_value);
        }
        else if (cmd_token.type == TokenType::Hook)
        {
            auto off_arg = next_argument("offset", TokenType::Number);
            auto reg_arg = next_argument("register", TokenType::Number);
            auto sym_arg = next_argument("symbol", TokenType::Identifier);
            jobs.hooks.emplace_back(off_arg.int_value, reg_arg.int_value, sym_arg.text_value);
        }
        else if (cmd_token.type == TokenType::Rewrite)
        {
            auto off_arg = next_argument("offset", TokenType::Number);
            auto num_arg = next_argument("num-args", TokenType::Number);
            auto ret_arg = next_argument("returns", TokenType::Number);
            auto sym_arg = next_argument("symbol", TokenType::Identifier);
            jobs.rewrites.emplace_back(off_arg.int_value, num_arg.int_value, ret_arg.int_value, sym_arg.text_value);
        }
        else
        {
            source_error({file, cmd_token.line}, "setence does not start with an command");
        }
    }

    return jobs;
}
*/

static const char* token_name(const Token& token)
{
    switch (token.type)
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

PatchCommands load_patch_file(std::string_view file_path)
{
    std::string source = read_text_file(file_path);

    std::vector<Token> tokens = tokenize(source, file_path);
    for (const auto& token : tokens)
    {
        if (token.type == TokenType::Number)
            std::printf("%s(%X)", token_name(token), token.number);
        else
            std::printf("%s(%s)", token_name(token), token.text.data());

        if (token.type == TokenType::Newline)
            std::putchar('\n');
        else
            std::putchar(' ');
    }

    return {};
}

