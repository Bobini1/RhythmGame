#include "Lr2SkinModel.h"

namespace gameplay_logic::lr2_skin {

namespace {

QVariantMap findMouseCursorElement(const QList<Lr2Element>& elements) {
    for (const auto& element : elements) {
        if (element.type != 0 || !element.src.canConvert<Lr2SrcImage>()) {
            continue;
        }

        const auto src = element.src.value<Lr2SrcImage>();
        if (!src.mouseCursor) {
            continue;
        }

        return {
            {"src", QVariant::fromValue(src)},
            {"dsts", element.dsts}
        };
    }

    return {};
}

bool hasMouseHoverElement(const QList<Lr2Element>& elements) {
    for (const auto& element : elements) {
        if (element.type != 0 || !element.src.canConvert<Lr2SrcImage>()) {
            continue;
        }

        const auto src = element.src.value<Lr2SrcImage>();
        if (src.onMouse) {
            return true;
        }
    }

    return false;
}

}

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

int Lr2SkinModel::loadStart() const {
    return m_loadStart;
}

int Lr2SkinModel::loadEnd() const {
    return m_loadEnd;
}

int Lr2SkinModel::playStart() const {
    return m_playStart;
}

int Lr2SkinModel::fadeOut() const {
    return m_fadeOut;
}

int Lr2SkinModel::skip() const {
    return m_skip;
}

int Lr2SkinModel::skinWidth() const {
    return m_skinWidth;
}

int Lr2SkinModel::skinHeight() const {
    return m_skinHeight;
}

QVariantList Lr2SkinModel::effectiveActiveOptions() const {
    return m_effectiveActiveOptions;
}

QVariantList Lr2SkinModel::usedOptions() const {
    return m_usedOptions;
}

QVariantList Lr2SkinModel::barLampVariants() const {
    return m_barLampVariants;
}

QVariantList Lr2SkinModel::barRows() const {
    return m_barRows;
}

QVariantList Lr2SkinModel::helpFiles() const {
    return m_helpFiles;
}

QVariantMap Lr2SkinModel::mouseCursor() const {
    return m_mouseCursor;
}

bool Lr2SkinModel::hasMouseHover() const {
    return m_hasMouseHover;
}

QString Lr2SkinModel::transColor() const {
    return m_transColor;
}

bool Lr2SkinModel::hasTransColor() const {
    return m_hasTransColor;
}

bool Lr2SkinModel::reloadBanner() const {
    return m_reloadBanner;
}

int Lr2SkinModel::barCenter() const {
    return m_barCenter;
}

int Lr2SkinModel::barAvailableStart() const {
    return m_barAvailableStart;
}

int Lr2SkinModel::barAvailableEnd() const {
    return m_barAvailableEnd;
}

QVariantList Lr2SkinModel::noteSources() const {
    return m_noteSources;
}

QVariantList Lr2SkinModel::mineSources() const {
    return m_mineSources;
}

QVariantList Lr2SkinModel::lnStartSources() const {
    return m_lnStartSources;
}

QVariantList Lr2SkinModel::lnEndSources() const {
    return m_lnEndSources;
}

QVariantList Lr2SkinModel::lnBodySources() const {
    return m_lnBodySources;
}

QVariantList Lr2SkinModel::lnBodyActiveSources() const {
    return m_lnBodyActiveSources;
}

QVariantList Lr2SkinModel::autoNoteSources() const {
    return m_autoNoteSources;
}

QVariantList Lr2SkinModel::autoMineSources() const {
    return m_autoMineSources;
}

QVariantList Lr2SkinModel::autoLnStartSources() const {
    return m_autoLnStartSources;
}

QVariantList Lr2SkinModel::autoLnEndSources() const {
    return m_autoLnEndSources;
}

QVariantList Lr2SkinModel::autoLnBodySources() const {
    return m_autoLnBodySources;
}

QVariantList Lr2SkinModel::autoLnBodyActiveSources() const {
    return m_autoLnBodyActiveSources;
}

QVariantList Lr2SkinModel::noteDsts() const {
    return m_noteDsts;
}

QVariantList Lr2SkinModel::lineSources() const {
    return m_lineSources;
}

QVariantList Lr2SkinModel::lineDsts() const {
    return m_lineDsts;
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
    if (m_csvPath.isEmpty()) {
        beginResetModel();
        m_elements.clear();
        const bool metadataChanged = !m_effectiveActiveOptions.isEmpty() ||
                                     !m_usedOptions.isEmpty() ||
                                     !m_barLampVariants.isEmpty() ||
                                     !m_barRows.isEmpty() ||
                                     m_startInput != 0 ||
                                     m_sceneTime != 0 ||
                                     m_loadStart != 0 ||
                                     m_loadEnd != 0 ||
                                     m_playStart != 2000 ||
                                     m_fadeOut != 0 ||
                                     m_skip != 0 ||
                                     m_skinWidth != 640 ||
                                     m_skinHeight != 480 ||
                                     !m_helpFiles.isEmpty() ||
                                     !m_mouseCursor.isEmpty() ||
                                     m_hasMouseHover ||
                                     m_transColor != "#000000" ||
                                     m_hasTransColor ||
                                     m_reloadBanner ||
                                     m_barCenter != 0 ||
                                     m_barAvailableStart != 0 ||
                                     m_barAvailableEnd != -1 ||
                                     !m_noteSources.isEmpty() ||
                                     !m_mineSources.isEmpty() ||
                                     !m_lnStartSources.isEmpty() ||
                                     !m_lnEndSources.isEmpty() ||
                                     !m_lnBodySources.isEmpty() ||
                                     !m_lnBodyActiveSources.isEmpty() ||
                                     !m_autoNoteSources.isEmpty() ||
                                     !m_autoMineSources.isEmpty() ||
                                     !m_autoLnStartSources.isEmpty() ||
                                     !m_autoLnEndSources.isEmpty() ||
                                     !m_autoLnBodySources.isEmpty() ||
                                     !m_autoLnBodyActiveSources.isEmpty() ||
                                     !m_noteDsts.isEmpty() ||
                                     !m_lineSources.isEmpty() ||
                                     !m_lineDsts.isEmpty();
        m_effectiveActiveOptions.clear();
        m_usedOptions.clear();
        m_barLampVariants.clear();
        m_barRows.clear();
        m_helpFiles.clear();
        m_mouseCursor.clear();
        m_hasMouseHover = false;
        m_transColor = "#000000";
        m_hasTransColor = false;
        m_reloadBanner = false;
        m_startInput = 0;
        m_sceneTime = 0;
        m_loadStart = 0;
        m_loadEnd = 0;
        m_playStart = 2000;
        m_fadeOut = 0;
        m_skip = 0;
        m_skinWidth = 640;
        m_skinHeight = 480;
        m_barCenter = 0;
        m_barAvailableStart = 0;
        m_barAvailableEnd = -1;
        m_noteSources.clear();
        m_mineSources.clear();
        m_lnStartSources.clear();
        m_lnEndSources.clear();
        m_lnBodySources.clear();
        m_lnBodyActiveSources.clear();
        m_autoNoteSources.clear();
        m_autoMineSources.clear();
        m_autoLnStartSources.clear();
        m_autoLnEndSources.clear();
        m_autoLnBodySources.clear();
        m_autoLnBodyActiveSources.clear();
        m_noteDsts.clear();
        m_lineSources.clear();
        m_lineDsts.clear();
        endResetModel();
        if (metadataChanged) {
            emit skinMetadataChanged();
        }
        return;
    }

    beginResetModel();
    const auto skinData = Lr2SkinParser::parseData(m_csvPath, m_settingValues, m_activeOptions);
    m_elements = skinData.elements;
    const auto mouseCursor = findMouseCursorElement(m_elements);
    const bool hasMouseHover = hasMouseHoverElement(m_elements);
    const bool metadataChanged = m_startInput != skinData.startInput ||
                                 m_sceneTime != skinData.sceneTime ||
                                 m_loadStart != skinData.loadStart ||
                                 m_loadEnd != skinData.loadEnd ||
                                 m_playStart != skinData.playStart ||
                                 m_fadeOut != skinData.fadeOut ||
                                 m_skip != skinData.skip ||
                                 m_skinWidth != skinData.skinWidth ||
                                 m_skinHeight != skinData.skinHeight ||
                                 m_effectiveActiveOptions != skinData.activeOptions ||
                                 m_usedOptions != skinData.usedOptions ||
                                 m_barLampVariants != skinData.barLampVariants ||
                                 m_barRows != skinData.barRows ||
                                 m_helpFiles != skinData.helpFiles ||
                                 m_mouseCursor != mouseCursor ||
                                 m_hasMouseHover != hasMouseHover ||
                                 m_transColor != skinData.transColor ||
                                 m_hasTransColor != skinData.hasTransColor ||
                                 m_reloadBanner != skinData.reloadBanner ||
                                 m_barCenter != skinData.barCenter ||
                                 m_barAvailableStart != skinData.barAvailableStart ||
                                 m_barAvailableEnd != skinData.barAvailableEnd ||
                                 m_noteSources != skinData.noteSources ||
                                 m_mineSources != skinData.mineSources ||
                                 m_lnStartSources != skinData.lnStartSources ||
                                 m_lnEndSources != skinData.lnEndSources ||
                                 m_lnBodySources != skinData.lnBodySources ||
                                 m_lnBodyActiveSources !=
                                   skinData.lnBodyActiveSources ||
                                 m_autoNoteSources != skinData.autoNoteSources ||
                                 m_autoMineSources != skinData.autoMineSources ||
                                 m_autoLnStartSources != skinData.autoLnStartSources ||
                                 m_autoLnEndSources != skinData.autoLnEndSources ||
                                 m_autoLnBodySources != skinData.autoLnBodySources ||
                                 m_autoLnBodyActiveSources !=
                                   skinData.autoLnBodyActiveSources ||
                                 m_noteDsts != skinData.noteDsts ||
                                 m_lineSources != skinData.lineSources ||
                                 m_lineDsts != skinData.lineDsts;
    m_startInput = skinData.startInput;
    m_sceneTime = skinData.sceneTime;
    m_loadStart = skinData.loadStart;
    m_loadEnd = skinData.loadEnd;
    m_playStart = skinData.playStart;
    m_fadeOut = skinData.fadeOut;
    m_skip = skinData.skip;
    m_skinWidth = skinData.skinWidth;
    m_skinHeight = skinData.skinHeight;
    m_effectiveActiveOptions = skinData.activeOptions;
    m_usedOptions = skinData.usedOptions;
    m_barLampVariants = skinData.barLampVariants;
    m_barRows = skinData.barRows;
    m_helpFiles = skinData.helpFiles;
    m_mouseCursor = mouseCursor;
    m_hasMouseHover = hasMouseHover;
    m_transColor = skinData.transColor;
    m_hasTransColor = skinData.hasTransColor;
    m_reloadBanner = skinData.reloadBanner;
    m_barCenter = skinData.barCenter;
    m_barAvailableStart = skinData.barAvailableStart;
    m_barAvailableEnd = skinData.barAvailableEnd;
    m_noteSources = skinData.noteSources;
    m_mineSources = skinData.mineSources;
    m_lnStartSources = skinData.lnStartSources;
    m_lnEndSources = skinData.lnEndSources;
    m_lnBodySources = skinData.lnBodySources;
    m_lnBodyActiveSources = skinData.lnBodyActiveSources;
    m_autoNoteSources = skinData.autoNoteSources;
    m_autoMineSources = skinData.autoMineSources;
    m_autoLnStartSources = skinData.autoLnStartSources;
    m_autoLnEndSources = skinData.autoLnEndSources;
    m_autoLnBodySources = skinData.autoLnBodySources;
    m_autoLnBodyActiveSources = skinData.autoLnBodyActiveSources;
    m_noteDsts = skinData.noteDsts;
    m_lineSources = skinData.lineSources;
    m_lineDsts = skinData.lineDsts;
    endResetModel();
    if (metadataChanged) {
        emit skinMetadataChanged();
    }
    emit skinLoaded();
}

} // namespace gameplay_logic::lr2_skin
