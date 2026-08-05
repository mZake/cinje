#pragma once

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>
#include <unordered_map>

#include <stdint.h>
#include <stddef.h>

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

#define LOCATION Location{__FILE__, __LINE__}

#define REPOINT(...) g_patcher->repoint(LOCATION, __VA_ARGS__)
#define REPLACE(...) g_patcher->replace(LOCATION, __VA_ARGS__)
#define REWRITE(...) g_patcher->rewrite(LOCATION, __VA_ARGS__)
#define HOOK(...) g_patcher->hook(LOCATION, __VA_ARGS__)

namespace fs = std::filesystem;

class BufferParser
{
public:
    BufferParser(const uint8_t* begin, const uint8_t* end)
        : m_buffer_begin(begin), m_buffer_end(end), m_buffer_current(begin) {}

    BufferParser(const uint8_t* begin, size_t length)
        : m_buffer_begin(begin), m_buffer_end(begin + length), m_buffer_current(begin) {}

    bool read_bytes(uint8_t* output, size_t count);
    bool read_byte(uint8_t& output);

    bool read_uint16(uint16_t& output);
    bool read_uint32(uint32_t& output);
    bool read_uint64(uint64_t& output);

    bool read_little_uint16(uint16_t& output);
    bool read_little_uint32(uint32_t& output);
    bool read_little_uint64(uint64_t& output);

    bool seek(size_t offset);

private:
    const uint8_t* m_buffer_begin = nullptr;
    const uint8_t* m_buffer_end = nullptr;
    const uint8_t* m_buffer_current = nullptr;
};

struct BufferBuilder
{
    std::vector<uint8_t> buffer;

    void write_bytes(uint8_t byte);
    void write_bytes(std::initializer_list<uint8_t> bytes);
    void write_bytes(const uint8_t* bytes, size_t count);
    void write_bytes(size_t count, uint8_t value);

    void write_uint16(uint16_t value);
    void write_uint32(uint32_t value);
    void write_uint64(uint64_t value);

    void write_little_uint16(uint16_t value);
    void write_little_uint32(uint32_t value);
    void write_little_uint64(uint64_t value);
};

// Tiny ELF library for parsing ARM 32-bit LSB objects

typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;
typedef uint16_t Elf32_Half;

constexpr int EI_MAG0 = 0;
constexpr int EI_MAG1 = 1;
constexpr int EI_MAG2 = 2;
constexpr int EI_MAG3 = 3;
constexpr int EI_CLASS = 4;
constexpr int EI_DATA = 5;
constexpr int EI_VERSION = 6;
constexpr int EI_PAD = 7;
constexpr int EI_NIDENT = 16;

constexpr int ELFMAG0 = 0x7F;
constexpr int ELFMAG1 = 'E';
constexpr int ELFMAG2 = 'L';
constexpr int ELFMAG3 = 'F';

constexpr int ELFCLASSNONE = 0;
constexpr int ELFCLASS32 = 1;
constexpr int ELFCLASS64 = 2;

constexpr int ELFDATANONE = 0;
constexpr int ELFDATA2LSB = 1;
constexpr int ELFDATA2MSB = 2;

constexpr int EV_NONE = 0;
constexpr int EV_CURRENT = 1;

constexpr int ET_NONE = 0;
constexpr int ET_REL = 1;
constexpr int ET_EXEC = 2;
constexpr int ET_DYN = 3;
constexpr int ET_CORE = 4;

constexpr int SHN_UNDEF = 0;
constexpr int SHN_LORESERVE = 0xFF00;
constexpr int SHN_LOPROC = 0xFF00;
constexpr int SHN_HIPROC = 0xFF1F;
constexpr int SHN_ABS = 0xFFF1;
constexpr int SHN_COMMON = 0xFFF2;
constexpr int SHN_HIRESERVE = 0xFFFF;

constexpr int SHF_WRITE = 0x1;
constexpr int SHF_ALLOC = 0x2;
constexpr int SHF_EXECINSTR = 0x4;

constexpr int SHT_NULL = 0;
constexpr int SHT_PROGBITS = 1;
constexpr int SHT_SYMTAB = 2;
constexpr int SHT_STRTAB = 3;
constexpr int SHT_RELA = 4;
constexpr int SHT_HASH = 5;
constexpr int SHT_DYNAMIC = 6;
constexpr int SHT_NOTE = 7;
constexpr int SHT_NOBITS = 8;
constexpr int SHT_REL = 9;
constexpr int SHT_SHLIB = 10;
constexpr int SHT_DYNSYM = 11;

