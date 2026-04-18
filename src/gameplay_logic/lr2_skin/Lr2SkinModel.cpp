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

int Lr2SkinModel::startInput() const {
    return m_startInput;
}

int Lr2SkinModel::sceneTime() const {
    return m_sceneTime;
}

int Lr2SkinModel::fadeOut() const {
    return m_fadeOut;
}

int Lr2SkinModel::skip() const {
    return m_skip;
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
    const auto skinData = Lr2SkinParser::parseData(m_csvPath, m_settingValues, m_activeOptions);
    m_elements = skinData.elements;
    const bool metadataChanged = m_startInput != skinData.startInput ||
                                 m_sceneTime != skinData.sceneTime ||
                                 m_fadeOut != skinData.fadeOut ||
                                 m_skip != skinData.skip;
    m_startInput = skinData.startInput;
    m_sceneTime = skinData.sceneTime;
    m_fadeOut = skinData.fadeOut;
    m_skip = skinData.skip;
    endResetModel();
    if (metadataChanged) {
        emit skinMetadataChanged();
    }
    emit skinLoaded();
}

} // namespace gameplay_logic::lr2_skin
