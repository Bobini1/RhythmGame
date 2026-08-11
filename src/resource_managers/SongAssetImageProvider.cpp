#include "SongAssetImageProvider.h"

#include "SongAssetStore.h"

#include <QBuffer>
#include <QImageReader>
#include <QQuickImageResponse>
#include <QQuickTextureFactory>
#include <QRunnable>
#include <QThreadPool>

#include <atomic>
#include <mutex>

namespace resource_managers {
namespace {

class SongAssetImageResponse final
  : public QQuickImageResponse
  , public QRunnable
{
  public:
    SongAssetImageResponse(SongAssetStore* store,
                           QString id,
                           QSize requestedSize)
      : store(store)
      , id(std::move(id))
      , requestedSize(requestedSize)
    {
        setAutoDelete(false);
    }

    void run() override
    {
        try {
            if (!cancelled.load()) {
                auto contents =
                  store->read(SongAssetStore::pathFromUrl(
                                QStringLiteral("image://song-assets/") + id),
                              &cancelled);
                if (!cancelled.load()) {
                    auto buffer = QBuffer{ &contents };
                    if (!buffer.open(QIODevice::ReadOnly)) {
                        throw std::runtime_error(
                          "Could not open archived image buffer");
                    }
                    auto reader = QImageReader{ &buffer };
                    reader.setAutoTransform(true);
                    if (requestedSize.isValid()) {
                        reader.setScaledSize(requestedSize);
                    }
                    auto loaded = reader.read();
                    std::scoped_lock lock{ mutex };
                    if (loaded.isNull()) {
                        error = reader.errorString();
                    } else {
                        image = std::move(loaded);
                    }
                }
            }
        } catch (const std::exception& exception) {
            if (!cancelled.load()) {
                std::scoped_lock lock{ mutex };
                error = QString::fromUtf8(exception.what());
            }
        }
        emit finished();
    }

    auto textureFactory() const -> QQuickTextureFactory* override
    {
        std::scoped_lock lock{ mutex };
        return image.isNull()
                 ? nullptr
                 : QQuickTextureFactory::textureFactoryForImage(image);
    }

    auto errorString() const -> QString override
    {
        std::scoped_lock lock{ mutex };
        return error;
    }

    void cancel() override { cancelled.store(true); }

  private:
    SongAssetStore* store;
    QString id;
    QSize requestedSize;
    mutable std::mutex mutex;
    QImage image;
    QString error;
    std::atomic_bool cancelled = false;
};

} // namespace

SongAssetImageProvider::SongAssetImageProvider(SongAssetStore* store)
  : store(store)
{
    threadPool.setMaxThreadCount(1);
}

auto
SongAssetImageProvider::requestImageResponse(const QString& id,
                                             const QSize& requestedSize)
  -> QQuickImageResponse*
{
    auto* response = new SongAssetImageResponse{ store, id, requestedSize };
    threadPool.start(response);
    return response;
}

} // namespace resource_managers
