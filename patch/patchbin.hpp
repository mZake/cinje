#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <unordered_map>

// Macros for byte-order conversion
#if defined(__linux__)
    #include <endian.h>
    #define LITTLE_TO_HOST16(x) le16toh(x)
    #define LITTLE_TO_HOST32(x) le32toh(x)
    #define LITTLE_TO_HOST64(x) le64toh(x)
    #define HOST_TO_LITTLE16(x) htole16(x)
    #define HOST_TO_LITTLE32(x) htole32(x)
    #define HOST_TO_LITTLE64(x) htole64(x)
#elif defined(_WIN32) // Windows is always little-endian
    #define LITTLE_TO_HOST16(x) (x)
    #define LITTLE_TO_HOST32(x) (x)
    #define LITTLE_TO_HOST64(x) (x)
    #define HOST_TO_LITTLE16(x) (x)
    #define HOST_TO_LITTLE32(x) (x)
    #define HOST_TO_LITTLE64(x) (x)
#endif

#define LOCATION            Location{__FILE__, __LINE__}

#define PATCH_BYTES(...)    patchbin->patch_bytes(LOCATION, __VA_ARGS__)
#define PATCH_POINTER(...)  patchbin->patch_pointer(LOCATION, __VA_ARGS__)
#define PATCH_HOOK(...)     patchbin->patch_hook(LOCATION, __VA_ARGS__)
#define PATCH_FUNC(...)     patchbin->patch_function(LOCATION, __VA_ARGS__)

class BufferParser
{
public:
    BufferParser(const std::uint8_t* begin, const std::uint8_t* end)
        : m_buffer_begin(begin), m_buffer_end(end), m_buffer_current(begin) {}

    BufferParser(const std::uint8_t* begin, std::size_t length)
        : m_buffer_begin(begin), m_buffer_end(begin + length), m_buffer_current(begin) {}

    bool read_bytes(std::uint8_t* output, std::size_t count);
    bool read_byte(std::uint8_t& output);

    bool read_uint16(std::uint16_t& output);
    bool read_uint32(std::uint32_t& output);
    bool read_uint64(std::uint64_t& output);

    bool read_little_uint16(std::uint16_t& output);
    bool read_little_uint32(std::uint32_t& output);
    bool read_little_uint64(std::uint64_t& output);

    bool seek(std::size_t offset);

private:
    const std::uint8_t* m_buffer_begin = nullptr;
    const std::uint8_t* m_buffer_end = nullptr;
    const std::uint8_t* m_buffer_current = nullptr;
};

struct BufferBuilder
{
    std::vector<std::uint8_t> buffer;

    void write_byte(std::uint8_t byte);
    void write_byte(std::uint8_t byte, std::size_t count);
    void write_bytes(std::initializer_list<std::uint8_t> bytes);
    void write_bytes(const std::uint8_t* bytes, std::size_t count);

    void write_uint16(std::uint16_t value);
    void write_uint32(std::uint32_t value);
    void write_uint64(std::uint64_t value);

    void write_little_uint16(std::uint16_t value);
    void write_little_uint32(std::uint32_t value);
    void write_little_uint64(std::uint64_t value);
};

struct Location
{
    const char* file = nullptr;
    int line = 0;
};

enum class PatchErrorCode
{
    None,
    ImageBoundsExceeded,
    SymbolNotFound,
    TooManyParams,
    UnknownRegister,
};

struct PatchError
{
    Location location;
    PatchErrorCode code = PatchErrorCode::None;
    int int_data = 0;
    std::string string_data;
};

class Patchbin
{
public:
    Patchbin(int argc, char** argv);

    void patch_bytes(Location location, std::uint32_t offset,
                     const std::vector<std::uint8_t>& bytes);

    void patch_pointer(Location location, std::uint32_t offset,
                       std::string_view symbol, bool is_thumb);

    void patch_hook(Location location, std::uint32_t offset,
                    std::string_view symbol, std::uint8_t reg);

    void patch_function(Location location, std::uint32_t offset,
                        std::string_view symbol, std::uint32_t params, bool returns);

