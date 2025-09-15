//
// Created by PC on 15/09/2025.
//

#ifndef RHYTHMGAME_AVATARIMAGEPROVIDER_H
#define RHYTHMGAME_AVATARIMAGEPROVIDER_H
#include <QQuickImageProvider>
#include <filesystem>

namespace resource_managers {
class AvatarImageProvider final : public QQuickImageProvider
{
    std::vector<std::filesystem::path> avatarFolders;
  public:
    AvatarImageProvider(std::vector<std::filesystem::path> avatarFolders);
    QPixmap requestPixmap(const QString& id,
                          QSize* size,
                          const QSize& requestedSize) override;
};
} // namespace resource_managers

#endif // RHYTHMGAME_AVATARIMAGEPROVIDER_H