constexpr int STT_NOTYPE = 0;
constexpr int STT_OBJECT = 1;
constexpr int STT_FUNC = 2;
constexpr int STT_SECTION = 3;
constexpr int STT_FILE = 4;

#define ELF32_ST_BIND(x) ((x) >> 4)
#define ELF32_ST_TYPE(x) ((x) & 0xF)

struct Elf32_Ehdr
{
    unsigned char e_ident[EI_NIDENT] = {0};
    Elf32_Half e_type = 0;
    Elf32_Half e_machine = 0;
    Elf32_Word e_version = 0;
    Elf32_Addr e_entry = 0;
    Elf32_Off e_phoff = 0;
    Elf32_Off e_shoff = 0;
    Elf32_Word e_flags = 0;
    Elf32_Half e_ehsize = 0;
    Elf32_Half e_phentsize = 0;
    Elf32_Half e_phnum = 0;
    Elf32_Half e_shentsize = 0;
    Elf32_Half e_shnum = 0;
    Elf32_Half e_shstrndx = 0;
};

struct Elf32_Shdr
{
    Elf32_Word sh_name = 0;
    Elf32_Word sh_type = 0;
    Elf32_Word sh_flags = 0;
    Elf32_Addr sh_addr = 0;
    Elf32_Off sh_offset = 0;
    Elf32_Word sh_size = 0;
    Elf32_Word sh_link = 0;
    Elf32_Word sh_info = 0;
    Elf32_Word sh_addralign = 0;
    Elf32_Word sh_entsize = 0;
};

struct Elf32_Sym
{
    Elf32_Word st_name = 0;
    Elf32_Addr st_value = 0;
    Elf32_Word st_size = 0;
    unsigned char st_info = 0;
    unsigned char st_other = 0;
    Elf32_Half st_shndx = 0;
};

class Elf32
{
public:
    Elf32(const char* path);

    Elf32_Shdr get_section(uint32_t index) const;
    const char* get_string(const Elf32_Shdr& section, uint32_t index) const;

    std::vector<Elf32_Sym> extract_symbols(const Elf32_Shdr& section) const;
    std::vector<uint8_t> extract_binary() const;

    Elf32_Ehdr header() const { return m_header; }
    const std::vector<Elf32_Shdr>& sections() const { return m_sections; }

private:
    Elf32_Ehdr m_header;
    std::vector<Elf32_Shdr> m_sections;
    std::vector<uint8_t> m_payload;
};

struct Patch
{
    std::vector<uint8_t> bytes;
    uint32_t offset = 0;
};

struct Location
{
    const char* file = nullptr;
    int line = 0;
};

using SymbolMap = std::unordered_map<std::string_view, uint32_t>;

class Patcher
{
public:
    Patcher(const char* input_path, size_t input_size, const SymbolMap* symbol_map)
        : m_input_path(input_path), m_input_size(input_size), m_symbol_map(symbol_map) {}

    void repoint(Location location, const char* symbol, uint32_t offset, bool set_thumb_bit);
    void replace(Location location, uint32_t offset, std::vector<uint8_t> bytes);
    void rewrite(Location location, const char* symbol, uint32_t offset,
                 uint32_t param_count, uint8_t returns);
    void hook(Location location, const char* symbol, uint32_t offset, uint8_t register_id);

    void patch(const char* output_path);

    const std::vector<Patch>& patches() const { return m_patches; }

private:
    void error(Location location, const char* format, ...) const;
    std::optional<uint32_t> get_symbol_address(const char* symbol) const;

private:
    const char* m_input_path = nullptr;
    size_t m_input_size = 0;
    const std::unordered_map<std::string_view, uint32_t>* m_symbol_map;
    std::vector<Patch> m_patches;
    mutable bool m_has_error = false;
};

inline std::unique_ptr<Patcher> g_patcher;

#ifdef PATCHBIN_IMPLEMENTATION

