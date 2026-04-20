//
// Created by PC on 08/10/2023.
//

#include "FileQuery.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"
#include <QFileInfo>
#include <QStringDecoder>
#include <spdlog/spdlog.h>
namespace {

auto
getSelectableFilesForDirectory(const std::filesystem::path& path)
  -> QList<QString>
{
    auto files = QList<QString>{};
    for (const auto& file : std::filesystem::directory_iterator(path)) {
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
    if (data.startsWith("\xEF\xBB\xBF")) {
        return QString::fromUtf8(data.sliced(3));
    }

    // LR2-era text files are commonly saved as Japanese Windows text without
    // a BOM. Prefer that family for no-BOM files; ASCII survives unchanged.
    for (const auto* encoding :
         { "CP932", "windows-31j", "Shift-JIS" }) {
        QStringDecoder decoder(encoding);
        if (!decoder.isValid()) {
            continue;
        }
        const auto decoded = decoder.decode(data);
        if (!decoder.hasError()) {
            return decoded;
        }
    }

    QStringDecoder utf8Decoder(QStringConverter::Utf8);
    const auto utf8 = utf8Decoder.decode(data);
    if (!utf8Decoder.hasError()) {
        return utf8;
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
