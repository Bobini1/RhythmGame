//
// Created by bobini on 29.11.23.
//

#include "SerializeConfig.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <QJsonDocument>
#include <QFile>
#include <QSaveFile>
#include <QUuid>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <QJsonObject>

namespace resource_managers {
namespace {
auto
readDocument(const std::filesystem::path& path) -> QJsonDocument
{
    QFile file(path);
    if (!file.exists()) {
        return QJsonDocument(QJsonObject{});
    }
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(file.errorString().toStdString());
    }
    const auto contents = file.readAll();
    if (file.error() != QFile::NoError) {
        throw std::runtime_error(file.errorString().toStdString());
    }
    return QJsonDocument::fromJson(contents);
}
}

auto
readJsonConfig(const std::filesystem::path& path) -> QJsonObject
{
    const auto document = readDocument(path);
    if (!document.isObject()) {
        throw std::runtime_error("Invalid JSON configuration: " +
                                 path.string());
    }
    return document.object();
}

void
writeJsonConfig(const std::filesystem::path& path,
                QJsonObject values,
                bool merge)
{
    const auto previous = readDocument(path);
    if (!previous.isObject()) {
        const auto filename = support::pathToQString(path);
        const auto backup = filename + ".invalid-" +
                            QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!QFile::copy(filename, backup)) {
            throw std::runtime_error(
              "Could not preserve invalid configuration: " + path.string());
        }
        spdlog::warn("Saved invalid configuration to {}", backup.toStdString());
    } else if (merge) {
        auto merged = previous.object();
        for (auto it = values.begin(); it != values.end(); ++it) {
            merged[it.key()] = it.value();
        }
        values = std::move(merged);
    }
    const auto contents = QJsonDocument(values).toJson();
    QSaveFile file(support::pathToQString(path));
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(contents) != contents.size() || !file.commit()) {
        throw std::runtime_error("Could not save configuration " +
                                 path.string() + ": " +
                                 file.errorString().toStdString());
    }
}

ConfigWriter::~ConfigWriter()
{
    pool.waitForDone();
}

void
ConfigWriter::write(const std::filesystem::path& path,
                    QJsonObject values,
                    bool merge)
{
    const auto lock = std::lock_guard(mutex);
    pending.insert(support::pathToQString(path), { std::move(values), merge });
    if (running) {
        return;
    }
    running = true;
    pool.start([this] {
        while (true) {
            QString path;
            PendingWrite write;
            {
                const auto lock = std::lock_guard(mutex);
                if (pending.isEmpty()) {
                    running = false;
                    return;
                }
                auto first = pending.begin();
                path = first.key();
                write = std::move(first.value());
                pending.erase(first);
            }
            try {
                writeJsonConfig(support::qStringToPath(path),
                                std::move(write.values),
                                write.merge);
            } catch (const std::exception& error) {
                spdlog::error(
                  "Failed to save {}: {}", path.toStdString(), error.what());
            }
        }
    });
}

auto
writeConfig(const std::filesystem::path& path, QQmlPropertyMap& object) -> void
{
    auto map = QVariantMap{};
    for (auto&& key : object.keys()) {
        map[key] = object.value(key);
    }
    writeJsonConfig(path, QJsonObject::fromVariantMap(map));
}
auto
readConfig(const std::filesystem::path& path,
           QQmlPropertyMap& object,
           const QMap<QString, qml_components::ThemeFamily>& themeFamilies)
  -> void
{
    auto file = QFile{ path };
    if (!file.exists()) {
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::error("Failed to open config for reading: {}", path.string());
        return;
    }
    try {
        for (const auto& [key, value] :
             readJsonConfig(path).toVariantMap().asKeyValueRange()) {
            if (object.contains(key)) {
                if (themeFamilies.contains(value.toString()) &&
                    themeFamilies[value.toString()].getScreens().contains(
                      key)) {
                    object.insert(key, value);
                }
            }
        }
    } catch (std::exception& exception) {
        spdlog::error("{}", exception.what());
    }
}
} // namespace resource_managers
