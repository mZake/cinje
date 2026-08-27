#pragma once

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <initializer_list>
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

#define LOCATION            Location{__FILE__, __LINE__}

#define PATCH_POINTER(...)  patch_pointer_at(LOCATION, __VA_ARGS__)
#define PATCH_BYTES(...)    patch_bytes_at(LOCATION, __VA_ARGS__)
#define PATCH_HOOK(...)     patch_hook_at(LOCATION, __VA_ARGS__)
#define PATCH_FUNC(...)     patch_function_at(LOCATION, __VA_ARGS__)

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

    void write_byte(uint8_t byte);
    void write_byte(uint8_t byte, size_t count);
    void write_bytes(std::initializer_list<uint8_t> bytes);
    void write_bytes(const uint8_t* bytes, size_t count);

    void write_uint16(uint16_t value);
    void write_uint32(uint32_t value);
    void write_uint64(uint64_t value);

    void write_little_uint16(uint16_t value);
    void write_little_uint32(uint32_t value);
    void write_little_uint64(uint64_t value);
};

// Tiny ELF library for parsing ARM 32-bit LSB executables
namespace elf
{
    #define ELF32_ST_BIND(x) ((x) >> 4)
    #define ELF32_ST_TYPE(x) ((x) & 0xF)

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

    struct Elf32_Object
    {
        Elf32_Ehdr elf_header;
        std::vector<uint8_t> file_data;
        std::vector<Elf32_Shdr> sections;
        std::unordered_map<std::string_view, size_t> section_index_map;
    };

    struct Elf32_SymbolTable
    {
        std::vector<Elf32_Sym> symbols;
        std::unordered_map<std::string_view, size_t> symbol_index_map;
    };

    Elf32_Object read_elf_object(const char* filepath);

    std::string_view get_string(const Elf32_Object& object, size_t section_index, uint32_t offset);

    Elf32_Shdr get_section(const Elf32_Object& object, size_t index);
    Elf32_Shdr get_section(const Elf32_Object& object, std::string_view name);

    Elf32_SymbolTable read_symbol_table(const Elf32_Object& object);

    Elf32_Sym get_symbol(const Elf32_SymbolTable& symbol_table, size_t index);
    Elf32_Sym get_symbol(const Elf32_SymbolTable& symbol_table, std::string_view name);

    uint32_t resolve_symbol(const Elf32_Object& object, const Elf32_Sym& symbol);

    std::vector<uint8_t> read_image_data(const Elf32_Object& object);
}

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

using SymbolAddressMap = std::unordered_map<std::string_view, uint32_t>;

struct Patcher
{
    const char* input_binary = nullptr;
    const char* output_binary = nullptr;
    size_t binary_size = 0;
    SymbolAddressMap symbol_address_map;
    std::vector<Patch> patches;
    bool has_error = false;
};

void begin_patching(const char* input_binary, const char* output_binary, const elf::Elf32_Object& elf);
void end_patching();

void patch_pointer_at(Location location, uint32_t offset, const char* name, bool set_thumb_bit);
void patch_bytes_at(Location location, uint32_t offset, std::vector<uint8_t> bytes);
void patch_hook_at(Location location, uint32_t offset, const char* name, uint8_t register_id);
void patch_function_at(Location location, uint32_t offset, const char* name,
                       uint32_t param_count, uint8_t returns);

#ifdef PATCHBIN_IMPLEMENTATION

static Patcher s_patcher;

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

void BufferBuilder::write_byte(uint8_t byte)
{
    buffer.push_back(byte);
}

void BufferBuilder::write_byte(uint8_t byte, size_t count)
{
    buffer.insert(buffer.end(), count, byte);
}

void BufferBuilder::write_bytes(std::initializer_list<uint8_t> bytes)
{
    buffer.insert(buffer.end(), bytes);
}

void BufferBuilder::write_bytes(const uint8_t* bytes, size_t count)
{
    buffer.insert(buffer.end(), bytes, bytes + count);
}

