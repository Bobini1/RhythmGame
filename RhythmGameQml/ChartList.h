#ifndef CHARTLIST_H
#define CHARTLIST_H

#include <QAbstractListModel>

class ChartList : public QAbstractListModel
{
    Q_OBJECT

  public:
    explicit ChartList(QObject* parent = nullptr);

    enum
    {
        TitleRole = Qt::UserRole,
        ArtistRole,
    };

    [[nodiscard]] auto rowCount(const QModelIndex& parent) const
      -> int override;

    auto data(const QModelIndex& index, int role) const -> QVariant override;

    auto columnCount(const QModelIndex& parent) const -> int override;

    auto roleNames() const -> QHash<int, QByteArray> override;

  private:
    QList<QPair<QString, QString>> chartList;
};

#endif // CHARTLIST_H
