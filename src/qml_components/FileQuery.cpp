//
// Created by PC on 08/10/2023.
//

#include "FileQuery.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QStringList>
#include <QUrl>
#include <QStringDecoder>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iconv.h>
#include <limits>
#include <spdlog/spdlog.h>
#include <vector>
namespace {

auto
fileNameStartsWithDot(const std::filesystem::path& path) -> bool
{
    const auto filename = path.filename();
    if (filename.empty()) {
        return false;
    }

    const auto& native = filename.native();
    return !native.empty() &&
           native.front() ==
             static_cast<std::filesystem::path::value_type>('.');
}

auto
getSelectableFilesForDirectory(const std::filesystem::path& path)
  -> QList<QString>
{
    auto files = QList<QString>{};
    std::error_code ec;
    if (!std::filesystem::is_directory(path, ec)) {
        return files;
    }

    for (auto iterator = std::filesystem::directory_iterator(path, ec);
         iterator != std::filesystem::directory_iterator{};
         iterator.increment(ec)) {
        if (ec) {
            break;
        }
        const auto& file = *iterator;
        if (!file.is_regular_file() && !file.is_directory()) {
            continue;
        }
        if (file.path().extension() == ".ini") {
            continue;
        }
        if (fileNameStartsWithDot(file.path())) {
            continue;
        }
        files.push_back(support::pathToQString(file.path().filename()));
    }
    return files;
}

auto
fontFamilyHasTabularDigits(const QString& family) -> bool
{
    auto font = QFont(family);
    font.setPixelSize(32);
    font.setFeature(QFont::Tag("tnum"), 1);
    const auto metrics = QFontMetricsF(font);

    auto minAdvance = std::numeric_limits<qreal>::max();
    auto maxAdvance = qreal{ 0 };
    for (auto digit = QChar{ u'0' }; digit <= QChar{ u'9' };
         digit = QChar(digit.unicode() + 1)) {
        const auto advance = metrics.horizontalAdvance(QString(digit));
        minAdvance = std::min(minAdvance, advance);
        maxAdvance = std::max(maxAdvance, advance);
    }
    return std::abs(maxAdvance - minAdvance) < 0.01;
}

auto
fontFamilyMatchesConstraints(const QString& family,
                             const bool fixedPitchOnly,
                             const bool tabularDigitsOnly) -> bool
{
    if (fixedPitchOnly && !QFontDatabase::isFixedPitch(family)) {
        return false;
    }
    return !tabularDigitsOnly || fontFamilyHasTabularDigits(family);
}

auto
fontFileMatchesConstraints(const std::filesystem::path& path,
                           const bool fixedPitchOnly,
                           const bool tabularDigitsOnly) -> bool
{
    const auto fontId =
      QFontDatabase::addApplicationFont(support::pathToQString(path));
    if (fontId < 0) {
        return false;
    }
    const auto families = QFontDatabase::applicationFontFamilies(fontId);
    const auto matches =
      std::any_of(families.begin(),
                  families.end(),
                  [fixedPitchOnly, tabularDigitsOnly](const QString& family) {
                      return fontFamilyMatchesConstraints(
                        family, fixedPitchOnly, tabularDigitsOnly);
                  });
    QFontDatabase::removeApplicationFont(fontId);
    return matches;
}

auto
getSelectableFontFilesForDirectory(const std::filesystem::path& path,
                                   const bool fixedPitchOnly,
                                   const bool tabularDigitsOnly)
  -> QList<QString>
{
    auto files = getSelectableFilesForDirectory(path);
    if (!fixedPitchOnly && !tabularDigitsOnly) {
        return files;
    }

    auto matchingFiles = QList<QString>{};
    for (const auto& file : files) {
        if (fontFileMatchesConstraints(path / support::qStringToPath(file),
                                       fixedPitchOnly,
                                       tabularDigitsOnly)) {
            matchingFiles.push_back(file);
        }
    }
    return matchingFiles;
}

auto
getSystemFontFamilies(const bool fixedPitchOnly, const bool tabularDigitsOnly)
  -> QList<QString>
{
    auto families = QFontDatabase::families();
    families.erase(
      std::remove_if(
        families.begin(),
        families.end(),
        [fixedPitchOnly, tabularDigitsOnly](const QString& family) {
            if (QFontDatabase::isPrivateFamily(family)) {
                return true;
            }
            return !fontFamilyMatchesConstraints(
              family, fixedPitchOnly, tabularDigitsOnly);
        }),
      families.end());
    return families;
}

auto
getDefaultSystemFontFamily(const bool fixedPitchOnly,
                           const bool tabularDigitsOnly) -> QString
{
    const auto defaultFamily =
      QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
    if (!defaultFamily.isEmpty() &&
        fontFamilyMatchesConstraints(
          defaultFamily, fixedPitchOnly, tabularDigitsOnly)) {
        return defaultFamily;
    }

    const auto families =
      getSystemFontFamilies(fixedPitchOnly, tabularDigitsOnly);
    if (!families.empty()) {
        return families.first();
    }
    return {};
}

auto
decodeTextFile(const QByteArray& data) -> QString
{
    const auto startsWithBytes = [&data](QByteArrayView prefix) {
        return data.size() >= prefix.size() &&
               QByteArrayView(data.constData(), prefix.size()) == prefix;
    };
    const auto decode = [](QStringConverter::Encoding encoding,
                           QByteArrayView bytes) -> QString {
        QStringDecoder decoder(encoding);
        return decoder.decode(bytes);
    };

    if (startsWithBytes(QByteArrayView("\xEF\xBB\xBF", 3))) {
        return decode(QStringConverter::Utf8, data.sliced(3));
    }
    if (startsWithBytes(QByteArrayView("\x00\x00\xFE\xFF", 4))) {
        return decode(QStringConverter::Utf32BE, data.sliced(4));
    }
    if (startsWithBytes(QByteArrayView("\xFF\xFE\x00\x00", 4))) {
        return decode(QStringConverter::Utf32LE, data.sliced(4));
    }
    if (startsWithBytes(QByteArrayView("\xFE\xFF", 2))) {
        return decode(QStringConverter::Utf16BE, data.sliced(2));
    }
    if (startsWithBytes(QByteArrayView("\xFF\xFE", 2))) {
        return decode(QStringConverter::Utf16LE, data.sliced(2));
    }

    // LR2-era text files commonly omit a BOM and use Japanese Windows CP932.
    // Do not auto-detect UTF-8 for these files; ASCII survives unchanged.
    for (const auto* encoding : { "CP932",
                                  "windows-31j",
                                  "Shift-JIS",
                                  "Shift_JIS",
                                  "SJIS",
                                  "MS_Kanji" }) {
        QStringDecoder decoder(encoding);
        if (!decoder.isValid()) {
            continue;
        }
        const auto decoded = decoder.decode(data);
        if (!decoder.hasError()) {
            return decoded;
        }
    }

    const auto invalidIconv =
      reinterpret_cast<iconv_t>(static_cast<std::intptr_t>(-1));
    for (const auto* encoding :
         { "CP932", "Windows-31J", "SHIFT_JIS", "Shift-JIS" }) {
        iconv_t cd = iconv_open("UTF-8", encoding);
        if (cd == invalidIconv) {
            continue;
        }

        auto* srcPtr = const_cast<char*>(data.constData());
        auto srcLeft = static_cast<size_t>(data.size());
        const auto dstSize = static_cast<size_t>(data.size()) * 4 + 4;
        std::vector<char> dstBuf(dstSize);
        auto* dstPtr = dstBuf.data();
        auto dstLeft = dstSize;

        const auto result = iconv(cd, &srcPtr, &srcLeft, &dstPtr, &dstLeft);
        iconv_close(cd);
        if (result != static_cast<size_t>(-1)) {
            return QString::fromUtf8(dstBuf.data(),
                                     static_cast<qsizetype>(dstSize - dstLeft));
        }
    }

    return QString::fromLatin1(data);
}

auto
localPathFromUserPath(const QString& path) -> QString
{
    const auto url = QUrl(path);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return path;
}
} // namespace

