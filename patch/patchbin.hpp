#pragma once

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <string>
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
    #define HOST_TO_LITTLE16(x) htole16(x)
    #define HOST_TO_LITTLE32(x) htole32(x)
#elif defined(_WIN32) // Windows is always little-endian
    #define LITTLE_TO_HOST16(x) (x)
    #define LITTLE_TO_HOST32(x) (x)
    #define HOST_TO_LITTLE16(x) (x)
    #define HOST_TO_LITTLE32(x) (x)
#endif

#define LOCATION    Location{__FILE__, __LINE__}
#define VECTOR(T)   std::vector<T>

#define REPOINT(symbol, offset, set_thumb_bit) \
    push_patch_command<RepointCommand>(LOCATION, symbol, offset, set_thumb_bit)

#define REPLACE(offset, ...) \
    push_patch_command<ReplaceCommand>(LOCATION, offset, VECTOR(uint8_t)(__VA_ARGS__))

#define HOOK(symbol, offset, register_id) \
    push_patch_command<HookCommand>(LOCATION, symbol, offset, register_id)

namespace fs = std::filesystem;

inline void log_fatal(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);

    std::fprintf(stderr, "patchbin: error: ");
    std::vfprintf(stderr, format, args);
    std::fprintf(stderr, "\n");

    va_end(args);
    std::exit(EXIT_FAILURE);
}

inline void log_debug(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);

    std::vfprintf(stderr, format, args);
    std::fprintf(stderr, "\n");

    va_end(args);
}

template<typename T>
bool stream_read_array(std::FILE* stream, size_t count, T* output)
{
    size_t read_count = std::fread(output, sizeof(T), count, stream);
    if (read_count != count || std::feof(stream) || std::ferror(stream))
        log_fatal("cannot read stream");
    return true;
}

template<typename T>
bool stream_read_object(std::FILE* stream, T* output)
{
    return stream_read_array(stream, 1, output);
}

inline bool stream_read_object_little(std::FILE* stream, uint16_t* output)
{
    bool result = stream_read_object(stream, output);
    *output = LITTLE_TO_HOST16(*output);
    return result;
}

inline bool stream_read_object_little(std::FILE* stream, uint32_t* output)
{
    bool result = stream_read_object(stream, output);
    *output = LITTLE_TO_HOST32(*output);
    return result;
}

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

struct ELFData
{
    Elf32_Ehdr header;
    std::vector<Elf32_Shdr> sections;
    std::vector<Elf32_Sym> symbols;

    size_t shstrtab_index = 0;
    size_t strtab_index = 0;

    std::vector<uint8_t> payload;
};

