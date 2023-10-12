//
// Created by bobini on 09.10.23.
//

#include "CycleModel.h"
namespace qml_components {
CycleModel::CycleModel(QObject* parent)
  : QAbstractListModel(parent)
{
}
auto
CycleModel::getModel() const -> QAbstractListModel*
{
    return model;
}
void
CycleModel::setModel(QAbstractListModel* newModel)
{
    if (model != newModel) {
        beginResetModel();
        if (model != nullptr) {
            model->setParent(nullptr);
        }
        model = newModel;
        if (model != nullptr) {
            model->setParent(this);
        }
        endResetModel();
        emit modelChanged();
    }
}
auto
CycleModel::getMinimumAmount() const -> int
{
    return minimumAmount;
}
void
CycleModel::setMinimumAmount(int count)
{
    if (minimumAmount != count) {
        beginResetModel();
        minimumAmount = count;
        endResetModel();
        emit minimumAmountChanged();
    }
}
auto
CycleModel::data(const QModelIndex& index, int role) const -> QVariant
{
    if (model == nullptr || !index.isValid()) {
        return {};
    }
    auto rowCount = model->rowCount();
    auto modelIndex = index.row() % rowCount;
    return model->data(model->index(modelIndex), role);
}
auto
CycleModel::rowCount(const QModelIndex& parent) const -> int
{
    if (model == nullptr) {
        return 0;
    }
    auto count = model->rowCount();
    if (count < minimumAmount) {
        return minimumAmount;
    }
    return count;
}
QVariant
CycleModel::at(int index)
{
    if (model == nullptr) {
        return {};
    }
    auto rowCount = model->rowCount();
    auto modelIndex = index % rowCount;
    return model->data(model->index(modelIndex));
}
auto
CycleModel::roleNames() const -> QHash<int, QByteArray>
{
    if (model == nullptr) {
        return {};
    }
    return model->roleNames();
}

} // namespace qml_components
