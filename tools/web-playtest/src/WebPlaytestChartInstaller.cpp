#include "WebPlaytestChartInstaller.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMap>
#include <QRegularExpression>
#include <QSet>

#include <limits>

namespace web_playtest {
namespace {

constexpr auto manifestResource =
  ":/web-playtest/web-playtest-chart-manifest.json";
constexpr auto chartResourceRoot = ":/web-playtest/chart";
constexpr auto installedChartRoot = "/playtest/chart";

struct ManifestFile
{
    QString virtualPath;
    QString resourcePath;
    QByteArray sha256;
    qint64 size{};
};

auto
fail(QString& error, QString message) -> QString
{
    error = std::move(message);
    return {};
}

auto
readSmallFile(QFile& file, QString& error) -> QByteArray
{
    QByteArray contents;
    constexpr auto chunkSize = qint64{ 64 * 1024 };
    while (true) {
        const auto chunk = file.read(chunkSize);
        if (chunk.isEmpty()) {
            if (file.error() != QFileDevice::NoError) {
                error = QStringLiteral("Could not read chart manifest: %1")
                          .arg(file.errorString());
            }
            break;
        }
        contents.append(chunk);
    }
    return contents;
}

auto
parseManifestFile(const QJsonObject& object,
                  ManifestFile& output,
                  QString& error) -> bool
{
    const auto virtualPath = object.value(QStringLiteral("virtualPath"));
    const auto sha256 = object.value(QStringLiteral("sha256"));
    const auto size = object.value(QStringLiteral("size"));
    if (!virtualPath.isString() || !sha256.isString() || !size.isDouble()) {
        error = QStringLiteral("Chart manifest file entry has invalid types");
        return false;
    }

    output.virtualPath = virtualPath.toString();
    const auto prefix = QStringLiteral("/playtest/chart/");
    if (!output.virtualPath.startsWith(prefix) ||
        output.virtualPath.contains(u'\\') ||
        QDir::cleanPath(output.virtualPath) != output.virtualPath) {
        error = QStringLiteral("Unsafe chart manifest path: %1")
                  .arg(output.virtualPath);
        return false;
    }
    const auto relative = output.virtualPath.sliced(prefix.size());
    if (relative.isEmpty() || relative.startsWith(u'/') ||
        relative.contains(QStringLiteral("/../")) ||
        relative.contains(QStringLiteral("/./")) || relative.endsWith(u'/')) {
        error = QStringLiteral("Unsafe chart manifest relative path: %1")
                  .arg(relative);
        return false;
    }
    output.resourcePath =
      QStringLiteral("%1/%2").arg(chartResourceRoot, relative);

    const auto hashText = sha256.toString();
    static const QRegularExpression lowercaseSha256{ QStringLiteral(
      "^[0-9a-f]{64}$") };
    if (!lowercaseSha256.match(hashText).hasMatch()) {
        error =
          QStringLiteral("Invalid SHA-256 for %1").arg(output.virtualPath);
        return false;
    }
    output.sha256 = hashText.toLatin1();

    const auto sizeValue = size.toDouble();
    if (sizeValue < 0.0 ||
        sizeValue > static_cast<double>(std::numeric_limits<qint64>::max()) ||
        sizeValue != static_cast<double>(static_cast<qint64>(sizeValue))) {
        error =
          QStringLiteral("Invalid byte size for %1").arg(output.virtualPath);
        return false;
    }
    output.size = static_cast<qint64>(sizeValue);
    return true;
}

auto
copyVerified(const ManifestFile& entry, QString& error) -> bool
{
    QFile input{ entry.resourcePath };
    if (!input.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Could not open chart resource %1: %2")
                  .arg(entry.resourcePath, input.errorString());
        return false;
    }

    const auto outputPath = entry.virtualPath;
    if (!QDir{}.mkpath(QFileInfo{ outputPath }.path())) {
        error = QStringLiteral("Could not create chart directory for %1")
                  .arg(outputPath);
        return false;
    }
    QFile output{ outputPath };
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = QStringLiteral("Could not create chart file %1: %2")
                  .arg(outputPath, output.errorString());
        return false;
    }