static void log_fatal(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);

    std::fprintf(stderr, "patchbin: error: ");
    std::vfprintf(stderr, format, args);
    std::fprintf(stderr, "\n");

    va_end(args);
    std::exit(EXIT_FAILURE);
}

static std::vector<uint8_t> read_entire_file(const char* filepath)
{
    std::FILE* stream = std::fopen(filepath, "rb");
    if (!stream)
        log_fatal("cannot open file: %s", filepath);

    std::fseek(stream, 0, SEEK_END);
    long size = std::ftell(stream);
    std::fseek(stream, 0, SEEK_SET);

    std::vector<uint8_t> buffer;
    buffer.resize(size);
    if (std::fread(buffer.data(), 1, buffer.size(), stream) != buffer.size())
        log_fatal("cannot read file: %s", filepath);

    return buffer;
}

static uint32_t to_offset(uint32_t address)
{
    return address - 0x8000000;
}

bool BufferParser::read_bytes(uint8_t* output, size_t count)
{
    if (m_buffer_current >= m_buffer_end)
        return false;

    auto next_current = m_buffer_current + count;
    if (next_current >= m_buffer_end)
        return false;

    auto bytes = reinterpret_cast<const uint8_t*>(m_buffer_current);
    std::memcpy(output, bytes, count);
    m_buffer_current = next_current;

    return true;
}

bool BufferParser::read_byte(uint8_t& output)
{
    return read_bytes(&output, sizeof(uint8_t));
}

bool BufferParser::read_uint16(uint16_t& output)
{
    return read_bytes(reinterpret_cast<uint8_t*>(&output), sizeof(uint16_t));
}

bool BufferParser::read_uint32(uint32_t& output)
{
    return read_bytes(reinterpret_cast<uint8_t*>(&output), sizeof(uint32_t));
}

bool BufferParser::read_uint64(uint64_t& output)
{
    return read_bytes(reinterpret_cast<uint8_t*>(&output), sizeof(uint64_t));
}

bool BufferParser::read_little_uint16(uint16_t& output)
{
    if (read_uint16(output)) {
        output = LITTLE_TO_HOST16(output);
        return true;
    }

    return false;
}

bool BufferParser::read_little_uint32(uint32_t& output)
{
    if (read_uint32(output)) {
        output = LITTLE_TO_HOST32(output);
        return true;
    }

    return false;
}

bool BufferParser::read_little_uint64(uint64_t& output)
{
    if (read_uint64(output)) {
        output = LITTLE_TO_HOST64(output);
        return true;
    }

    return false;
}

bool BufferParser::seek(size_t offset)
{
    auto next_current = m_buffer_begin + offset;
    if (next_current >= m_buffer_end)
        return false;

    m_buffer_current = next_current;
    return true;
}

void BufferBuilder::write_bytes(uint8_t byte)
{
    buffer.push_back(byte);
}

void BufferBuilder::write_bytes(std::initializer_list<uint8_t> bytes)
{
    buffer.insert(buffer.end(), bytes);
}

void BufferBuilder::write_bytes(const uint8_t* bytes, size_t count)
{
    buffer.insert(buffer.end(), bytes, bytes + count);
}

void BufferBuilder::write_bytes(size_t count, uint8_t value)
{
    buffer.insert(buffer.end(), count, value);
}

void BufferBuilder::write_uint16(uint16_t value)
{
    auto data_begin = reinterpret_cast<uint8_t*>(&value);
    auto data_end = data_begin + sizeof(uint16_t);
    buffer.insert(buffer.end(), data_begin, data_end);
}

void BufferBuilder::write_uint32(uint32_t value)
{
    auto data_begin = reinterpret_cast<uint8_t*>(&value);
    auto data_end = data_begin + sizeof(uint32_t);
    buffer.insert(buffer.end(), data_begin, data_end);
}

void BufferBuilder::write_uint64(uint64_t value)
{
    auto data_begin = reinterpret_cast<uint8_t*>(&value);
    auto data_end = data_begin + sizeof(uint64_t);
    buffer.insert(buffer.end(), data_begin, data_end);
}

void BufferBuilder::write_little_uint16(uint16_t value)
{
    write_uint16(HOST_TO_LITTLE16(value));
}

void BufferBuilder::write_little_uint32(uint32_t value)
{
    write_uint32(HOST_TO_LITTLE32(value));
}

