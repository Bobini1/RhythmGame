#pragma once

#include <QHash>
#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace resource_managers {

class ChartFolderModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(int keymodeFilter READ keymodeFilter WRITE setKeymodeFilter NOTIFY keymodeFilterChanged)
    Q_PROPERTY(int difficultyFilter READ difficultyFilter WRITE setDifficultyFilter NOTIFY difficultyFilterChanged)
    Q_PROPERTY(bool unscoredItemsLast READ unscoredItemsLast WRITE setUnscoredItemsLast NOTIFY unscoredItemsLastChanged)
    Q_PROPERTY(QVariantMap scores READ scores WRITE setScores NOTIFY scoresChanged)

  public:
    explicit ChartFolderModel(QObject* parent = nullptr);

    int sortMode() const;
    void setSortMode(int value);
    int keymodeFilter() const;
    void setKeymodeFilter(int value);
    int difficultyFilter() const;
    void setDifficultyFilter(int value);
    bool unscoredItemsLast() const;
    void setUnscoredItemsLast(bool value);
    QVariantMap scores() const;
    void setScores(const QVariantMap& value);

    Q_INVOKABLE QVariantList filterAndSort(const QVariantList& input) const;
    Q_INVOKABLE int indexOfItem(const QVariantList& items, const QVariant& item) const;
    Q_INVOKABLE bool sortModeUsesScores(int mode = -1) const;
    Q_INVOKABLE void rebuildFolderIndexes(const QVariantList& input);
    Q_INVOKABLE QVariantList chartsForSameFolderAndKeymode(const QVariant& chart) const;
    Q_INVOKABLE int difficultyForChart(const QVariant& chart) const;

  signals:
    void sortModeChanged();
    void keymodeFilterChanged();
    void difficultyFilterChanged();
    void unscoredItemsLastChanged();
    void scoresChanged();

  private:
    int m_sortMode = 1;
    int m_keymodeFilter = 0;
    int m_difficultyFilter = 0;
    bool m_unscoredItemsLast = true;
    QVariantMap m_scores;
    QHash<QString, QHash<QString, QVariantList>> m_chartGroupsByFolderKeymode;
    QHash<QString, int> m_chartDifficultyByPath;
};

} // namespace resource_managers