    void finish();

private:
    std::uint32_t get_symbol_value(std::string_view symbol);

    bool copy_bytes_to_image(std::uint32_t offset,
                             const std::vector<std::uint8_t>& bytes);

    void report_image_bounds_exceeded(Location location);
    void report_symbol_not_found(Location location, std::string_view symbol);
    void report_too_many_params(Location location, std::uint32_t params);
    void report_unknown_register(Location location, std::uint8_t reg);

    bool process_patch_errors();

private:
    std::string m_base_image_path;
    std::string m_output_image_path;
    std::vector<std::uint8_t> m_image;
    std::unordered_map<std::string, std::uint32_t> m_symbols;
    std::vector<PatchError> m_patch_errors;
};

#ifdef PATCHBIN_IMPLEMENTATION

static std::unique_ptr<Patchbin> patchbin;

static void log_error(Location location, const char* format ...);
static void log_fatal_no_prefix(const char* format, std::va_list args);
static void log_fatal_no_prefix(const char* format, ...);
static void log_fatal(const char* format, ...);

static std::vector<std::uint8_t> read_binary_file(const char* filepath);
static std::string read_text_file(const char* filepath);
static void write_binary_file(const char* filepath, const std::uint8_t* data, std::size_t size);

static bool is_space_char(char c);
static bool is_alpha_char(char c);
static bool is_alnum_char(char c);

static bool is_symbol_name_valid(std::string_view name);

static std::unordered_map<std::string, std::uint32_t> parse_symbol_list(const char* filepath);

static void log_error(Location location, const char* format ...)
{
    std::va_list args;
    va_start(args, format);

    std::fprintf(stderr, "patchbin: %s(%d): ", location.file, location.line);
    std::vfprintf(stderr, format, args);
    std::fprintf(stderr, "\n");

    va_end(args);
}

static void log_fatal_no_prefix(const char* format, std::va_list args)
{
    std::vfprintf(stderr, format, args);
    std::fprintf(stderr, "\n");
    std::exit(EXIT_FAILURE);
}

static void log_fatal_no_prefix(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    log_fatal_no_prefix(format, args);
    // No va_end because the program is terminated at this point.
}

static void log_fatal(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    std::fprintf(stderr, "patchbin: error: ");
    log_fatal_no_prefix(format, args);
    // No va_end because the program is terminated at this point.
}

static std::vector<std::uint8_t> read_binary_file(const char* filepath)
{
    std::FILE* stream = std::fopen(filepath, "rb");
    if (!stream)
    {
        log_fatal("cannot open file for reading: %s", filepath);
    }

    std::fseek(stream, 0, SEEK_END);
    long size = std::ftell(stream);
    std::fseek(stream, 0, SEEK_SET);

    std::vector<std::uint8_t> buffer;
    buffer.resize(static_cast<std::size_t>(size));

    if (std::fread(buffer.data(), 1, buffer.size(), stream) != buffer.size())
    {
        log_fatal("cannot read file: %s", filepath);
    }

    std::fclose(stream);

    return buffer;
}

static std::string read_text_file(const char* filepath)
{
    std::FILE* stream = std::fopen(filepath, "r");
    if (!stream)
    {
        log_fatal("cannot open file for reading: %s", filepath);
    }

    std::fseek(stream, 0, SEEK_END);
    long size = std::ftell(stream);
    std::fseek(stream, 0, SEEK_SET);

    std::string text;
    text.resize(static_cast<std::size_t>(size));

    if (std::fread(text.data(), 1, text.size(), stream) != text.size())
    {
        log_fatal("cannot read file: %s", filepath);
    }

    std::fclose(stream);

    return text;
}

static void write_binary_file(const char* filepath, const std::uint8_t* data, std::size_t size)
{
    std::FILE* stream = std::fopen(filepath, "wb");
    if (!stream)
    {
        log_fatal("cannot open file for writing: %s", filepath);
    }

    if (std::fwrite(data, 1, size, stream) != size)
    {
        log_fatal("cannot write file: %s", filepath);
    }

    std::fclose(stream);
}

static bool is_space_char(char c)
{
    return std::isspace(static_cast<unsigned char>(c));
}

