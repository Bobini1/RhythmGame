#pragma once

#include <QString>
#include <QUrl>

namespace support {

inline auto
folderName(QString path) -> QString
{
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
    }
    const QUrl url(path);
    // A one-letter scheme is a Windows drive, whose name must stay literal.
    if (url.isValid() && url.scheme().size() > 1) {
        return QUrl::fromPercentEncoding(
          url.fileName(QUrl::FullyEncoded).toUtf8());
    }
    return path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);
}

} // namespace support
