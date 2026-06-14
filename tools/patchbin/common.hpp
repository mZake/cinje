#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <string_view>

#define SV_ARG(x) static_cast<int>((x).size()), (x).data()

void log_status(std::string_view format, ...);
void log_fatal(std::string_view format, ...);

#endif // COMMON_HPP_