namespace qml_components {
auto
FileQuery::readTextFile(const QString& path) const -> QString
{
    QFile file(localPathFromUserPath(path));
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::warn("Failed to read text file {}", path.toStdString());
        return {};
    }
    return decodeTextFile(file.readAll());
}

auto
FileQuery::getSelectableFilesForDirectory(const QString& directory) const
  -> QList<QString>
{
    try {
        return ::getSelectableFilesForDirectory(
          support::qStringToPath(directory));
    } catch (const std::exception& e) {
        spdlog::error("Error getting selectable files for directory {}: {}",
                      directory.toStdString(),
                      e.what());
        return {};
    }
}

auto
FileQuery::getSelectableFontFilesForDirectory(
  const QString& directory,
  const bool fixedPitchOnly,
  const bool tabularDigitsOnly) const -> QList<QString>
{
    try {
        return ::getSelectableFontFilesForDirectory(
          support::qStringToPath(directory), fixedPitchOnly, tabularDigitsOnly);
    } catch (const std::exception& e) {
        spdlog::error("Error getting selectable font files for directory {}: "
                      "{}",
                      directory.toStdString(),
                      e.what());
        return {};
    }
}

auto
FileQuery::getSystemFontFamilies(const bool fixedPitchOnly,
                                 const bool tabularDigitsOnly) const
  -> QList<QString>
{
    return ::getSystemFontFamilies(fixedPitchOnly, tabularDigitsOnly);
}

auto
FileQuery::getDefaultSystemFontFamily(const bool fixedPitchOnly,
                                      const bool tabularDigitsOnly) const
  -> QString
{
    return ::getDefaultSystemFontFamily(fixedPitchOnly, tabularDigitsOnly);
}
} // namespace qml_components
