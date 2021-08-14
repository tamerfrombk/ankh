#include <ankh/pkg/path.h>

#include <ankh/log.h>

std::string_view ankh::pkg::parse_file_name_from_full_path(const std::string_view& path) noexcept
{
        const size_t ptr = path.find_last_of('/');

        ankh_VERIFY(ptr != std::string_view::npos);                

        return path.substr(ptr + 1);
}