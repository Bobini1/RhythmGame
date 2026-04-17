#include "Lr2SkinModel.h"

namespace gameplay_logic::lr2_skin {

Lr2SkinModel::Lr2SkinModel(QObject* parent) : QAbstractListModel(parent) {}

int Lr2SkinModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_elements.size();
}

QVariant Lr2SkinModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_elements.size()) return QVariant();

    const auto& item = m_elements[index.row()];
    switch (role) {
    case TypeRole: return item.type;
    case SrcRole: return item.src;
    case DstsRole: return item.dsts;
    default: return QVariant();
    }
}

QHash<int, QByteArray> Lr2SkinModel::roleNames() const {
    return {
        {TypeRole, "type"},
        {SrcRole, "src"},
        {DstsRole, "dsts"}
    };
}

QString Lr2SkinModel::csvPath() const {
    return m_csvPath;
}

QVariantMap Lr2SkinModel::settingValues() const {
    return m_settingValues;
}

QVariantList Lr2SkinModel::activeOptions() const {
    return m_activeOptions;
}

void Lr2SkinModel::setCsvPath(const QString& path) {
    if (m_csvPath == path) return;
    m_csvPath = path;
    emit csvPathChanged();
    loadSkin();
}

void Lr2SkinModel::setSettingValues(const QVariantMap& values) {
    if (m_settingValues == values) return;
    m_settingValues = values;
    emit settingValuesChanged();
    loadSkin();
}

void Lr2SkinModel::setActiveOptions(const QVariantList& options) {
    if (m_activeOptions == options) return;
    m_activeOptions = options;
    emit activeOptionsChanged();
    loadSkin();
}

void Lr2SkinModel::loadSkin() {
    beginResetModel();
    m_elements = Lr2SkinParser::parse(m_csvPath, m_settingValues, m_activeOptions);
    endResetModel();
}

} // namespace gameplay_logic::lr2_skin