static bool is_alpha_char(char c)
{
    return std::isalpha(static_cast<unsigned char>(c));
}

static bool is_alnum_char(char c)
{
    return std::isalnum(static_cast<unsigned char>(c));
}

static bool is_symbol_name_valid(std::string_view name)
{
    if (char c = name[0]; !is_alpha_char(c) && c != '_' && c != '.' && c != '$')
    {
        return false;
    }

    for (char c : name)
    {
        if (!is_alnum_char(c) && c != '_' && c != '.' && c != '$')
        {
            return false;
        }
    }

    return true;
}

static std::unordered_map<std::string, std::uint32_t> parse_symbol_list(const char* filepath)
{
    std::vector<std::string> tokens;

    std::string source = read_text_file(filepath);
    const char* c = source.data();

    while (true)
    {
        while (*c != '\0' && is_space_char(*c))
        {
            ++c;
        }

        if (*c == '\0')
        {
            break;
        }

        std::string token;
        while (*c != '\0' && !is_space_char(*c))
        {
            token += *(c++);
        }

        tokens.push_back(std::move(token));
    }

    std::unordered_map<std::string, std::uint32_t> symbols;

    auto token = tokens.begin();
    while (token != tokens.end())
    {
        if ((token + 2) >= tokens.end())
        {
            log_fatal("[%s] unexpected EOF", filepath);
        }

        auto addr_token = *(token++);
        auto type_token = *(token++);
        auto name_token = *(token++);

        std::uint32_t address = 0;
        auto result = std::from_chars(addr_token.data(), addr_token.data() + addr_token.size(),
                                      address, 16);
        if (result.ec == std::errc::invalid_argument)
        {
            log_fatal("[%s] invalid address: %s", filepath, addr_token.c_str());
        }
        if (result.ec == std::errc::result_out_of_range)
        {
            log_fatal("[%s] out of range address: %s", filepath, addr_token.c_str());
        }

        std::string name = std::move(name_token);
        if (!is_symbol_name_valid(name))
        {
            log_fatal("[%s] invalid name: %s", filepath, name.c_str());
        }

        symbols[name] = address;
    }

    return symbols;
}

bool BufferParser::read_bytes(std::uint8_t* output, std::size_t count)
{
    if (m_buffer_current >= m_buffer_end)
        return false;

    auto next_current = m_buffer_current + count;
    if (next_current >= m_buffer_end)
        return false;

    auto bytes = reinterpret_cast<const std::uint8_t*>(m_buffer_current);
    std::memcpy(output, bytes, count);
    m_buffer_current = next_current;

    return true;
}

bool BufferParser::read_byte(std::uint8_t& output)
{
    return read_bytes(&output, sizeof(std::uint8_t));
}

bool BufferParser::read_uint16(std::uint16_t& output)
{
    return read_bytes(reinterpret_cast<std::uint8_t*>(&output), sizeof(std::uint16_t));
}

bool BufferParser::read_uint32(std::uint32_t& output)
{
    return read_bytes(reinterpret_cast<std::uint8_t*>(&output), sizeof(std::uint32_t));
}

bool BufferParser::read_uint64(std::uint64_t& output)
{
    return read_bytes(reinterpret_cast<std::uint8_t*>(&output), sizeof(std::uint64_t));
}

bool BufferParser::read_little_uint16(std::uint16_t& output)
{
    if (read_uint16(output)) {
        output = LITTLE_TO_HOST16(output);
        return true;
    }

    return false;
}

bool BufferParser::read_little_uint32(std::uint32_t& output)
{
    if (read_uint32(output)) {
        output = LITTLE_TO_HOST32(output);
        return true;
    }

    return false;
}

bool BufferParser::read_little_uint64(std::uint64_t& output)
{
    if (read_uint64(output)) {
        output = LITTLE_TO_HOST64(output);
        return true;
    }

    return false;
}

bool BufferParser::seek(std::size_t offset)
{
    auto next_current = m_buffer_begin + offset;
    if (next_current >= m_buffer_end)
        return false;

    m_buffer_current = next_current;
    return true;
}