void BufferBuilder::write_uint16(uint16_t value)
{
    write_bytes(reinterpret_cast<uint8_t*>(&value), sizeof(uint16_t));
}

void BufferBuilder::write_uint32(uint32_t value)
{
    write_bytes(reinterpret_cast<uint8_t*>(&value), sizeof(uint32_t));
}

void BufferBuilder::write_uint64(uint64_t value)
{
    write_bytes(reinterpret_cast<uint8_t*>(&value), sizeof(uint64_t));
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

namespace elf
{
    Elf32_Object read_elf_object(const char* filepath)
    {
        Elf32_Object object;

        object.file_data = read_entire_file(filepath);
        BufferParser parser(object.file_data.data(), object.file_data.size());

        parser.read_bytes(object.elf_header.e_ident, EI_NIDENT);
        if (object.elf_header.e_ident[EI_MAG0] != ELFMAG0 ||
            object.elf_header.e_ident[EI_MAG1] != ELFMAG1 ||
            object.elf_header.e_ident[EI_MAG2] != ELFMAG2 ||
            object.elf_header.e_ident[EI_MAG3] != ELFMAG3)
        {
            log_fatal("file is not an ELF object: %s", filepath);
        }

        if (object.elf_header.e_ident[EI_CLASS] != ELFCLASS32)
        {
            log_fatal("ELF is not 32-bit: %s", filepath);
        }

        if (object.elf_header.e_ident[EI_DATA] != ELFDATA2LSB)
        {
            log_fatal("ELF is not LSB: %s", filepath);
        }

        if (object.elf_header.e_ident[EI_VERSION] != EV_CURRENT)
        {
            log_fatal("ELF version is invalid: %s", filepath);
        }

        parser.read_little_uint16(object.elf_header.e_type);
        parser.read_little_uint16(object.elf_header.e_machine);
        parser.read_little_uint32(object.elf_header.e_version);
        parser.read_little_uint32(object.elf_header.e_entry);
        parser.read_little_uint32(object.elf_header.e_phoff);
        parser.read_little_uint32(object.elf_header.e_shoff);
        parser.read_little_uint32(object.elf_header.e_flags);
        parser.read_little_uint16(object.elf_header.e_ehsize);
        parser.read_little_uint16(object.elf_header.e_phentsize);
        parser.read_little_uint16(object.elf_header.e_phnum);
        parser.read_little_uint16(object.elf_header.e_shentsize);
        parser.read_little_uint16(object.elf_header.e_shnum);
        parser.read_little_uint16(object.elf_header.e_shstrndx);

        if (object.elf_header.e_type != ET_EXEC)
        {
            log_fatal("ELF is not an executable: %s", filepath);
        }

        if (object.elf_header.e_shoff == 0)
        {
            log_fatal("ELF does not contain a section header table: %s", filepath);
        }

        parser.seek(object.elf_header.e_shoff);

        object.sections.resize(object.elf_header.e_shnum);
        for (auto& section : object.sections)
        {
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

        // Map indexes here to ensure the ELF object is in a valid state
        for (size_t i = 0; i < object.sections.size(); ++i)
        {
            const auto& section = object.sections[i];
            auto name = get_string(object, object.elf_header.e_shstrndx, section.sh_name);
            object.section_index_map[name] = i;
        }

        return object;
    }

    std::string_view get_string(const Elf32_Object& object, size_t section_index, uint32_t offset)
    {
        const auto& section = object.sections[section_index];
        auto data = reinterpret_cast<const char*>(object.file_data.data() + section.sh_offset);

        std::string_view result{data + offset};
        return result;
    }

    Elf32_Shdr get_section(const Elf32_Object& object, size_t index)
    {
        return object.sections[index];
    }

    Elf32_Shdr get_section(const Elf32_Object& object, std::string_view name)
    {
        if (auto it = object.section_index_map.find(name);
            it != object.section_index_map.end())
        {
            return get_section(object, it->second);
        }

        return {};
    }

    Elf32_SymbolTable read_symbol_table(const Elf32_Object& object)
    {
        Elf32_SymbolTable symbol_table;

        Elf32_Shdr section = get_section(object, ".symtab");

        BufferParser parser(object.file_data.data(), object.file_data.size());
        parser.seek(section.sh_offset);

        size_t symbol_count = section.sh_size / section.sh_entsize;
        symbol_table.symbols.resize(symbol_count);

        for (auto& symbol : symbol_table.symbols)
        {
            parser.read_little_uint32(symbol.st_name);
            parser.read_little_uint32(symbol.st_value);
            parser.read_little_uint32(symbol.st_size);
            parser.read_byte(symbol.st_info);
            parser.read_byte(symbol.st_other);
            parser.read_little_uint16(symbol.st_shndx);
        }

        for (size_t i = 0; i < symbol_table.symbols.size(); ++i)
        {
            const auto& symbol = symbol_table.symbols[i];
            auto name = get_string(object, section.sh_link, symbol.st_name);
            symbol_table.symbol_index_map[name] = i;
        }

        return symbol_table;
    }

    Elf32_Sym get_symbol(const Elf32_SymbolTable& symbol_table, size_t index)
    {
        return symbol_table.symbols[index];
    }

    Elf32_Sym get_symbol(const Elf32_SymbolTable& symbol_table, std::string_view name)
    {
        if (auto it = symbol_table.symbol_index_map.find(name);
            it != symbol_table.symbol_index_map.end())
        {
            return get_symbol(symbol_table, it->second);
        }

        return {};
    }

    uint32_t resolve_symbol(const Elf32_Object& object, const Elf32_Sym& symbol)
    {
        if (symbol.st_shndx == SHN_UNDEF)
        {
            return 0;
        }

        if (symbol.st_shndx == SHN_ABS)
        {
            return symbol.st_value;
        }

        Elf32_Shdr section = get_section(object, symbol.st_shndx);
        uint32_t result = section.sh_offset + symbol.st_value;
        return result;
    }

    std::vector<uint8_t> read_image_data(const Elf32_Object& object)
    {
        // Collect all sections whose type is SHF_ALLOC
        std::vector<std::pair<Elf32_Shdr, size_t>> alloc_sections;
        for (size_t i = 0; i < object.sections.size(); ++i)
        {
            auto& section = object.sections[i];
            if (section.sh_flags & SHF_ALLOC)
            {
                alloc_sections.emplace_back(section, i);
            }
        }

        // Sort sections by the virtual address (or index if the addresses match) in ascending order
        std::sort(alloc_sections.begin(), alloc_sections.end(), [&](auto& lhs, auto& rhs)
        {
            if (lhs.first.sh_addr == rhs.first.sh_addr)
            {
                return lhs.second < rhs.second;
            }

            return lhs.first.sh_addr < rhs.first.sh_addr;
        });

        BufferBuilder builder;
        for (const auto& [section, index] : alloc_sections)
        {
            // Calculate necessary padding, if it is necessary in the first place
            uint32_t align = section.sh_addralign;
            if (align != 0 && builder.buffer.size() % align != 0)
            {
                uint32_t padding = align - builder.buffer.size() % align;
                builder.write_byte(0x00, padding);
            }

            if (section.sh_type == SHT_PROGBITS)
            {
                const uint8_t* bytes = object.file_data.data() + section.sh_offset;
                builder.write_bytes(bytes, section.sh_size);
            }
            else if (section.sh_type == SHT_NOBITS)
            {
                builder.write_byte(0x00, section.sh_size);
            }
        }

        return builder.buffer;
    }
}

static void patch_error(Location location, const char* format, ...)
{
    std::va_list args;
    va_start(args, format);

    std::fprintf(stderr, "%s:%d: ", location.file, location.line);
    std::vfprintf(stderr, format, args);
    std::fprintf(stderr, "\n");

    va_end(args);
    s_patcher.has_error = true;
}

static std::optional<uint32_t> find_symbol(const SymbolAddressMap& address_map, std::string_view name)
{
    if (auto it = address_map.find(name); it != address_map.end())
    {
        return it->second;
    }

    return std::nullopt;
}

void begin_patching(const char* input_binary, const char* output_binary, const elf::Elf32_Object& elf)
{
    Patcher p;
    p.input_binary = input_binary;
    p.output_binary = output_binary;

    std::error_code ec;
    p.binary_size = fs::file_size(input_binary, ec);

    elf::Elf32_SymbolTable symbol_table = elf::read_symbol_table(elf);

    elf::Elf32_Sym blob_begin = elf::get_symbol(symbol_table, "BLOB_BEGIN");
    uint32_t blob_offset = to_offset(elf::resolve_symbol(elf, blob_begin));

    for (const auto& [name, index] : symbol_table.symbol_index_map)
    {
        elf::Elf32_Sym symbol = elf::get_symbol(symbol_table, index);

        uint32_t st_type = ELF32_ST_TYPE(symbol.st_info);
        if (st_type == elf::STT_SECTION || st_type == elf::STT_FILE)
        {
            continue;
        }

        if (symbol.st_shndx == elf::SHN_UNDEF || symbol.st_shndx == elf::SHN_COMMON)
        {
            continue;
        }

        p.symbol_address_map[name] = elf::resolve_symbol(elf, symbol);
    }

    auto blob_bytes = elf::read_image_data(elf);

    Patch blob_patch{std::move(blob_bytes), blob_offset};
    p.patches.push_back(std::move(blob_patch));

    uint32_t end_offset = blob_offset + blob_bytes.size();
    if (end_offset >= p.binary_size)
    {
        size_t overflow = end_offset - p.binary_size;
        log_fatal("blob overflows binary by 0x%zX bytes", overflow);
    }

    s_patcher = std::move(p);
}

void end_patching()
{
    auto& p = s_patcher;
    if (p.has_error)
    {
        std::exit(EXIT_FAILURE);
    }

    std::error_code ec; // Tag to force use of non-throwing overload
    if (!fs::copy_file(p.input_binary, p.output_binary, fs::copy_options::overwrite_existing, ec))
    {
        log_fatal("cannot copy %s to %s", p.input_binary, p.output_binary);
    }

    std::FILE* stream = std::fopen(p.output_binary, "r+b");
    if (!stream)
    {
        log_fatal("cannot open file: %s", p.output_binary);
    }

    for (const auto& patch : p.patches)
    {
        if (std::fseek(stream, patch.offset, SEEK_SET) != 0)
        {
            log_fatal("failed to seek file: %s", p.output_binary);
        }

        if (std::fwrite(patch.bytes.data(), 1, patch.bytes.size(), stream) != patch.bytes.size())
        {
            log_fatal("failed to write file: %s", p.output_binary);
        }
    }

    s_patcher = {};
}

void patch_pointer_at(Location location, uint32_t offset, const char* name, bool set_thumb_bit)
{
    auto& p = s_patcher;

    auto address = find_symbol(p.symbol_address_map, name);
    if (!address)
    {
        patch_error(location, "symbol not found: %s", name);
        return;
    }
 
    *address = set_thumb_bit ? (*address | 1) : (*address & ~1);

    BufferBuilder builder;
    builder.write_little_uint32(*address);

    uint32_t end_offset = offset + builder.buffer.size();
    if (end_offset >= p.binary_size)
    {
        size_t count = end_offset - p.binary_size;
        patch_error(location, "operation overflows %s by 0x%zX bytes", p.input_binary, count);
        return;
    }

    Patch patch;
    patch.offset = offset;
    patch.bytes = std::move(builder.buffer);
    p.patches.push_back(std::move(patch));
}

void patch_bytes_at(Location location, uint32_t offset, std::vector<uint8_t> bytes)
{
    auto& p = s_patcher;

    uint32_t end_offset = offset + bytes.size();
    if (end_offset >= p.binary_size)
    {
        size_t count = end_offset - p.binary_size;
        patch_error(location, "operation overflows %s by 0x%zX bytes", p.input_binary, count);
        return;
    }

    Patch patch;
    patch.offset = offset;
    patch.bytes = std::move(bytes);
    p.patches.push_back(std::move(patch));
}

void patch_hook_at(Location location, uint32_t offset, const char* name, uint8_t register_id)
{
    auto& p = s_patcher;

    auto address = find_symbol(p.symbol_address_map, name);
    if (!address)
    {
        patch_error(location, "symbol not found: %s", name);
        return;
    }

    if (register_id > 12)
    {
        patch_error(location, "register R%u is not usable", register_id);
        return;
    }

    uint8_t register_bits = register_id & 7;

    BufferBuilder builder;
    if (*address % 4)
    {
        builder.write_byte(0x01);
        builder.write_byte(0x48 | register_bits);
        builder.write_byte(0x00 | (register_bits << 3));
        builder.write_bytes({0x47, 0x00, 0x00});
    } 
    else
    {
        builder.write_byte(0x00);
        builder.write_byte(0x48 | register_bits);
        builder.write_byte(0x00 | (register_bits << 3));
        builder.write_byte(0x47);
    }
    builder.write_little_uint32(*address | 1);

    uint32_t end_offset = offset + builder.buffer.size();
    if (end_offset >= p.binary_size)
    {
        size_t count = end_offset - p.binary_size;
        patch_error(location, "operation overflows %s by 0x%zX bytes", p.input_binary, count);
        return;
    }

    Patch patch;
    patch.bytes = std::move(builder.buffer);
    patch.offset = offset & ~1; // Ensure offset alignment is 2
    p.patches.push_back(std::move(patch));
}

void patch_function_at(Location location, uint32_t offset, const char* name,
                       uint32_t param_count, uint8_t returns)
{
    auto& p = s_patcher;

    auto address = find_symbol(p.symbol_address_map, name);
    if (!address)
    {
        patch_error(location, "symbol not found: %s", name);
        return;
    }

    if (param_count > 4)
    {
        patch_error(location, "cannot rewrite function with more than 4 parameters");
        return;
    }

    BufferBuilder builder;
    builder.write_bytes({0x10, 0xB5, 0x03, 0x4C, 0x00, 0xF0, 0x03, 0xF8, 0x10, 0xBC});
    builder.write_byte(returns + 1);
    builder.write_byte(0xBC);
    builder.write_byte(returns << 3);
    builder.write_bytes({0x47, 0x20, 0x47});
    builder.write_little_uint32(*address | 1);

    uint32_t end_offset = offset + builder.buffer.size();
    if (end_offset >= p.binary_size)
    {
        size_t count = end_offset - p.binary_size;
        patch_error(location, "operation overflows %s by 0x%zX bytes", p.input_binary, count);
        return;
    }

    Patch patch;
    patch.offset = offset;
    patch.bytes = std::move(builder.buffer);
    p.patches.push_back(std::move(patch));
}

extern void patchbin_main();

struct ParsedArgs
{
    const char* input_binary_path = nullptr;
    const char* elf_object_path = nullptr;
    const char* output_binary_path = nullptr;
};

static ParsedArgs parse_args(int argc, char** argv)
{
    if (argc != 4)
    {
        log_fatal_no_prefix("Usage: patchbin input-binary elf-object output-binary");
    }

    // Skip executable path argument.
    ++argv;

    ParsedArgs args;
    args.input_binary_path = *argv++;
    args.elf_object_path = *argv++;
    args.output_binary_path = *argv++;
    return args;
}

int main(int argc, char** argv)
{
    ParsedArgs args = parse_args(argc, argv);

    elf::Elf32_Object elf = elf::read_elf_object(args.elf_object_path);

    begin_patching(args.input_binary_path, args.output_binary_path, elf);
    patchbin_main();
    end_patching();
}

#endif

