#include "ChartList.h"

ChartList::ChartList(QObject* parent)
  : QAbstractListModel(parent)
{
    chartList.push_back(std::make_pair("title1", "artist1"));
    chartList.push_back(std::make_pair("title2", "artist2"));
}

auto
ChartList::rowCount(const QModelIndex& parent) const -> int
{
    return chartList.count();
}

auto
ChartList::data(const QModelIndex& index, int role) const -> QVariant
{
    // the index returns the requested row and column information.
    // we ignore the column and only use the row information
    const int row = index.row();

    // boundary check for the row
    if (row < 0 || row >= chartList.count()) {
        return {};
    }

    // FIXME: Implement me!
    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return chartList[index.row()].first;
        }
        if (index.column() == 1) {
            return chartList[index.row()].second;
        }
    }
}
auto
ChartList::roleNames() const -> QHash<int, QByteArray>
{
    QHash<int, QByteArray> roles;
    roles[TitleRole] = "title";
    roles[ArtistRole] = "artist";
    return roles;
}
auto
ChartList::columnCount(const QModelIndex& parent) const -> int
{
    return 2;
}
