#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>

#include <stddef.h>
#include <stdint.h>

#include "common.hpp"
#include "parser.hpp"

enum class TokenType
{
    Data,
    Func,
    Hook,
    Rewrite,
    Identifier,
    String,
    Number,
    Newline,
    EndOfFile,
};

struct Token
{
    TokenType type;
    size_t line;
    uint32_t int_value;
    std::string_view text_value;
};

struct SourceLocation
{
    std::string_view file;
    size_t line;
};

static std::unordered_map<std::string_view, TokenType> s_keyword_table =
{
    {"data", TokenType::Data},
    {"func", TokenType::Func},
    {"hook", TokenType::Hook},
    {"rewrite", TokenType::Rewrite},
};

// The behavior of all functions from <cctype> is undefined if the argument's value is
// neither representable as unsigned char nor equal to EOF. To use these functions safely
// with plain chars (or signed chars), the argument should first be converted to unsigned char.

static bool is_space_char(char c)
{
    return c != '\n' && std::isspace(static_cast<unsigned char>(c));
}

static bool is_alpha_char(char c)
{
    return std::isalpha(static_cast<unsigned char>(c));
}

static bool is_alnum_char(char c)
{
    return std::isalnum(static_cast<unsigned char>(c));
}

static bool is_xdigit_char(char c)
{
    return std::isxdigit(static_cast<unsigned char>(c));
}

static bool is_identifier_start(char c)
{
    return is_alpha_char(c) || c == '_';
}

static bool is_identifier_char(char c)
{
    return is_alnum_char(c) || c == '_';
}

static Token keyword_token(size_t line, TokenType type)
{
    Token token;
    token.type = type;
    token.line = line;
    return token;
}

static Token identifier_token(size_t line, std::string_view text_value)
{
    Token token;
    token.type = TokenType::Identifier;
    token.line = line;
    token.text_value = text_value;
    return token;
}

static Token string_token(size_t line, std::string_view text_value)
{
    Token token;
    token.type = TokenType::String;
    token.line = line;
    token.text_value = text_value;
    return token;
}

static Token number_token(size_t line, uint32_t int_value)
{
    Token token;
    token.type = TokenType::Number;
    token.line = line;
    token.int_value = int_value;
    return token;
}

static std::string_view token_type_name(TokenType type)
{
    switch (type)
    {
        case TokenType::Data: return "data";
        case TokenType::Func: return "func";
        case TokenType::Hook: return "hook";
        case TokenType::Rewrite: return "rewrite";
        case TokenType::Identifier: return "identifier";
        case TokenType::String: return "string";
        case TokenType::Number: return "number";
        case TokenType::Newline: return "newline";
        case TokenType::EndOfFile: return "EOF";
        default: return "unknown";
    }
}

static std::ostream& operator<<(std::ostream& stream, SourceLocation loc)
{
    stream << loc.file << ":" << loc.line << ": ";
    return stream;
}

template<typename... TArgs>
static void source_error(SourceLocation loc, std::format_string<TArgs...> fmt, TArgs&&... args)
{
    std::cerr << loc << std::format(fmt, std::forward<TArgs>(args)...) << '\n';
    std::exit(EXIT_FAILURE);
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
            auto token = keyword_token(line, TokenType::Newline);
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
                auto token = keyword_token(line, table_it->second);
                tokens.push_back(token);
            }
            else
            {
                auto token = identifier_token(line, text_value);
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

            auto token = number_token(line, int_value);
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
            auto token = string_token(line, text_value);
            tokens.push_back(token);
        }
    }

    Token eof_token = keyword_token(line, TokenType::EndOfFile);
    tokens.push_back(eof_token);

    return tokens;
}

static PatchJobs parse(const std::vector<Token>& tokens, std::string_view file)
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

PatchJobs load_patch_file(std::string_view file)
{
    std::string source = read_text_file(file);
    std::vector<Token> tokens = tokenize(source, file);
    PatchJobs jobs = parse(tokens, file);
    return jobs;
}

