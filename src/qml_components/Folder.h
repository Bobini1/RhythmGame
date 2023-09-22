//
// Created by bobini on 19.09.23.
//

#ifndef RHYTHMGAME_FOLDER_H
#define RHYTHMGAME_FOLDER_H

#include <QAbstractListModel>
#include <QtQmlIntegration>
#include "db/SqliteCppDb.h"
#include "gameplay_logic/ChartData.h"
namespace qml_components {

class Folder : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Folder is not creatable from QML")

    Q_PROPERTY(QString path READ getPath CONSTANT)
    Q_PROPERTY(int minimumAmount READ getMinimumAmount WRITE setMinimumAmount
                 NOTIFY minimumAmountChanged)
    Q_PROPERTY(QString parentFolder READ parentFolder CONSTANT)

    QString path;
    db::SqliteCppDb* db;
    QList<QString> childrenFolders;
    std::vector<gameplay_logic::ChartData*> chartData;
    int minimumAmount = 0;

  public:
    explicit Folder(QString path,
                    QList<QString> childrenDirs,
                    std::vector<gameplay_logic::ChartData::DTO> childrenCharts,
                    db::SqliteCppDb* db,
                    QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent = QModelIndex()) const
      -> int override;
    auto data(const QModelIndex& index, int role = Qt::DisplayRole) const
      -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;
    auto getPath() const -> QString;
    auto parentFolder() -> QString;
    Q_INVOKABLE QVariant at(int index);
    void setMinimumAmount(int amount);
    auto getMinimumAmount() const -> int;

  signals:
    void minimumAmountChanged();
};

} // namespace qml_components

#endif // RHYTHMGAME_FOLDER_H
