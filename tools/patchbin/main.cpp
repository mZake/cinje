#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <span>
#include <string_view>
#include <vector>

#include <stdint.h>

#include "common.hpp"
#include "parser.hpp"

namespace fs = std::filesystem;

static uint32_t string_to_uint32(std::string_view string)
{
    uint32_t value = 0;
    std::from_chars_result result;

    if (string.starts_with("0x"))
        result = std::from_chars(string.begin() + 2, string.end(), value, 16);
    else
        result = std::from_chars(string.begin(), string.end(), value);

    if (result.ec == std::errc::invalid_argument)
        log_fatal("%s is not a valid integer representation", string.data());
    if (result.ec == std::errc::result_out_of_range)
        log_fatal("%s cannot be stored in a 32-bit unsigned integer", string.data());

    return value;
}

static std::vector<char> read_entire_file(std::string_view file_path)
{
    std::ifstream stream{file_path.data(), std::ios::binary};
    if (!stream.is_open())
        log_fatal("cannot open %s for reading", file_path.data());

    size_t file_size = fs::file_size(file_path);
    if (file_size == 0)
        log_fatal("file %s is empty", file_path.data());

    std::vector<char> buffer;
    buffer.resize(file_size);
    if (!stream.read(buffer.data(), file_size))
        log_fatal("cannot read %s", file_path.data());

    return buffer;
}

static void patch_buffer_into_rom(std::string_view rom_path, std::span<char> buffer, size_t offset)
{
    std::fstream rom_stream{rom_path.data(), std::ios::in | std::ios::out | std::ios::binary};
    if (!rom_stream.is_open())
        log_fatal("cannot open %s for writing", rom_path.data());

    size_t rom_size = fs::file_size(rom_path);
    if (rom_size < offset)
    {
        size_t overflow_size = offset - rom_size;
        log_fatal("offset is out of ROM bounds (overflowed by %zu bytes)", overflow_size);
    }

    size_t available_space = rom_size - offset;
    if (available_space < buffer.size())
    {
        size_t overflow_size = buffer.size() - available_space;
        log_fatal("ROM does not have enough space (overflowed by %zu bytes)", overflow_size);
    }

    rom_stream.seekp(offset, std::ios::beg);
    if (!rom_stream.write(buffer.data(), buffer.size()))
        log_fatal("cannot write to %s", rom_path.data());
}

static void copy_file(std::string_view src_path, std::string_view dest_path)
{
    std::error_code code; // force use of non-throwing overload
    if (!fs::copy_file(src_path, dest_path, fs::copy_options::overwrite_existing, code))
        log_fatal("cannot copy %s to %s", src_path.data(), dest_path.data());
}

static void print_usage()
{
    std::fprintf(stderr, "Usage: patchbin BASE_ROM OUT_ROM BIN_FILE OFFSET\n");
    std::exit(EXIT_SUCCESS);
}

int main(int argc, char** argv)
{
    if (argc == 1)
        print_usage();

    if (argc != 5)
        log_fatal("expected 4 arguments but %d were given instead", argc - 1);

    std::string_view base_rom_path = argv[1];
    std::string_view out_rom_path = argv[2];
    std::string_view bin_blob_path = argv[3];
    std::string_view offset_string = argv[4];

    copy_file(base_rom_path, out_rom_path);
    std::vector<char> blob_buffer = read_entire_file(bin_blob_path);
    uint32_t offset = string_to_uint32(offset_string);

    log_status("patching %s into %s at offset 0x%x", bin_blob_path.data(), out_rom_path.data(), offset);
    patch_buffer_into_rom(out_rom_path, blob_buffer, offset);

    log_status("operation completed with success");
}

