//
// Created by bobini on 13/12/2025.
//

#ifndef RHYTHMGAME_DXAIMAGEPROVIDER_H
#define RHYTHMGAME_DXAIMAGEPROVIDER_H

#include "support/dxa.h"

#include <QQuickImageProvider>
#include <filesystem>
#include <mutex>
namespace resource_managers {

class DxaImageProvider final : public QQuickImageProvider
{
    QHash<QString, QHash<QRect, QPixmap>> pixmaps;
    std::mutex pixmapsMutex;

    std::mutex archiveMutex;
    std::unordered_map<std::filesystem::path, support::DXArchive>
      loadedArchives;
    void loadArchive(const std::filesystem::path& path);
    void unloadArchive(const std::string& name);
    void clearArchives();

  public:
    DxaImageProvider();
    auto requestPixmap(const QString& id,
                       QSize* size,
                       const QSize& requestedSize) -> QPixmap override;
};

} // namespace resource_managers

#endif // RHYTHMGAME_DXAIMAGEPROVIDER_H
