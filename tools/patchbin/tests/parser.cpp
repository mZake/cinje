#include "script.hpp"

static const char* s_source = R"(
    import sym "test1.sym"
    import sym "test2.sym"
)";

int main()
{
    auto tokens = tokenize_script(s_source);
    parse_script(tokens);
}

