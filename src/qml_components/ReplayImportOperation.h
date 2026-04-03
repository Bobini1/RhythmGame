// Created by Codex on 01.04.2026.

#ifndef RHYTHMGAME_REPLAYIMPORTOPERATION_H
#define RHYTHMGAME_REPLAYIMPORTOPERATION_H

#include <QAbstractListModel>
#include <QStringList>
#include <qqmlintegration.h>

namespace qml_components {

/**
 * @brief Tracks the progress of an asynchronous beatoraja replay import.
 * @details Instances are created by BeatorajaReplayImporter::importFolder().
 * Each processed file (whether imported, skipped, or failed) increments done
 * toward total. The error() signal fires for each failure without halting the
 * operation.
 */
class ReplayImportOperation final : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by BeatorajaReplayImporter")

    Q_PROPERTY(int done READ getDone NOTIFY progressChanged)
    Q_PROPERTY(int total READ getTotal NOTIFY progressChanged)
    Q_PROPERTY(int imported READ getImported NOTIFY progressChanged)
    Q_PROPERTY(int skipped READ getSkipped NOTIFY progressChanged)
    Q_PROPERTY(int errored READ getErrored NOTIFY progressChanged)
    Q_PROPERTY(bool finished READ isFinished NOTIFY finishedChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

    int currentDone{ 0 };
    int currentTotal{ 0 };
    int importedCount{ 0 };
    int skippedCount{ 0 };
    int erroredCount{ 0 };
    bool finishedFlag{ false };
    QStringList errorMessages;

    void checkFinished();

  public:
    enum Roles
    {
        MessageRole = Qt::UserRole
    };
    Q_ENUM(Roles)

    explicit ReplayImportOperation(int total, QObject* parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] auto getDone() const -> int { return currentDone; }
    [[nodiscard]] auto getTotal() const -> int { return currentTotal; }
    [[nodiscard]] auto getImported() const -> int { return importedCount; }
    [[nodiscard]] auto getSkipped() const -> int { return skippedCount; }
    [[nodiscard]] auto getErrored() const -> int { return erroredCount; }
    [[nodiscard]] auto isFinished() const -> bool { return finishedFlag; }

    /** Called on the main thread when a replay was successfully saved. */
    void incrementImported();

    /** Called on the main thread when a replay was already present and skipped.
     */
    void incrementSkipped();

    /** Called on the main thread when a replay failed to import. */
    void incrementErrored();

    /** Emits the error() signal with the given message (main thread). */
    void reportError(const QString& message);

  signals:
    void progressChanged();
    void finishedChanged();
    void countChanged();
    void error(const QString& message);
};

} // namespace qml_components

#endif // RHYTHMGAME_REPLAYIMPORTOPERATION_H