    QCryptographicHash hash{ QCryptographicHash::Sha256 };
    qint64 copied = 0;
    constexpr auto chunkSize = qint64{ 64 * 1024 };
    while (true) {
        const auto chunk = input.read(chunkSize);
        if (chunk.isEmpty()) {
            if (input.error() != QFileDevice::NoError) {
                error = QStringLiteral("Could not read %1: %2")
                          .arg(entry.resourcePath, input.errorString());
                output.close();
                output.remove();
                return false;
            }
            break;
        }
        if (output.write(chunk) != chunk.size()) {
            error = QStringLiteral("Could not write %1: %2")
                      .arg(outputPath, output.errorString());
            output.close();
            output.remove();
            return false;
        }
        hash.addData(chunk);
        copied += chunk.size();
    }
    if (!output.flush()) {
        error = QStringLiteral("Could not flush %1: %2")
                  .arg(outputPath, output.errorString());
        output.close();
        output.remove();
        return false;
    }
    output.close();

    if (copied != entry.size || hash.result().toHex() != entry.sha256) {
        error = QStringLiteral("Chart resource digest mismatch: %1")
                  .arg(entry.virtualPath);
        output.remove();
        return false;
    }
    return true;
}

}

auto
WebPlaytestChartInstaller::install(QString& error) -> QString
{
    error.clear();
    QFile manifest{ QString::fromLatin1(manifestResource) };
    if (!manifest.open(QIODevice::ReadOnly)) {
        return fail(error,
                    QStringLiteral("Could not open embedded chart manifest: %1")
                      .arg(manifest.errorString()));
    }
    auto manifestBytes = readSmallFile(manifest, error);
    if (!error.isEmpty()) {
        return {};
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(manifestBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return fail(error,
                    QStringLiteral("Invalid embedded chart manifest: %1")
                      .arg(parseError.errorString()));
    }
    const auto object = document.object();
    if (object.value(QStringLiteral("schema")).toInt(-1) != 1) {
        return fail(error, QStringLiteral("Unsupported chart manifest schema"));
    }
    const auto selectedValue =
      object.value(QStringLiteral("selectedVirtualPath"));
    const auto selectedSha =
      object.value(QStringLiteral("selectedChartSha256"));
    const auto filesValue = object.value(QStringLiteral("files"));
    if (!selectedValue.isString() || !selectedSha.isString() ||
        !filesValue.isArray()) {
        return fail(error, QStringLiteral("Incomplete chart manifest"));
    }

    QMap<QString, ManifestFile> manifestFiles;
    for (const auto& value : filesValue.toArray()) {
        if (!value.isObject()) {
            return fail(
              error,
              QStringLiteral("Chart manifest contains a non-object file"));
        }
        ManifestFile entry;
        if (!parseManifestFile(value.toObject(), entry, error)) {
            return {};
        }
        if (manifestFiles.contains(entry.virtualPath)) {
            return fail(error,
                        QStringLiteral("Duplicate chart manifest path: %1")
                          .arg(entry.virtualPath));
        }
        manifestFiles.insert(entry.virtualPath, std::move(entry));
    }
    if (manifestFiles.isEmpty()) {
        return fail(error, QStringLiteral("Chart manifest is empty"));
    }

    const auto selectedPath = selectedValue.toString();
    const auto selectedIterator = manifestFiles.constFind(selectedPath);
    if (selectedIterator == manifestFiles.cend() ||
        QString::fromLatin1(selectedIterator->sha256) !=
          selectedSha.toString()) {
        return fail(
          error,
          QStringLiteral("Selected chart is not authoritative in manifest"));
    }

    QSet<QString> resourceFiles;
    QDirIterator iterator{ QString::fromLatin1(chartResourceRoot),
                           QDir::Files | QDir::NoDotAndDotDot,
                           QDirIterator::Subdirectories };
    while (iterator.hasNext()) {
        resourceFiles.insert(iterator.next());
    }
    const QSet<QString> expectedResources = [&manifestFiles] {
        QSet<QString> result;
        for (const auto& entry : manifestFiles) {
            result.insert(entry.resourcePath);
        }
        return result;
    }();
    if (resourceFiles != expectedResources) {
        return fail(
          error,
          QStringLiteral(
            "Embedded chart resource inventory differs from manifest"));
    }

    QDir installedRoot{ QString::fromLatin1(installedChartRoot) };
    if (installedRoot.exists() && !installedRoot.removeRecursively()) {
        return fail(
          error, QStringLiteral("Could not reset installed chart directory"));
    }
    if (!QDir{}.mkpath(QString::fromLatin1(installedChartRoot))) {
        return fail(
          error, QStringLiteral("Could not create installed chart directory"));
    }
    for (const auto& entry : manifestFiles) {
        if (!copyVerified(entry, error)) {
            installedRoot.removeRecursively();
            return {};
        }
    }
    return selectedPath;
}

}
