//
// Created by PC on 08/10/2023.
//

#include "FileQuery.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"
#include <QFileInfo>
#include <QStringDecoder>
#include <cstdint>
#include <iconv.h>
#include <spdlog/spdlog.h>
#include <vector>
namespace {

auto
getSelectableFilesForDirectory(const std::filesystem::path& path)
  -> QList<QString>
{
    auto files = QList<QString>{};
    std::error_code ec;
    if (!std::filesystem::is_directory(path, ec)) {
        return files;
    }

    for (const auto& file : std::filesystem::directory_iterator(path, ec)) {
        if (ec) {
            break;
        }
        if (!file.is_regular_file() && !file.is_directory()) {
            continue;
        }
        if (file.path().extension() == ".ini") {
            continue;
        }
        // ignore files starting with dot
        if (file.path().filename().string().front() == '.') {
            continue;
        }
        files.push_back(support::pathToQString(file.path().filename()));
    }
    return files;
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
    for (const auto* encoding :
         { "CP932", "windows-31j", "Shift-JIS", "Shift_JIS", "SJIS",
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
            return QString::fromUtf8(
              dstBuf.data(), static_cast<qsizetype>(dstSize - dstLeft));
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
FileQuery::exists(const QString& path) -> bool
{
    const QFileInfo info(localPathFromUserPath(path));
    return info.exists() && info.isFile() && !info.fileName().isEmpty();
}

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
} // namespace qml_components
