#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "common.hpp"

static void log_base(std::FILE* stream, std::string_view category, std::string_view format, std::va_list args)
{
    std::fprintf(stream, "patchbin: [%s] ", category.data());
    std::vfprintf(stream, format.data(), args);
    std::fputc('\n', stream);
}

void log_status(std::string_view format, ...)
{
    std::va_list args;
    va_start(args, format);
    log_base(stdout, "status", format, args);
    va_end(args);
}

void log_fatal(std::string_view format, ...)
{
    std::va_list args;
    va_start(args, format);
    log_base(stderr, "error", format, args);
    va_end(args);
    std::exit(EXIT_FAILURE);
}
