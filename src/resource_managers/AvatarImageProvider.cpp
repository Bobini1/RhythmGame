//
// Created by PC on 15/09/2025.
//

#include "AvatarImageProvider.h"
namespace resource_managers {
AvatarImageProvider::AvatarImageProvider(
  std::vector<std::filesystem::path> assetsPaths)
  : QQuickImageProvider(Pixmap)
  , assetsPaths(std::move(assetsPaths))
{
}
auto
AvatarImageProvider::requestPixmap(const QString& id,
                                   QSize* size,
                                   const QSize& requestedSize) -> QPixmap
{
    for (const auto& folder : assetsPaths) {
        auto path = folder / "avatars" / id.toStdString();
        QPixmap pixmap(QString::fromStdString(path.string()));
        if (pixmap.isNull()) {
            continue;
        }
        if (size) {
            *size = pixmap.size();
        }
        if (requestedSize.isValid()) {
            return pixmap.scaled(
              requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        return pixmap;
    }
    return {};
}
} // namespace resource_managers