inline ELFData elf_from_file(const char* filepath)
{
    ELFData elf;

    std::FILE* stream = std::fopen(filepath, "rb");
    if (!stream)
        log_fatal("cannot open file: %s", filepath);

    stream_read_array(stream, EI_NIDENT, elf.header.e_ident);

    if (elf.header.e_ident[EI_MAG0] != ELFMAG0 || elf.header.e_ident[EI_MAG1] != ELFMAG1 ||
        elf.header.e_ident[EI_MAG2] != ELFMAG2 || elf.header.e_ident[EI_MAG3] != ELFMAG3)
        log_fatal("file is not an ELF object: %s", filepath);

    if (elf.header.e_ident[EI_CLASS] != ELFCLASS32)
        log_fatal("ELF is not 32-bit: %s", filepath);

    if (elf.header.e_ident[EI_DATA] != ELFDATA2LSB)
        log_fatal("ELF is not LSB: %s", filepath);

    if (elf.header.e_ident[EI_VERSION] != EV_CURRENT)
        log_fatal("ELF version is invalid: %s", filepath);

    stream_read_object_little(stream, &elf.header.e_type);
    stream_read_object_little(stream, &elf.header.e_machine);
    stream_read_object_little(stream, &elf.header.e_version);
    stream_read_object_little(stream, &elf.header.e_entry);
    stream_read_object_little(stream, &elf.header.e_phoff);
    stream_read_object_little(stream, &elf.header.e_shoff);
    stream_read_object_little(stream, &elf.header.e_flags);
    stream_read_object_little(stream, &elf.header.e_ehsize);
    stream_read_object_little(stream, &elf.header.e_phentsize);
    stream_read_object_little(stream, &elf.header.e_phnum);
    stream_read_object_little(stream, &elf.header.e_shentsize);
    stream_read_object_little(stream, &elf.header.e_shnum);
    stream_read_object_little(stream, &elf.header.e_shstrndx);

    if (elf.header.e_type != ET_REL)
        log_fatal("ELF is not a relocatable object: %s", filepath);

    if (elf.header.e_shoff == 0)
        log_fatal("ELF does not contain a section header table: %s", filepath);

    std::fseek(stream, elf.header.e_shoff, SEEK_SET);
    elf.sections.resize(elf.header.e_shnum);
    for (auto& section : elf.sections) {
        stream_read_object_little(stream, &section.sh_name);
        stream_read_object_little(stream, &section.sh_type);
        stream_read_object_little(stream, &section.sh_flags);
        stream_read_object_little(stream, &section.sh_addr);
        stream_read_object_little(stream, &section.sh_offset);
        stream_read_object_little(stream, &section.sh_size);
        stream_read_object_little(stream, &section.sh_link);
        stream_read_object_little(stream, &section.sh_info);
        stream_read_object_little(stream, &section.sh_addralign);
        stream_read_object_little(stream, &section.sh_entsize);
    }

    size_t symtab_index = 0;
    for (size_t index = 0; index < elf.sections.size(); ++index) {
        auto& section = elf.sections[index];
        if (section.sh_type == SHT_SYMTAB) {
            symtab_index = index;
            break;
        }
    }

    auto& symtab_section = elf.sections[symtab_index];
    size_t symtab_count = symtab_section.sh_size / symtab_section.sh_entsize;

    std::fseek(stream, symtab_section.sh_offset, SEEK_SET);
    elf.symbols.resize(symtab_count);
    for (auto& symbol : elf.symbols) {
        stream_read_object_little(stream, &symbol.st_name);
        stream_read_object_little(stream, &symbol.st_value);
        stream_read_object_little(stream, &symbol.st_size);
        stream_read_object(stream, &symbol.st_info);
        stream_read_object(stream, &symbol.st_other);
        stream_read_object_little(stream, &symbol.st_shndx);
    }

    elf.shstrtab_index = elf.header.e_shstrndx;
    elf.strtab_index = symtab_section.sh_link;

    std::fseek(stream, 0, SEEK_END);
    long size = std::ftell(stream);
    std::fseek(stream, 0, SEEK_SET);

    elf.payload.resize(size);
    stream_read_array(stream, elf.payload.size(), elf.payload.data());
    std::fclose(stream);

    return elf;
}

inline std::vector<uint8_t> elf_get_raw_binary(const ELFData& elf)
{
    // Collect all sections whose type is SHF_ALLOC
    std::vector<std::pair<Elf32_Shdr, size_t>> alloc_sections;
    for (size_t index = 0; index < elf.sections.size(); ++index) {
        auto& section = elf.sections[index];
        if (section.sh_flags & SHF_ALLOC)
            alloc_sections.emplace_back(section, index);
    }

    // Sort sections by the virtual address (or index if the addresses match) in ascending order
    std::sort(alloc_sections.begin(), alloc_sections.end(), [&](auto& lhs, auto& rhs) {
        if (lhs.first.sh_addr == rhs.first.sh_addr)
            return lhs.second < rhs.second;
        return lhs.first.sh_addr < rhs.first.sh_addr;
    });

    std::vector<uint8_t> payload;
    for (const auto& [section, index] : alloc_sections) {
        // Calculate necessary padding, if it is necessary in the first place
        uint32_t align = section.sh_addralign;
        if (align > 1 && payload.size() % align != 0) {
            uint32_t padding = align - payload.size() % align;
            payload.insert(payload.end(), padding, 0);
        }

        if (section.sh_type == SHT_PROGBITS) {
            const uint8_t* buffer_begin = elf.payload.data() + section.sh_offset;
            const uint8_t* buffer_end = buffer_begin + section.sh_size;
            payload.insert(payload.end(), buffer_begin, buffer_end);
        } else if (section.sh_type == SHT_NOBITS) {
            payload.insert(payload.end(), section.sh_size, 0);
        }
    }

    return payload;
}

inline const char* elf_get_section_name(const ELFData& elf, Elf32_Word sh_name)
{
    Elf32_Shdr shstrtab_section = elf.sections[elf.shstrtab_index];
    uint32_t offset = shstrtab_section.sh_offset + sh_name;
    return reinterpret_cast<const char*>(elf.payload.data() + offset);
}

inline const char* elf_get_symbol_name(const ELFData& elf, Elf32_Word st_name)
{
    Elf32_Shdr strtab_section = elf.sections[elf.strtab_index];
    uint32_t offset = strtab_section.sh_offset + st_name;
    return reinterpret_cast<const char*>(elf.payload.data() + offset);
}

