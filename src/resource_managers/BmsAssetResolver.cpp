#include "BmsAssetResolver.h"

#include <QByteArray>
#include <QString>

#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>

namespace charts {
namespace {
auto
isValidUtf8(std::string_view input) noexcept -> bool
{
    auto remaining = 0;
    auto minimum = std::uint32_t{};
    auto codepoint = std::uint32_t{};
    for (const auto raw : input) {
        const auto byte = static_cast<unsigned char>(raw);
        if (remaining == 0) {
            if (byte <= 0x7f) {
                continue;
            }
            if ((byte & 0xe0) == 0xc0) {
                remaining = 1;
                codepoint = byte & 0x1f;
                minimum = 0x80;
            } else if ((byte & 0xf0) == 0xe0) {
                remaining = 2;
                codepoint = byte & 0x0f;
                minimum = 0x800;
            } else if ((byte & 0xf8) == 0xf0) {
                remaining = 3;
                codepoint = byte & 0x07;
                minimum = 0x10000;
            } else {
                return false;
            }
        } else {
            if ((byte & 0xc0) != 0x80) {
                return false;
            }
            codepoint = (codepoint << 6) | (byte & 0x3f);
            --remaining;
            if (remaining == 0 &&
                (codepoint < minimum || codepoint > 0x10ffff ||
                 (codepoint >= 0xd800 && codepoint <= 0xdfff))) {
                return false;
            }
        }
    }
    return remaining == 0;
}

auto
fold(std::string_view utf8) -> std::string
{
    const auto value =
      QString::fromUtf8(utf8.data(), static_cast<qsizetype>(utf8.size()))
        .normalized(QString::NormalizationForm_C)
        .toCaseFolded()
        .normalized(QString::NormalizationForm_C)
        .toUtf8();
    return { value.constData(), static_cast<std::size_t>(value.size()) };
}

auto
withExtension(std::string path, std::string_view extension) -> std::string
{
    const auto slash = path.find_last_of('/');
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos ||
        (slash != std::string::npos && dot < slash)) {
        path.append(extension);
    } else {
        path.replace(dot, std::string::npos, extension);
    }
    return path;
}

auto
utf8Path(const std::filesystem::path& path) -> std::string
{
    const auto value = path.generic_u8string();
    return { reinterpret_cast<const char*>(value.data()), value.size() };
}
} // namespace

BmsAssetResolver::BmsAssetResolver(std::vector<Entry> entries)
{
    auto normalizedEntries = std::vector<Entry>{};
    normalizedEntries.reserve(entries.size());
    for (auto& entry : entries) {
        const auto normalized = normalizeRelative(entry.relativeUtf8);
        if (!normalized) {
            terminalDiagnostic =
              "Invalid indexed relative path: " + entry.relativeUtf8;
            return;
        }
        entry.relativeUtf8 = *normalized;
        normalizedEntries.push_back(std::move(entry));
    }
    std::ranges::sort(normalizedEntries, {}, &Entry::relativeUtf8);

    auto foldedCandidates = std::map<std::string, std::vector<std::string>>{};
    for (const auto& entry : normalizedEntries) {
        foldedCandidates[fold(entry.relativeUtf8)].push_back(
          entry.relativeUtf8);
    }
    for (auto& [unused, candidates] : foldedCandidates) {
        std::ranges::sort(candidates);
        candidates.erase(std::unique(candidates.begin(), candidates.end()),
                         candidates.end());
        if (candidates.size() > 1) {
            terminalDiagnostic = "Unicode case-fold collision: ";
            for (std::size_t i = 0; i < candidates.size(); ++i) {
                if (i != 0) {
                    terminalDiagnostic += ", ";
                }
                terminalDiagnostic += candidates[i];
            }
            return;
        }
    }

    indexedEntries = std::move(normalizedEntries);
    exactIndex.reserve(indexedEntries.size());
    foldedIndex.reserve(indexedEntries.size());
    for (std::size_t i = 0; i < indexedEntries.size(); ++i) {
        exactIndex.emplace(indexedEntries[i].relativeUtf8, i);
        foldedIndex.emplace(fold(indexedEntries[i].relativeUtf8), i);
    }
}

