#ifndef PARSER_HPP_
#define PARSER_HPP_

#include <string_view>
#include <vector>

#include <stdint.h>

struct PointerPatch
{
    uint32_t offset;
    std::string_view symbol;
};

struct RewritePatch
{
    uint32_t offset;
    uint32_t num_args;
    bool returns;
    std::string_view symbol;
};

struct HookPatch
{
    uint32_t offset;
    uint8_t register_id;
    std::string_view symbol;
};

struct PatchJobs
{
    std::vector<PointerPatch> pointers;
    std::vector<RewritePatch> rewrites;
    std::vector<HookPatch> hooks;
};

PatchJobs load_patch_file(std::string_view file);

#endif // PARSER_HPP_
