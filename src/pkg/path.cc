#include <fak/pkg/path.h>

#include <fak/log.h>

std::string_view fk::pkg::parse_file_name_from_full_path(const std::string_view& path) noexcept
{
        const size_t ptr = path.find_last_of('/');

        FK_VERIFY(ptr != std::string_view::npos);                

        return path.substr(ptr + 1);
}