// Return a table that maps symbol names to their final addresses,
// filtering irrelevant symbols in the process.
inline std::unordered_map<std::string, uint32_t> elf_get_symbol_table(const ELFData& elf)
{
    std::unordered_map<std::string, uint32_t> symbol_table;

    for (const auto& symbol : elf.symbols) {
        int st_type = ELF32_ST_TYPE(symbol.st_info);
        if (st_type == STT_SECTION || st_type == STT_FILE)
            continue;

        // Filter symbols in irrelevant sections
        if (symbol.st_shndx == SHN_UNDEF || symbol.st_shndx == SHN_COMMON)
            continue;

        uint32_t address = 0;
        if (symbol.st_shndx == SHN_ABS) {
            address = symbol.st_value;
        } else {
            Elf32_Shdr section = elf.sections[symbol.st_shndx];
            address = section.sh_addr + symbol.st_value;
        }

        std::string name = elf_get_symbol_name(elf, symbol.st_name);
        symbol_table[name] = address;
    }

    return symbol_table;
}

template<typename T>
void bytes_push(std::vector<uint8_t>& bytes, T object)
{
    auto data_begin = reinterpret_cast<uint8_t*>(&object);
    auto data_end = data_begin + sizeof(T);
    bytes.insert(bytes.end(), data_begin, data_end);
}

inline void bytes_push_little(std::vector<uint8_t>& bytes, uint16_t value)
{
    value = HOST_TO_LITTLE16(value);
    bytes_push(bytes, value);
}

inline void bytes_push_little(std::vector<uint8_t>& bytes, uint32_t value)
{
    value = HOST_TO_LITTLE32(value);
    bytes_push(bytes, value);
}

inline void bytes_push_list(std::vector<uint8_t>& bytes, std::initializer_list<uint8_t> list)
{
    bytes.insert(bytes.end(), list);
}

/////////////////////////

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

using SymbolTable = std::unordered_map<std::string, uint32_t>;

inline uint32_t require_symbol(const SymbolTable& sym_table, const char* name)
{
    auto it = sym_table.find(name);
    if (it == sym_table.end())
        log_fatal("symbol not found: %s", name);

    uint32_t address = it->second;
    return address;
}

class PatchCommand
{
public:
    virtual ~PatchCommand() = default;

    virtual Patch generate(const SymbolTable& sym_table) const = 0;
};

class RepointCommand : public PatchCommand
{
public:
    static constexpr int RepointSize = 4;

    RepointCommand(Location location, const char* symbol, uint32_t offset, bool set_thumb_bit)
        : m_location(location), m_symbol(symbol), m_offset(offset), m_set_thumb_bit(set_thumb_bit) {}

    inline Patch generate(const SymbolTable& sym_table) const override
    {
        uint32_t address = require_symbol(sym_table, m_symbol);
        address = m_set_thumb_bit ? (address | 1) : (address & ~1);

        std::vector<uint8_t> bytes;
        bytes.reserve(RepointSize);
        bytes_push_little(bytes, address);

        Patch patch;
        patch.offset = m_offset;
        patch.bytes = std::move(bytes);
        return patch;
    }

private:
    Location m_location;
    const char* m_symbol = nullptr;
    uint32_t m_offset = 0;
    bool m_set_thumb_bit = false;
};

class ReplaceCommand : public PatchCommand
{
public:
    ReplaceCommand(Location location, uint32_t offset, std::vector<uint8_t> bytes)
        : m_location(location), m_offset(offset), m_bytes(std::move(bytes)) {}

    inline Patch generate(const SymbolTable&) const override
    {
        Patch patch;
        patch.bytes = m_bytes;
        patch.offset = m_offset;
        return patch;
    }

private:
    Location m_location;
    uint32_t m_offset = 0;
    std::vector<uint8_t> m_bytes;
};

class HookCommand : public PatchCommand
{
public:
    static constexpr int HookSize = 10;

    HookCommand(Location location, const char* symbol, uint32_t offset, uint8_t register_id)
        : m_location(location), m_symbol(symbol), m_offset(offset), m_register_id(register_id) {}

