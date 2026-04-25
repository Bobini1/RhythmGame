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
#include <iconv.h>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <spdlog/spdlog.h>
#include <vector>

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
    const auto key = support::normalizeDxaPath(innerPath);
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
    if (data.data.isEmpty()) {
        spdlog::warn("Could not find LR2FONT segment '{}' in DXA '{}'",
                     support::normalizeDxaPath(dxaPath->second),
                     dxaPath->first.string());
    }
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

auto
invalidIconv() -> iconv_t
{
    return reinterpret_cast<iconv_t>(static_cast<std::intptr_t>(-1));
}

auto
openCp932Iconv() -> iconv_t
{
    for (const auto* encoding : { "CP932", "Windows-31J", "SHIFT_JIS" }) {
        iconv_t cd = iconv_open("UTF-8", encoding);
        if (cd != invalidIconv()) {
            return cd;
        }
    }
    return invalidIconv();
}

auto
decodeWithIconv(iconv_t cd, const QByteArray& data) -> std::optional<QString>
{
    if (cd == invalidIconv()) {
        return std::nullopt;
    }

    iconv(cd, nullptr, nullptr, nullptr, nullptr);

    auto* srcPtr = const_cast<char*>(data.constData());
    auto srcLeft = static_cast<size_t>(data.size());
    const auto dstSize = static_cast<size_t>(data.size()) * 4 + 4;
    std::vector<char> dstBuf(dstSize);
    auto* dstPtr = dstBuf.data();
    auto dstLeft = dstSize;

    const auto result = iconv(cd, &srcPtr, &srcLeft, &dstPtr, &dstLeft);

    if (result == static_cast<size_t>(-1)) {
        return std::nullopt;
    }

    return QString::fromUtf8(dstBuf.data(),
                             static_cast<qsizetype>(dstSize - dstLeft));
}

auto
decodeWithQt(const char* encoding, const QByteArray& data)
  -> std::optional<QString>
{
    QStringDecoder decoder(encoding);
    if (!decoder.isValid()) {
        return std::nullopt;
    }

    const auto decoded = decoder.decode(data);
    if (decoder.hasError()) {
        return std::nullopt;
    }

    return decoded;
}

auto
decodeWithQtCp932(const QByteArray& data) -> std::optional<QString>
{
    for (const auto* encoding : { "CP932", "windows-31j", "Shift-JIS" }) {
        if (auto decoded = decodeWithQt(encoding, data)) {
            return decoded;
        }
    }

    return std::nullopt;
}

class Cp932Decoder
{
  public:
    Cp932Decoder()
      : m_iconv(openCp932Iconv())
    {
    }

    ~Cp932Decoder()
    {
        if (m_iconv != invalidIconv()) {
            iconv_close(m_iconv);
        }
    }

    Cp932Decoder(const Cp932Decoder&) = delete;
    Cp932Decoder& operator=(const Cp932Decoder&) = delete;

    auto decode(const QByteArray& data) -> std::optional<QString>
    {
        if (m_iconv != invalidIconv()) {
            return decodeWithIconv(m_iconv, data);
        }

        return decodeWithQtCp932(data);
    }

  private:
    iconv_t m_iconv = invalidIconv();
};

auto
firstCodepoint(const QString& text) -> char32_t
{
    const auto codepoints = text.toUcs4();
    if (codepoints.isEmpty()) {
        return 0;
    }
    return static_cast<char32_t>(codepoints.front());
}

auto
decodeCp932Codepoint(Cp932Decoder& decoder, const QByteArray& bytes) -> char32_t
{
    const auto decoded = decoder.decode(bytes);
    if (!decoded) {
        return 0;
    }
    return firstCodepoint(*decoded);
}

auto
decodeLr2FontCharId(Cp932Decoder& decoder, const int chrId) -> char32_t
{
    if (chrId >= 0 && chrId <= 255) {
        QByteArray cp932Char;
        cp932Char.append(static_cast<char>(chrId));
        return decodeCp932Codepoint(decoder, cp932Char);
    }

    auto decodeTwoByteShiftJis = [&decoder](const int sjis) {
        QByteArray cp932Chars;
        cp932Chars.append(static_cast<char>((sjis >> 8) & 0xFF));
        cp932Chars.append(static_cast<char>(sjis & 0xFF));
        return decodeCp932Codepoint(decoder, cp932Chars);
    };

    if (chrId >= 256 && chrId <= 8126) {
        return decodeTwoByteShiftJis(chrId + 32832);
    }

    if (chrId >= 8127 && chrId <= 15306) {
        return decodeTwoByteShiftJis(chrId + 49281);
    }

    return 0;
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

    Cp932Decoder cp932Decoder;
    const QString textData = cp932Decoder.decode(fontData.data)
                               .value_or(QString::fromLatin1(fontData.data));

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
            dict.imgMap[mapId] = static_cast<int>(dict.textures.size());
            dict.textures.append(std::move(img));
        } else if (cmd == "#R" && tokens.size() > 6) {
            const int chrId = tokens[1].toInt();
            const int imgId = tokens[2].toInt();
            const QRect r(tokens[3].toInt(),
                          tokens[4].toInt(),
                          tokens[5].toInt(),
                          tokens[6].toInt());

            const auto charCode = decodeLr2FontCharId(cp932Decoder, chrId);

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
