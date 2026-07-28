#pragma once

#include <string_view>

namespace web_playtest {

[[nodiscard]] auto
buildInputSha256() noexcept -> std::string_view;

}
