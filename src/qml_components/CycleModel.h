//
// Created by bobini on 09.10.23.
//

#ifndef RHYTHMGAME_CYCLEMODEL_H
#define RHYTHMGAME_CYCLEMODEL_H

#include <QAbstractListModel>
#include <QtQmlIntegration>
namespace qml_components {
class CycleModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int minimumAmount READ getMinimumAmount WRITE setMinimumAmount
                 NOTIFY minimumAmountChanged)
    Q_PROPERTY(QAbstractListModel* model READ getModel WRITE setModel NOTIFY
                 modelChanged)

    QAbstractListModel* model{};
    int minimumAmount = 1;

  public:
    explicit CycleModel(QObject* parent = nullptr);

    auto rowCount(const QModelIndex& parent = QModelIndex()) const
      -> int override;
    auto data(const QModelIndex& index, int role = Qt::DisplayRole) const
      -> QVariant override;

    auto roleNames() const -> QHash<int, QByteArray> override;

    auto getMinimumAmount() const -> int;
    void setMinimumAmount(int count);

    QAbstractListModel* getModel() const;
    void setModel(QAbstractListModel* model);

    Q_INVOKABLE QVariant at(int index);

  signals:
    void minimumAmountChanged();
    void modelChanged();
};
} // namespace qml_components

#endif // RHYTHMGAME_CYCLEMODEL_H
