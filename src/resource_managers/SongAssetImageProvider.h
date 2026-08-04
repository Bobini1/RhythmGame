#ifndef RHYTHMGAME_SONGASSETIMAGEPROVIDER_H
#define RHYTHMGAME_SONGASSETIMAGEPROVIDER_H

#include <QQuickAsyncImageProvider>
#include <QThreadPool>

namespace resource_managers {

class SongAssetStore;

class SongAssetImageProvider final : public QQuickAsyncImageProvider
{
  public:
    explicit SongAssetImageProvider(SongAssetStore* store);
    auto requestImageResponse(const QString& id, const QSize& requestedSize)
      -> QQuickImageResponse* override;

  private:
    SongAssetStore* store;
    QThreadPool threadPool;
};

} // namespace resource_managers

#endif // RHYTHMGAME_SONGASSETIMAGEPROVIDER_H
