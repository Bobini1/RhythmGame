#include "Lr2FontCache.h"

#include "support/QStringToPath.h"
#include "support/dxa.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QStringDecoder>
#include <QStringList>
#include <filesystem>
#include <optional>
#include <spdlog/spdlog.h>

namespace resource_managers {
namespace {

struct FontData
{
    QByteArray data;
    std::optional<support::DXArchive> archive;
    std::filesystem::path innerDir;
    QDir filesystemDir;
};

auto
readDxaSegment(const support::DXArchive& archive,
               const std::filesystem::path& innerPath) -> QByteArray
{
    const auto key = std::filesystem::weakly_canonical(innerPath).string();
    const auto it = archive.find(key);
    if (it == archive.end()) {
        return {};
    }

    const auto& segment = it->second;
    return QByteArray(reinterpret_cast<const char*>(segment.data.get()),
                      static_cast<qsizetype>(segment.size));
}

auto
findDxaBackedPath(const std::filesystem::path& path)
  -> std::optional<std::pair<std::filesystem::path, std::filesystem::path>>
{
    for (auto parent = path.parent_path(); !parent.empty();) {
        auto archivePath = parent;
        archivePath += ".dxa";
        if (!std::filesystem::is_regular_file(archivePath)) {
            const auto nextParent = parent.parent_path();
            if (nextParent == parent) {
                break;
            }
            parent = nextParent;
            continue;
        }

        std::error_code ec;
        auto innerPath = std::filesystem::relative(path, parent, ec);
        if (ec) {
            innerPath = path.filename();
        }
        return std::make_pair(archivePath, innerPath);
    }
    return std::nullopt;
}

auto
loadFontData(const QString& path) -> FontData
{
    FontData data;

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        data.data = file.readAll();
        data.filesystemDir = QFileInfo(path).absoluteDir();
        return data;
    }

    const auto dxaPath = findDxaBackedPath(support::qStringToPath(path));
    if (!dxaPath) {
        return data;
    }

    auto archive = support::extractDxaToMem(dxaPath->first);
    if (archive.empty()) {
        return data;
    }

    data.data = readDxaSegment(archive, dxaPath->second);
    data.innerDir = dxaPath->second.parent_path();
    data.archive = std::move(archive);
    return data;
}

auto
readFontImage(const FontData& data, const QString& imagePath) -> QImage
{
    if (data.archive) {
        const auto innerPath =
          data.innerDir / support::qStringToPath(imagePath);
        const auto imageData = readDxaSegment(*data.archive, innerPath);
        if (imageData.isEmpty()) {
            return {};
        }

        QBuffer buffer;
        buffer.setData(imageData);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer);
        const auto suffix = QFileInfo(imagePath).suffix().toLatin1();
        if (!suffix.isEmpty()) {
            reader.setFormat(suffix);
        }
        return reader.read();
    }

    return QImage(data.filesystemDir.absoluteFilePath(imagePath));
}

} // namespace

Lr2FontCache&
Lr2FontCache::instance()
{
    static Lr2FontCache s;
    return s;
}

const Lr2FontDict*
Lr2FontCache::load(const QString& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (const auto it = m_cache.find(path); it != m_cache.end()) {
        return it.value().get();
    }

    const auto fontData = loadFontData(path);
    if (fontData.data.isEmpty()) {
        spdlog::warn("Could not open LR2FONT: {}", path.toStdString());
        return nullptr;
    }

    QStringDecoder decoder("Shift-JIS");
    if (!decoder.isValid()) {
        decoder = QStringDecoder(QStringConverter::Latin1);
    }
    const QString textData = decoder(fontData.data);

    Lr2FontDict dict;

    const auto lines = textData.split('\n');
    for (const auto& qline : lines) {
        const auto line = qline.trimmed();
        if (line.isEmpty() || line.startsWith("//")) {
            continue;
        }
        const auto tokens = line.split(',');
        const auto cmd = tokens[0].trimmed().toUpper();

        if (cmd == "#S" && tokens.size() > 1) {
            dict.height = tokens[1].toInt();
        } else if (cmd == "#T" && tokens.size() > 2) {
            const int mapId = tokens[1].toInt();
            const auto imgPath = tokens[2].trimmed();
            QImage img = readFontImage(fontData, imgPath);
            dict.imgMap[mapId] = dict.textures.size();
            dict.textures.append(std::move(img));
        } else if (cmd == "#R" && tokens.size() > 6) {
            const int chrId = tokens[1].toInt();
            const int imgId = tokens[2].toInt();
            const QRect r(tokens[3].toInt(),
                          tokens[4].toInt(),
                          tokens[5].toInt(),
                          tokens[6].toInt());

            char32_t charCode = 0;
            if (chrId >= 0 && chrId <= 255) {
                charCode = static_cast<char32_t>(chrId);
            } else if (chrId >= 256 && chrId <= 8126) {
                const int i = chrId + 32832;
                QByteArray sjisChars;
                sjisChars.append(static_cast<char>((i >> 8) & 0xFF));
                sjisChars.append(static_cast<char>(i & 0xFF));
                QStringDecoder sjisDecoder("Shift-JIS");
                const auto decoded = QString(sjisDecoder(sjisChars));
                if (!decoded.isEmpty()) {
                    charCode = decoded.at(0).unicode();
                }
            } else if (chrId >= 8127 && chrId <= 15306) {
                const int i = chrId + 49281;
                QByteArray sjisChars;
                sjisChars.append(static_cast<char>((i >> 8) & 0xFF));
                sjisChars.append(static_cast<char>(i & 0xFF));
                QStringDecoder sjisDecoder2("Shift-JIS");
                const auto decoded = QString(sjisDecoder2(sjisChars));
                if (!decoded.isEmpty()) {
                    charCode = decoded.at(0).unicode();
                }
            }

            if (charCode != 0) {
                Lr2FontGlyph g;
                // LR2 fonts define a space rectangle for advance only; Vibes
                // keeps it as a blank glyph instead of sampling an atlas.
                g.imgIdx = charCode == U' ' ? -1 : dict.imgMap.value(imgId, -1);
                g.rect = r;
                dict.glyphs[charCode] = g;
            }
        }
    }

    auto iterator =
      m_cache.insert(path, std::make_shared<Lr2FontDict>(std::move(dict)));
    return iterator.value().get();
}

} // namespace resource_managers