void BufferBuilder::write_byte(std::uint8_t byte)
{
    buffer.push_back(byte);
}

void BufferBuilder::write_byte(std::uint8_t byte, std::size_t count)
{
    buffer.insert(buffer.end(), count, byte);
}

void BufferBuilder::write_bytes(std::initializer_list<std::uint8_t> bytes)
{
    buffer.insert(buffer.end(), bytes);
}

void BufferBuilder::write_bytes(const std::uint8_t* bytes, std::size_t count)
{
    buffer.insert(buffer.end(), bytes, bytes + count);
}

void BufferBuilder::write_uint16(std::uint16_t value)
{
    write_bytes(reinterpret_cast<std::uint8_t*>(&value), sizeof(std::uint16_t));
}

void BufferBuilder::write_uint32(std::uint32_t value)
{
    write_bytes(reinterpret_cast<std::uint8_t*>(&value), sizeof(std::uint32_t));
}

void BufferBuilder::write_uint64(std::uint64_t value)
{
    write_bytes(reinterpret_cast<std::uint8_t*>(&value), sizeof(std::uint64_t));
}

void BufferBuilder::write_little_uint16(std::uint16_t value)
{
    write_uint16(HOST_TO_LITTLE16(value));
}

void BufferBuilder::write_little_uint32(std::uint32_t value)
{
    write_uint32(HOST_TO_LITTLE32(value));
}

void BufferBuilder::write_little_uint64(std::uint64_t value)
{
    write_uint64(HOST_TO_LITTLE64(value));
}

struct ParsedArgs
{
    const char* base_image_path = nullptr;
    const char* binary_path = nullptr;
    const char* symbol_list_path = nullptr;
    const char* output_image_path = nullptr;
};

static ParsedArgs parse_args(int argc, char** argv)
{
    if (argc != 5)
    {
        log_fatal_no_prefix("Usage: patchbin base-image binary symbol-list output-image");
    }

    // Skip executable path argument.
    ++argv;

    ParsedArgs args;
    args.base_image_path = *argv++;
    args.binary_path = *argv++;
    args.symbol_list_path = *argv++;
    args.output_image_path = *argv++;

    return args;
}

Patchbin::Patchbin(int argc, char** argv)
{
    ParsedArgs args = parse_args(argc, argv);

    m_base_image_path = args.base_image_path;
    m_output_image_path = args.output_image_path;

    m_image = read_binary_file(args.base_image_path);

    m_symbols = parse_symbol_list(args.symbol_list_path);

    std::vector<std::uint8_t> binary = read_binary_file(args.binary_path);
    std::uint32_t offset_to_insert = get_symbol_value("OFFSET_TO_INSERT");

    if (!copy_bytes_to_image(offset_to_insert, binary))
    {
        log_fatal("binary is out of range");
    }
}

void Patchbin::patch_bytes(Location location, std::uint32_t offset,
                           const std::vector<std::uint8_t>& bytes)
{
    if (!copy_bytes_to_image(offset, bytes))
    {
        report_image_bounds_exceeded(location);
    }
}

void Patchbin::patch_pointer(Location location, std::uint32_t offset,
                             std::string_view symbol, bool is_thumb)
{
    std::uint32_t address = get_symbol_value(symbol);
    if (!address)
    {
        report_symbol_not_found(location, symbol);
        return;
    }
 
    address = is_thumb ? (address | 1) : (address & ~1);

    BufferBuilder builder;
    builder.write_little_uint32(address);

    patch_bytes(location, offset, builder.buffer);
}

void Patchbin::patch_hook(Location location, std::uint32_t offset,
                          std::string_view symbol, std::uint8_t reg)
{
    std::uint32_t address = get_symbol_value(symbol);
    if (!address)
    {
        report_symbol_not_found(location, symbol);
        return;
    }

    if (reg > 15)
    {
        report_unknown_register(location, reg);
        return;
    }

    std::uint8_t reg_bits = reg & 7;

    BufferBuilder builder;
    if (offset % 4)
    {
        builder.write_byte(0x01);
        builder.write_byte(0x48 | reg_bits);
        builder.write_byte(0x00 | (reg_bits << 3));
        builder.write_bytes({0x47, 0x00, 0x00});
    } 
    else
    {
        builder.write_byte(0x00);
        builder.write_byte(0x48 | reg_bits);
        builder.write_byte(0x00 | (reg_bits << 3));
        builder.write_byte(0x47);
    }
    builder.write_little_uint32(address | 1);

    patch_bytes(location, offset, builder.buffer);
}