void BufferBuilder::write_little_uint64(uint64_t value)
{
    write_uint64(HOST_TO_LITTLE64(value));
}

Elf32::Elf32(const char* path)
{
    std::vector<uint8_t> payload = read_entire_file(path);
    BufferParser parser(payload.data(), payload.size());

    parser.read_bytes(m_header.e_ident, EI_NIDENT);
    if (m_header.e_ident[EI_MAG0] != ELFMAG0 || m_header.e_ident[EI_MAG1] != ELFMAG1 ||
        m_header.e_ident[EI_MAG2] != ELFMAG2 || m_header.e_ident[EI_MAG3] != ELFMAG3)
        log_fatal("file is not an ELF object: %s", path);

    if (m_header.e_ident[EI_CLASS] != ELFCLASS32)
        log_fatal("ELF is not 32-bit: %s", path);

    if (m_header.e_ident[EI_DATA] != ELFDATA2LSB)
        log_fatal("ELF is not LSB: %s", path);

    if (m_header.e_ident[EI_VERSION] != EV_CURRENT)
        log_fatal("ELF version is invalid: %s", path);

    parser.read_little_uint16(m_header.e_type);
    parser.read_little_uint16(m_header.e_machine);
    parser.read_little_uint32(m_header.e_version);
    parser.read_little_uint32(m_header.e_entry);
    parser.read_little_uint32(m_header.e_phoff);
    parser.read_little_uint32(m_header.e_shoff);
    parser.read_little_uint32(m_header.e_flags);
    parser.read_little_uint16(m_header.e_ehsize);
    parser.read_little_uint16(m_header.e_phentsize);
    parser.read_little_uint16(m_header.e_phnum);
    parser.read_little_uint16(m_header.e_shentsize);
    parser.read_little_uint16(m_header.e_shnum);
    parser.read_little_uint16(m_header.e_shstrndx);

    if (m_header.e_type != ET_EXEC)
        log_fatal("ELF is not an executable: %s", path);

    if (m_header.e_shoff == 0)
        log_fatal("ELF does not contain a section header table: %s", path);

    m_sections.resize(m_header.e_shnum);

    parser.seek(m_header.e_shoff);

    for (auto& section : m_sections) {
        parser.read_little_uint32(section.sh_name);
        parser.read_little_uint32(section.sh_type);
        parser.read_little_uint32(section.sh_flags);
        parser.read_little_uint32(section.sh_addr);
        parser.read_little_uint32(section.sh_offset);
        parser.read_little_uint32(section.sh_size);
        parser.read_little_uint32(section.sh_link);
        parser.read_little_uint32(section.sh_info);
        parser.read_little_uint32(section.sh_addralign);
        parser.read_little_uint32(section.sh_entsize);
    }

    m_payload = std::move(payload);
}

Elf32_Shdr Elf32::get_section(uint32_t index) const
{
    return m_sections[index];
}

const char* Elf32::get_string(const Elf32_Shdr& section, uint32_t index) const
{
    assert(section.sh_type == SHT_STRTAB);
    auto strings = reinterpret_cast<const char*>(m_payload.data() + section.sh_offset);
    return strings + index;
}

std::vector<Elf32_Sym> Elf32::extract_symbols(const Elf32_Shdr& section) const
{
    assert(section.sh_type == SHT_SYMTAB);

    std::vector<Elf32_Sym> symbols;
    size_t count = section.sh_size / section.sh_entsize;
    symbols.resize(count);

    BufferParser parser(m_payload.data(), m_payload.size());
    parser.seek(section.sh_offset);

    for (auto& symbol : symbols) {
        parser.read_little_uint32(symbol.st_name);
        parser.read_little_uint32(symbol.st_value);
        parser.read_little_uint32(symbol.st_size);
        parser.read_byte(symbol.st_info);
        parser.read_byte(symbol.st_other);
        parser.read_little_uint16(symbol.st_shndx);
    }

    return symbols;
}