    inline Patch generate(const SymbolTable& sym_table) const override
    {
        uint8_t register_bits = m_register_id & 7;

        std::vector<uint8_t> bytes;
        bytes.reserve(HookSize);

        uint32_t address = require_symbol(sym_table, m_symbol);
        if (address % 4) {
            bytes_push<uint8_t>(bytes, 0x01);
            bytes_push<uint8_t>(bytes, 0x48 | register_bits);
            bytes_push<uint8_t>(bytes, 0x00 | (register_bits << 3));
            bytes_push_list(bytes, {0x47, 0x00, 0x00});
        } else {
            bytes_push<uint8_t>(bytes, 0x00);
            bytes_push<uint8_t>(bytes, 0x48 | register_bits);
            bytes_push<uint8_t>(bytes, 0x00 | (register_bits << 3));
            bytes_push<uint8_t>(bytes, 0x47);
        }

        bytes_push_little(bytes, address | 1);

        Patch patch;
        patch.bytes = std::move(bytes);
        patch.offset = m_offset & ~1; // Ensure offset alignment is 2
        return patch;
    }

private:
    Location m_location;
    const char* m_symbol = nullptr;
    uint32_t m_offset = 0;
    uint8_t m_register_id = 0;
};

using CommandList = std::vector<std::unique_ptr<PatchCommand>>;

inline CommandList g_commands;

template<typename T, typename... TArgs>
void push_patch_command(TArgs&&... args)
{
    g_commands.push_back(std::make_unique<T>(std::forward<TArgs>(args)...));
}

inline uint32_t to_offset(uint32_t address)
{
    return address - 0x8000000;
}

inline std::vector<Patch> generate_patches(const ELFData& elf, const CommandList& cmds)
{
    auto sym_table = elf_get_symbol_table(elf);

    std::vector<Patch> patches(1);
    patches[0].bytes = elf_get_raw_binary(elf);
    patches[0].offset = to_offset(require_symbol(sym_table, "BLOB_BEGIN"));

    for (const auto& cmd : cmds) {
        Patch patch = cmd->generate(sym_table);
        patches.push_back(std::move(patch));
    }

    return patches;
}

inline void apply_patches(const char* input, const char* output, const std::vector<Patch>& patches)
{
    std::error_code ec; // Tag to force use of non-throwing overload
    if (!fs::copy_file(input, output, fs::copy_options::overwrite_existing, ec))
        log_fatal("cannot perform copy operation: from %s to %s", input, output);

    std::FILE* stream = std::fopen(output, "w+b");
    if (!stream)
        log_fatal("cannot open file: %s", output);

    for (const auto& patch : patches) {
        std::fseek(stream, patch.offset, SEEK_SET);
        std::fwrite(patch.bytes.data(), 1, patch.bytes.size(), stream);
    }
}

inline void print_section(const ELFData& elf, const Elf32_Shdr& section)
{
    log_debug("[%s]", elf_get_section_name(elf, section.sh_name));
    log_debug("  .sh_name = %u", section.sh_name);
    log_debug("  .sh_type = %u", section.sh_type);
    log_debug("  .sh_flags = 0x%X", section.sh_flags);
    log_debug("  .sh_addr = 0x%.08X", section.sh_addr);
    log_debug("  .sh_offset = 0x%.08X", section.sh_offset);
    log_debug("  .sh_size = %u", section.sh_size);
    log_debug("  .sh_link = %u", section.sh_link);
    log_debug("  .sh_info = %u", section.sh_info);
    log_debug("  .sh_addralign = %u", section.sh_addralign);
    log_debug("  .sh_entsize = %u", section.sh_entsize);
}

inline void print_symbol(const ELFData& elf, const Elf32_Sym& symbol)
{
    log_debug("[%s]", elf_get_symbol_name(elf, symbol.st_name));
    log_debug("  .st_name = %u", symbol.st_name);
    log_debug("  .st_value = 0x%.08X", symbol.st_value);
    log_debug("  .st_size = %u", symbol.st_size);
    log_debug("  .st_info = %hhX", symbol.st_info);
    log_debug("  .st_other = %hhX", symbol.st_other);
    log_debug("  .st_shndx = %hu", symbol.st_shndx);
}

inline void patch(int argc, char** argv)
{
    if (argc != 4) {
        std::printf("Usage: patchbin INPUT_ROM ELF_OBJECT OUTPUT_ROM\n");
        std::exit(EXIT_FAILURE);
    }

    const char* input_rom = argv[1];
    const char* elf_object = argv[2];
    const char* output_rom = argv[3];

    ELFData elf = elf_from_file(elf_object);
    auto patches = generate_patches(elf, g_commands);
    apply_patches(input_rom, output_rom, patches);

    for (const auto& patch : patches) {
        std::printf("%.06X", patch.offset);
        for (auto byte : patch.bytes)
            std::printf(" %02hhX", byte);
        std::printf("\n");
    }
}