void Patchbin::patch_function(Location location, std::uint32_t offset,
                              std::string_view symbol, std::uint32_t params, bool returns)
{
    std::uint32_t address = get_symbol_value(symbol);
    if (!address)
    {
        report_symbol_not_found(location, symbol);
        return;
    }

    if (params > 4)
    {
        report_too_many_params(location, params);
        return;
    }

    BufferBuilder builder;
    builder.write_bytes({0x10, 0xB5, 0x03, 0x4C, 0x00, 0xF0, 0x03, 0xF8, 0x10, 0xBC});
    builder.write_byte(returns + 1);
    builder.write_byte(0xBC);
    builder.write_byte(returns << 3);
    builder.write_bytes({0x47, 0x20, 0x47});
    builder.write_little_uint32(address | 1);

    patch_bytes(location, offset, builder.buffer);
}

void Patchbin::finish()
{
    if (process_patch_errors())
    {
        write_binary_file(m_output_image_path.c_str(), m_image.data(), m_image.size());
    }
}

std::uint32_t Patchbin::get_symbol_value(std::string_view symbol)
{
    if (auto it = m_symbols.find(std::string(symbol)); it != m_symbols.end())
    {
        return it->second;
    }

    return 0;
}

bool Patchbin::copy_bytes_to_image(std::uint32_t offset,
                                   const std::vector<std::uint8_t>& bytes)
{
    auto dest = m_image.begin() + offset;
    if (dest >= m_image.end())
    {
        return false;
    }

    std::copy(bytes.begin(), bytes.end(), dest);
    return true;
}

void Patchbin::report_image_bounds_exceeded(Location location)
{
    PatchError error{
        location,
        PatchErrorCode::ImageBoundsExceeded,
        0,
        ""
    };
    m_patch_errors.push_back(std::move(error));
}

void Patchbin::report_symbol_not_found(Location location, std::string_view symbol)
{
    PatchError error{
        location,
        PatchErrorCode::SymbolNotFound,
        0,
        std::string(symbol),
    };
    m_patch_errors.push_back(std::move(error));
}

void Patchbin::report_too_many_params(Location location, std::uint32_t params)
{
    PatchError error{
        location,
        PatchErrorCode::TooManyParams,
        static_cast<int>(params),
        ""
    };
    m_patch_errors.push_back(std::move(error));
}

void Patchbin::report_unknown_register(Location location, std::uint8_t reg)
{
    PatchError error{
        location,
        PatchErrorCode::UnknownRegister,
        static_cast<int>(reg),
        ""
    };
    m_patch_errors.push_back(std::move(error));
}

bool Patchbin::process_patch_errors()
{
    if (m_patch_errors.empty())
    {
        return true;
    }

    for (const auto& error : m_patch_errors)
    {
        switch (error.code)
        {
            case PatchErrorCode::ImageBoundsExceeded:
                log_error(error.location,
                          "patch exceeds '%s' bounds",
                          m_base_image_path.c_str());
                break;
            case PatchErrorCode::SymbolNotFound:
                log_error(error.location,
                          "symbol '%s' not found",
                          error.string_data.c_str());
                break;
            case PatchErrorCode::TooManyParams:
                log_error(error.location,
                          "expected at most 4 parameters, got %d",
                          error.int_data);
                break;
            case PatchErrorCode::UnknownRegister:
                log_error(error.location,
                          "unknown register 'r%d'",
                          error.int_data);
                break;
            case PatchErrorCode::None:
            default:
                break;
        }
    }

    return false;
}

extern void patchbin_main();

int main(int argc, char** argv)
{
    patchbin = std::make_unique<Patchbin>(argc, argv);

    patchbin_main();

    patchbin->finish();
}

#endif

