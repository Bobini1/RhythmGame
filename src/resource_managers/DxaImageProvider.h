//
// Created by bobini on 13/12/2025.
//

#ifndef RHYTHMGAME_DXAIMAGEPROVIDER_H
#define RHYTHMGAME_DXAIMAGEPROVIDER_H

#include "support/dxa.h"

#include <QQuickImageProvider>
#include <filesystem>
#include <mutex>
#include <unordered_map>
namespace resource_managers {

class DxaImageProvider final : public QQuickImageProvider
{
    std::mutex archiveMutex;
    std::unordered_map<std::filesystem::path, support::DXArchive>
      loadedArchives;
    void loadArchive(const std::filesystem::path& path);

  public:
    DxaImageProvider();
    auto requestPixmap(const QString& id,
                       QSize* size,
                       const QSize& requestedSize) -> QPixmap override;
};

} // namespace resource_managers

#endif // RHYTHMGAME_DXAIMAGEPROVIDER_H