std::vector<uint8_t> Elf32::extract_binary() const
{
    // Collect all sections whose type is SHF_ALLOC
    std::vector<std::pair<Elf32_Shdr, size_t>> alloc_sections;
    for (size_t index = 0; index < m_sections.size(); ++index) {
        auto& section = m_sections[index];
        if (section.sh_flags & SHF_ALLOC)
            alloc_sections.emplace_back(section, index);
    }

    // Sort sections by the virtual address (or index if the addresses match) in ascending order
    std::sort(alloc_sections.begin(), alloc_sections.end(), [&](auto& lhs, auto& rhs) {
        if (lhs.first.sh_addr == rhs.first.sh_addr)
            return lhs.second < rhs.second;
        return lhs.first.sh_addr < rhs.first.sh_addr;
    });

    BufferBuilder builder;
    for (const auto& [section, index] : alloc_sections) {
        // Calculate necessary padding, if it is necessary in the first place
        uint32_t align = section.sh_addralign;
        if (align != 0 && builder.buffer.size() % align != 0) {
            uint32_t padding = align - builder.buffer.size() % align;
            builder.write_bytes(padding, 0x00);
        }

        if (section.sh_type == SHT_PROGBITS) {
            const uint8_t* bytes = m_payload.data() + section.sh_offset;
            builder.write_bytes(bytes, section.sh_size);
        } else if (section.sh_type == SHT_NOBITS) {
            builder.write_bytes(section.sh_size, 0x00);
        }
    }

    return builder.buffer;
}

void Patcher::repoint(Location location, const char* symbol, uint32_t offset, bool set_thumb_bit)
{
    auto address = get_symbol_address(symbol);
    if (!address)
        error(location, "symbol not found: %s", symbol);
 
    *address = set_thumb_bit ? (*address | 1) : (*address & ~1);

    BufferBuilder builder;
    builder.write_little_uint32(*address);

    uint32_t offset_end = offset + builder.buffer.size();
    if (offset_end >= m_input_size) {
        size_t count = offset_end - m_input_size;
        error(location, "operation overflows %s by 0x%zX bytes", m_input_path, count);
    }

    Patch patch;
    patch.offset = offset;
    patch.bytes = std::move(builder.buffer);
    m_patches.push_back(std::move(patch));
}

void Patcher::replace(Location location, uint32_t offset, std::vector<uint8_t> bytes)
{
    uint32_t offset_end = offset + bytes.size();
    if (offset_end >= m_input_size) {
        size_t count = offset_end - m_input_size;
        error(location, "operation overflows %s by 0x%zX bytes", m_input_path, count);
    }

    Patch patch;
    patch.offset = offset;
    patch.bytes = std::move(bytes);
    m_patches.push_back(std::move(patch));
}
void Patcher::rewrite(Location location, const char* symbol, uint32_t offset,
                      uint32_t param_count, uint8_t returns)
{
    auto address = get_symbol_address(symbol);
    if (!address)
        error(location, "symbol not found: %s", symbol);

    BufferBuilder builder;
    if (param_count <= 4) {
        builder.write_bytes({0x10, 0xB5, 0x03, 0x4C, 0x00, 0xF0, 0x03, 0xF8, 0x10, 0xBC});
        builder.write_bytes(returns + 1);
        builder.write_bytes(0xBC);
        builder.write_bytes(returns << 3);
        builder.write_bytes({0x47, 0x20, 0x47});
    } else {
        error(location, "cannot rewrite function with more than 4 parameters");
    }

    builder.write_little_uint32(*address | 1);

    uint32_t offset_end = offset + builder.buffer.size();
    if (offset_end >= m_input_size) {
        size_t count = offset_end - m_input_size;
        error(location, "operation overflows %s by 0x%zX bytes", m_input_path, count);
    }

    Patch patch;
    patch.offset = offset;
    patch.bytes = std::move(builder.buffer);
    m_patches.push_back(std::move(patch));
}

void Patcher::hook(Location location, const char* symbol, uint32_t offset, uint8_t register_id)
{
    uint8_t register_bits = register_id & 7;

    auto address = get_symbol_address(symbol);
    if (!address)
        error(location, "symbol not found: %s", symbol);

    if (register_id > 12)
        error(location, "register R%u is not usable", register_id);

    BufferBuilder builder;
    if (*address % 4) {
        builder.write_bytes(0x01);
        builder.write_bytes(0x48 | register_bits);
        builder.write_bytes(0x00 | (register_bits << 3));
        builder.write_bytes({0x47, 0x00, 0x00});
    } else {
        builder.write_bytes(0x00);
        builder.write_bytes(0x48 | register_bits);
        builder.write_bytes(0x00 | (register_bits << 3));
        builder.write_bytes(0x47);
    }
    builder.write_little_uint32(*address | 1);

    uint32_t offset_end = offset + builder.buffer.size();
    if (offset_end >= m_input_size) {
        size_t count = offset_end - m_input_size;
        error(location, "operation overflows %s by 0x%zX bytes", m_input_path, count);
    }

    Patch patch;
    patch.bytes = std::move(builder.buffer);
    patch.offset = offset & ~1; // Ensure offset alignment is 2
    m_patches.push_back(std::move(patch));
}

