//
// Created by bobini on 29.11.23.
//

#ifndef CONFIGSERIALIZER_H
#define CONFIGSERIALIZER_H
#include "qml_components/ThemeFamily.h"

#include <QQmlPropertyMap>
#include <filesystem>
#include <QJsonObject>
#include <QThreadPool>
#include <mutex>

namespace resource_managers {

auto
readJsonConfig(const std::filesystem::path& path) -> QJsonObject;
void
writeJsonConfig(const std::filesystem::path& path,
                QJsonObject values,
                bool merge = false);

// Keeps at most one pending snapshot per file while a write is in progress.
class ConfigWriter
{
    struct PendingWrite
    {
        QJsonObject values;
        bool merge;
    };
    std::mutex mutex;
    QHash<QString, PendingWrite> pending;
    bool running{};
    QThreadPool pool;

  public:
    ~ConfigWriter();
    void write(const std::filesystem::path& path,
               QJsonObject values,
               bool merge = false);
};

auto
writeConfig(const std::filesystem::path& path, QQmlPropertyMap& object) -> void;
auto
readConfig(const std::filesystem::path& path,
           QQmlPropertyMap& object,
           const QMap<QString, qml_components::ThemeFamily>& themeFamilies)
  -> void;

} // namespace resource_managers

#endif // CONFIGSERIALIZER_H
