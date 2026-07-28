#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace charts {

class BmsAssetResolver
{
  public:
    struct Entry
    {
        std::string relativeUtf8;
        std::filesystem::path actualPath;
    };

    explicit BmsAssetResolver(std::vector<Entry> entries);

    [[nodiscard]] static auto fromDirectory(
      const std::filesystem::path& chartRoot) -> BmsAssetResolver;
    [[nodiscard]] auto resolve(std::string_view declaredRelativePath) const
      -> std::optional<Entry>;
    [[nodiscard]] auto valid() const noexcept -> bool;
    [[nodiscard]] auto diagnostic() const noexcept -> const std::string&;

    [[nodiscard]] static auto normalizeRelative(std::string_view path)
      -> std::optional<std::string>;

  private:
    std::vector<Entry> indexedEntries;
    std::unordered_map<std::string, std::size_t> exactIndex;
    std::unordered_map<std::string, std::size_t> foldedIndex;
    std::string terminalDiagnostic;
};

} // namespace charts