auto
BmsAssetResolver::fromDirectory(const std::filesystem::path& chartRoot)
  -> BmsAssetResolver
{
    auto entries = std::vector<Entry>{};
    std::error_code error;
    auto iterator =
      std::filesystem::recursive_directory_iterator(chartRoot, error);
    if (error) {
        auto resolver = BmsAssetResolver{ std::move(entries) };
        resolver.terminalDiagnostic = "BMS asset directory scan failed";
        return resolver;
    }
    const auto end = std::filesystem::recursive_directory_iterator{};
    while (iterator != end) {
        if (iterator->is_regular_file(error)) {
            if (error) {
                break;
            }
            const auto relative =
              std::filesystem::relative(iterator->path(), chartRoot, error);
            if (error) {
                break;
            }
            entries.push_back({ .relativeUtf8 = utf8Path(relative),
                                .actualPath = iterator->path() });
        } else if (error) {
            break;
        }
        iterator.increment(error);
        if (error) {
            break;
        }
    }
    auto resolver = BmsAssetResolver{ std::move(entries) };
    if (error) {
        resolver.terminalDiagnostic = "BMS asset directory scan failed";
    }
    return resolver;
}

auto
BmsAssetResolver::resolve(std::string_view declaredRelativePath) const
  -> std::optional<Entry>
{
    if (!valid()) {
        return std::nullopt;
    }
    const auto declared = normalizeRelative(declaredRelativePath);
    if (!declared) {
        return std::nullopt;
    }
    auto candidates = std::vector<std::string>{ *declared };
    for (const auto extension : { ".wav", ".flac", ".ogg", ".mp3" }) {
        const auto candidate = withExtension(*declared, extension);
        if (std::ranges::find(candidates, candidate) == candidates.end()) {
            candidates.push_back(candidate);
        }
    }
    for (const auto& candidate : candidates) {
        if (const auto exact = exactIndex.find(candidate);
            exact != exactIndex.end()) {
            return indexedEntries[exact->second];
        }
        if (const auto folded = foldedIndex.find(fold(candidate));
            folded != foldedIndex.end()) {
            return indexedEntries[folded->second];
        }
    }
    return std::nullopt;
}

auto
BmsAssetResolver::valid() const noexcept -> bool
{
    return terminalDiagnostic.empty();
}
auto
BmsAssetResolver::diagnostic() const noexcept -> const std::string&
{
    return terminalDiagnostic;
}

auto
BmsAssetResolver::normalizeRelative(std::string_view path)
  -> std::optional<std::string>
{
    if (path.empty() || path.find('\0') != std::string_view::npos ||
        !isValidUtf8(path)) {
        return std::nullopt;
    }
    auto value = std::string(path);
    std::ranges::replace(value, '\\', '/');
    if (value.starts_with('/') ||
        (value.size() >= 2 &&
         std::isalpha(static_cast<unsigned char>(value[0])) &&
         value[1] == ':')) {
        return std::nullopt;
    }
    auto components = std::vector<std::string>{};
    auto start = std::size_t{};
    while (start <= value.size()) {
        const auto end = value.find('/', start);
        const auto component = value.substr(
          start, end == std::string::npos ? std::string::npos : end - start);
        if (!component.empty() && component != ".") {
            if (component == "..") {
                if (components.empty()) {
                    return std::nullopt;
                }
                components.pop_back();
            } else {
                components.push_back(component);
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    if (components.empty()) {
        return std::nullopt;
    }
    auto normalized = components.front();
    for (std::size_t i = 1; i < components.size(); ++i) {
        normalized += '/';
        normalized += components[i];
    }
    return normalized;
}

} // namespace charts
