#include "Lr2FontCache.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringDecoder>
#include <QStringList>
#include <spdlog/spdlog.h>

namespace resource_managers {

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
        return &it.value();
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::warn("Could not open LR2FONT: {}", path.toStdString());
        return nullptr;
    }

    const auto data = file.readAll();
    QStringDecoder decoder("Shift-JIS");
    if (!decoder.isValid()) {
        decoder = QStringDecoder(QStringConverter::Latin1);
    }
    const QString textData = decoder(data);

    Lr2FontDict dict;
    const QFileInfo fi(path);
    const QDir dir = fi.absoluteDir();

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
            QImage img(dir.absoluteFilePath(imgPath));
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
                g.imgIdx = dict.imgMap.value(imgId, -1);
                g.rect = r;
                dict.glyphs[charCode] = g;
            }
        }
    }

    auto iterator = m_cache.insert(path, std::move(dict));
    return &iterator.value();
}

} // namespace resource_managers