void Patcher::patch(const char* output_path)
{
    if (m_has_error)
        std::exit(EXIT_FAILURE);

    std::error_code ec; // Tag to force use of non-throwing overload
    if (!fs::copy_file(m_input_path, output_path, fs::copy_options::overwrite_existing, ec))
        log_fatal("cannot perform copy operation: from %s to %s", m_input_path, output_path);

    std::FILE* stream = std::fopen(output_path, "r+b");
    if (!stream)
        log_fatal("cannot open file: %s", output_path);

    for (const auto& patch : m_patches) {
        std::fseek(stream, patch.offset, SEEK_SET);
        std::fwrite(patch.bytes.data(), 1, patch.bytes.size(), stream);
    }
}

void Patcher::error(Location location, const char* format, ...) const
{
    std::va_list args;
    va_start(args, format);

    std::fprintf(stderr, "%s:%d: ", location.file, location.line);
    std::vfprintf(stderr, format, args);
    std::fprintf(stderr, "\n");

    m_has_error = true;
    va_end(args);
}

std::optional<uint32_t> Patcher::get_symbol_address(const char* symbol) const
{
    auto it = m_symbol_map->find(symbol);
    if (it != m_symbol_map->end())
        return it->second;

    return std::nullopt;
}

static SymbolMap symbol_map_from_elf(const Elf32& elf)
{
    SymbolMap symbol_map;

    Elf32_Shdr symtab_section;
    for (const auto& section : elf.sections()) {
        if (section.sh_type == SHT_SYMTAB) {
            symtab_section = section;
            break;
        }
    }

    Elf32_Shdr strtab_section = elf.get_section(symtab_section.sh_link);

    std::vector<Elf32_Sym> symbols = elf.extract_symbols(symtab_section);
    for (const auto& symbol : symbols) {
        uint32_t st_type = ELF32_ST_TYPE(symbol.st_info);
        if (st_type == STT_SECTION || st_type == STT_FILE)
            continue;

        // Filter symbols in irrelevant sections
        if (symbol.st_shndx == SHN_UNDEF || symbol.st_shndx == SHN_COMMON)
            continue;

        uint32_t address = 0;
        if (symbol.st_shndx == SHN_ABS) {
            address = symbol.st_value;
        } else {
            Elf32_Shdr section = elf.get_section(symbol.st_shndx);
            address = section.sh_addr + symbol.st_value;
        }

        std::string_view name = elf.get_string(strtab_section, symbol.st_name);
        symbol_map[name] = address;
    }

    return symbol_map;
}

extern void patchbin_main();

int main(int argc, char** argv)
{
    if (argc != 4) {
        std::printf("Usage: patchbin INPUT_ROM ELF_OBJECT OUTPUT_ROM\n");
        std::exit(EXIT_FAILURE);
    }

    const char* input_rom = argv[1];
    const char* elf_object = argv[2];
    const char* output_rom = argv[3];

    std::error_code ec;
    size_t input_size = fs::file_size(input_rom, ec);

    Elf32 elf(elf_object);
    SymbolMap symbol_map = symbol_map_from_elf(elf);

    g_patcher = std::make_unique<Patcher>(input_rom, input_size, &symbol_map);

    std::vector<uint8_t> blob_bytes = elf.extract_binary();
    uint32_t blob_offset = to_offset(symbol_map["BLOB_BEGIN"]);
    REPLACE(blob_offset, blob_bytes);

    patchbin_main();

    g_patcher->patch(output_rom);

    for (const auto& patch : g_patcher->patches()) {
        std::printf("%.06X", patch.offset);
        for (auto byte : patch.bytes)
            std::printf(" %02hhX", byte);
        std::printf("\n");
    }
}

#endif

