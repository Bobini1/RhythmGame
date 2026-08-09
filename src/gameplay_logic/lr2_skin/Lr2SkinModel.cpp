#include "Lr2SkinModel.h"

#include <QSet>
#include <QtConcurrentRun>
#include <algorithm>
#include <cstdlib>

namespace gameplay_logic::lr2_skin {

namespace {

struct ChartAssetSourceUsage
{
    bool stageFile = false;
    bool backBmp = false;
    bool banner = false;

    [[nodiscard]] bool allKnown() const
    {
        return stageFile && backBmp && banner;
    }
};

void
includeChartAssetSpecialType(ChartAssetSourceUsage& usage, int specialType)
{
    switch (specialType) {
        case Lr2SrcImage::StageFile:
            usage.stageFile = true;
            break;
        case Lr2SrcImage::BackBmp:
            usage.backBmp = true;
            break;
        case Lr2SrcImage::Banner:
            usage.banner = true;
            break;
        default:
            break;
    }
}

void
includeChartAssetSourceUsage(ChartAssetSourceUsage& usage,
                             const QVariant& source,
                             int depth = 0);

void
includeChartAssetListUsage(ChartAssetSourceUsage& usage,
                           const QVariantList& sources,
                           int depth)
{
    for (const QVariant& child : sources) {
        includeChartAssetSourceUsage(usage, child, depth + 1);
        if (usage.allKnown()) {
            return;
        }
    }
}

void
includeChartAssetMapUsage(ChartAssetSourceUsage& usage,
                          const QVariantMap& source,
                          int depth)
{
    includeChartAssetSpecialType(
      usage, source.value(QStringLiteral("specialType")).toInt());
    includeChartAssetSourceUsage(
      usage, source.value(QStringLiteral("source")), depth + 1);
    includeChartAssetSourceUsage(
      usage, source.value(QStringLiteral("sources")), depth + 1);
    includeChartAssetSourceUsage(
      usage, source.value(QStringLiteral("imageSetSources")), depth + 1);
}

void
includeChartAssetSourceUsage(ChartAssetSourceUsage& usage,
                             const QVariant& source,
                             int depth)
{
    if (!source.isValid() || usage.allKnown() || depth > 4) {
        return;
    }

    if (source.canConvert<Lr2SrcImage>()) {
        const Lr2SrcImage image = source.value<Lr2SrcImage>();
        includeChartAssetSpecialType(usage, image.specialType);
        includeChartAssetListUsage(usage, image.imageSetSources, depth + 1);
        return;
    }
    if (source.canConvert<Lr2SrcBarGraph>()) {
        includeChartAssetSpecialType(
          usage, source.value<Lr2SrcBarGraph>().specialType);
        return;
    }
    if (source.canConvert<Lr2SrcBarImage>()) {
        const Lr2SrcBarImage barImage = source.value<Lr2SrcBarImage>();
        includeChartAssetSourceUsage(usage, barImage.source, depth + 1);
        includeChartAssetListUsage(usage, barImage.sources, depth + 1);
        return;
    }
    if (source.canConvert<Lr2SrcBarNumber>()) {
        includeChartAssetSourceUsage(
          usage, source.value<Lr2SrcBarNumber>().source, depth + 1);
        return;
    }
    if (source.canConvert<QVariantList>()) {
        includeChartAssetListUsage(usage, source.toList(), depth + 1);
        return;
    }
    if (source.canConvert<QVariantMap>()) {
        includeChartAssetMapUsage(usage, source.toMap(), depth + 1);
    }
}

ChartAssetSourceUsage
chartAssetSourceUsageForElements(const QList<Lr2Element>& elements)
{
    ChartAssetSourceUsage usage;
    for (const Lr2Element& element : elements) {
        includeChartAssetSourceUsage(usage, element.src);
        if (usage.allKnown()) {
            break;
        }
    }
    return usage;
}

bool
hasSelectChartRendererElement(const QList<Lr2Element>& elements)
{
    return std::any_of(
      elements.cbegin(), elements.cend(), [](const Lr2Element& element) {
          return element.type == 11 || element.type == 12;
      });
}

bool
selectNumberUsesDifficultyState(int num)
{
    return num >= 45 && num <= 49;
}

bool
selectBarGraphUsesDifficultyState(int graphType)
{
    return graphType >= 5 && graphType <= 9;
}

bool
selectButtonUsesDifficultyState(bool button, int buttonId)
{
    return button && buttonId >= 91 && buttonId <= 96;
}

bool
sourceUsesSelectDifficultyState(const QVariant& source, int depth = 0);

bool
sourceListUsesSelectDifficultyState(const QVariantList& sources, int depth)
{
    for (const QVariant& child : sources) {
        if (sourceUsesSelectDifficultyState(child, depth + 1)) {
            return true;
        }
    }
    return false;
}

bool
sourceMapUsesSelectDifficultyState(const QVariantMap& source, int depth)
{
    if (selectNumberUsesDifficultyState(
          source.value(QStringLiteral("num")).toInt())) {
        return true;
    }
    if (selectBarGraphUsesDifficultyState(
          source.value(QStringLiteral("graphType")).toInt())) {
        return true;
    }
    if (selectButtonUsesDifficultyState(
          source.value(QStringLiteral("button")).toBool(),
          source.value(QStringLiteral("buttonId")).toInt())) {
        return true;
    }
    return sourceUsesSelectDifficultyState(
             source.value(QStringLiteral("source")), depth + 1) ||
           sourceUsesSelectDifficultyState(
             source.value(QStringLiteral("sources")), depth + 1) ||
           sourceUsesSelectDifficultyState(
             source.value(QStringLiteral("imageSetSources")), depth + 1);
}

bool
sourceUsesSelectDifficultyState(const QVariant& source, int depth)
{
    if (!source.isValid() || depth > 4) {
        return false;
    }

    if (source.canConvert<Lr2SrcNumber>()) {
        return selectNumberUsesDifficultyState(
          source.value<Lr2SrcNumber>().num);
    }
    if (source.canConvert<Lr2SrcBarGraph>()) {
        return selectBarGraphUsesDifficultyState(
          source.value<Lr2SrcBarGraph>().graphType);
    }
    if (source.canConvert<Lr2SrcImage>()) {
        const Lr2SrcImage image = source.value<Lr2SrcImage>();
        return selectButtonUsesDifficultyState(image.button, image.buttonId) ||
               sourceListUsesSelectDifficultyState(image.imageSetSources,
                                                   depth + 1);
    }
    if (source.canConvert<Lr2SrcBarImage>()) {
        const Lr2SrcBarImage barImage = source.value<Lr2SrcBarImage>();
        return sourceUsesSelectDifficultyState(barImage.source, depth + 1) ||
               sourceListUsesSelectDifficultyState(barImage.sources, depth + 1);
    }
    if (source.canConvert<Lr2SrcBarNumber>()) {
        return sourceUsesSelectDifficultyState(
          source.value<Lr2SrcBarNumber>().source, depth + 1);
    }
    if (source.canConvert<QVariantList>()) {
        return sourceListUsesSelectDifficultyState(source.toList(), depth + 1);
    }
    if (source.canConvert<QVariantMap>()) {
        return sourceMapUsesSelectDifficultyState(source.toMap(), depth + 1);
    }
    return false;
}

bool
hasSelectDifficultySourceElement(const QList<Lr2Element>& elements)
{
    return std::any_of(
      elements.cbegin(), elements.cend(), [](const Lr2Element& element) {
          return sourceUsesSelectDifficultyState(element.src);
      });
}

bool
hasNowJudgeMaxGaugeVariant(const QList<Lr2Element>& elements, const int side)
{
    return std::ranges::any_of(elements, [side](const Lr2Element& element) {
        if (element.type != 0 || !element.src.canConvert<Lr2SrcImage>()) {
            return false;
        }
        const auto source = element.src.value<Lr2SrcImage>();
        return !element.dsts.isEmpty() && source.nowJudge &&
               source.side == side && source.judgementIndex == 6;
    });
}

QSet<int>
optionIdSet(const QVariantList& options)
{
    QSet<int> result;
    for (const QVariant& option : options) {
        bool ok = false;
        const int optionId = option.toInt(&ok);
        if (ok && optionId != 0) {
            result.insert(std::abs(optionId));
        }
    }
    return result;
}

bool
activeOptionChangeAffectsStructure(const QVariantList& before,
                                   const QVariantList& after,
                                   const QVariantList& conditionOptions)
{
    if (conditionOptions.isEmpty()) {
        return false;
    }

    const QSet<int> beforeIds = optionIdSet(before);
    const QSet<int> afterIds = optionIdSet(after);
    for (const int optionId : optionIdSet(conditionOptions)) {
        if (beforeIds.contains(optionId) != afterIds.contains(optionId)) {
            return true;
        }
    }
    return false;
}

int
firstSortId(const QVariantList& dsts)
{
    if (dsts.isEmpty()) {
        return 0;
    }

    const QVariant& first = dsts.first();
    return first.canConvert<Lr2Dst>() ? first.value<Lr2Dst>().sortId : 0;
}

int
staticNoteElementSortId(const QVariantList& noteDsts)
{
    int result = -1;
    for (const QVariant& laneDstsValue : noteDsts) {
        const auto laneDsts = laneDstsValue.toList();
        if (laneDsts.isEmpty()) {
            continue;
        }
        const int sortId = firstSortId(laneDsts);
        result = result < 0 || sortId < result ? sortId : result;
    }
    return result >= 0 ? result : 0;
}

bool
isSelectBarElement(const Lr2Element& element)
{
    return element.type == 4 || element.type == 5 || element.type == 13 ||
           (element.type == 3 && element.src.canConvert<Lr2SrcBarImage>());
}

double
selectBarElementLayer(const Lr2Element& element)
{
    if (element.type == 13) {
        return 0.20;
    }
    if (element.type == 4) {
        return 0.30;
    }
    if (element.type == 5) {
        return 0.60;
    }
    if (element.type != 3 || !element.src.canConvert<Lr2SrcBarImage>()) {
        return 0.0;
    }

    switch (element.src.value<Lr2SrcBarImage>().kind) {
        case Lr2SrcBarImage::BodyOff:
        case Lr2SrcBarImage::BodyOn:
            return 0.0;
        case Lr2SrcBarImage::Flash:
            return 0.10;
        case Lr2SrcBarImage::Lamp:
        case Lr2SrcBarImage::MyLamp:
        case Lr2SrcBarImage::RivalLamp:
            return 0.50;
        case Lr2SrcBarImage::Rank:
        case Lr2SrcBarImage::Rival:
            return 0.65;
        case Lr2SrcBarImage::Label:
            return 0.70;
        default:
            return 0.40;
    }
}

double
elementSortKey(const Lr2Element& element, int noteElementSortId)
{
    if (isSelectBarElement(element)) {
        return firstSortId(element.dsts) +
               selectBarElementLayer(element) * 0.001;
    }

    return element.type == 8 ? noteElementSortId : firstSortId(element.dsts);
}

void
sortElementsByDrawOrder(QList<Lr2Element>& elements,
                        const QVariantList& noteDsts)
{
    const int noteElementSortId = staticNoteElementSortId(noteDsts);
    std::stable_sort(
      elements.begin(),
      elements.end(),
      [noteElementSortId](const Lr2Element& lhs, const Lr2Element& rhs) {
          return elementSortKey(lhs, noteElementSortId) <
                 elementSortKey(rhs, noteElementSortId);
      });
}

QVariantMap
findMouseCursorElement(const QList<Lr2Element>& elements)
{
    for (const auto& element : elements) {
        if (element.type != 0 || !element.src.canConvert<Lr2SrcImage>()) {
            continue;
        }

        const auto src = element.src.value<Lr2SrcImage>();
        if (!src.mouseCursor) {
            continue;
        }

        return { { "src", QVariant::fromValue(src) },
                 { "dsts", element.dsts } };
    }

    return {};
}

bool
hasMouseHoverElement(const QList<Lr2Element>& elements)
{
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

int
scratchRotationSidesForElements(const QList<Lr2Element>& elements)
{
    int result = 0;
    for (const auto& element : elements) {
        if (element.type != 0 || element.dsts.isEmpty()) {
            continue;
        }
        const auto& dstValue = element.dsts.first();
        if (!dstValue.canConvert<Lr2Dst>()) {
            continue;
        }
        const auto dst = dstValue.value<Lr2Dst>();
        if (dst.op4 == 1 || dst.op4 == 2) {
            result |= dst.op4;
            if (result == 3) {
                return result;
            }
        }
    }
    return result;
}

}

Lr2SkinModel::Lr2SkinModel(QObject* parent)
  : QAbstractListModel(parent)
{
    connect(&m_loadWatcher,
            &QFutureWatcher<Lr2SkinData>::finished,
            this,
            &Lr2SkinModel::handleAsyncLoadFinished);
}

Lr2SkinModel::~Lr2SkinModel()
{
    disconnect(&m_loadWatcher, nullptr, this, nullptr);
    if (m_loadWatcher.isRunning()) {
        m_loadWatcher.waitForFinished();
    }
}

int
Lr2SkinModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_elements.size();
}

QVariant
Lr2SkinModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_elements.size())
        return QVariant();

    const auto& item = m_elements[index.row()];
    switch (role) {
        case TypeRole:
            return item.type;
        case SrcRole:
            return item.src;
        case DstsRole:
            return item.dsts;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray>
Lr2SkinModel::roleNames() const
{
    return { { TypeRole, "type" }, { SrcRole, "src" }, { DstsRole, "dsts" } };
}

QString
Lr2SkinModel::csvPath() const
{
    return m_csvPath;
}

QVariantMap
Lr2SkinModel::settingValues() const
{
    return m_settingValues;
}

QVariantList
Lr2SkinModel::activeOptions() const
{
    return m_activeOptions;
}

int
Lr2SkinModel::startInput() const
{
    return m_startInput;
}

int
Lr2SkinModel::sceneTime() const
{
    return m_sceneTime;
}

int
Lr2SkinModel::loadStart() const
{
    return m_loadStart;
}

int
Lr2SkinModel::loadEnd() const
{
    return m_loadEnd;
}

int
Lr2SkinModel::playStart() const
{
    return m_playStart;
}

int
Lr2SkinModel::fadeOut() const
{
    return m_fadeOut;
}

int
Lr2SkinModel::finishMargin() const
{
    return m_finishMargin;
}

int
Lr2SkinModel::skip() const
{
    return m_skip;
}

int
Lr2SkinModel::skinWidth() const
{
    return m_skinWidth;
}

int
Lr2SkinModel::skinHeight() const
{
    return m_skinHeight;
}

QVariantList
Lr2SkinModel::effectiveActiveOptions() const
{
    return m_effectiveActiveOptions;
}

QVariantList
Lr2SkinModel::usedOptions() const
{
    return m_usedOptions;
}

QVariantList
Lr2SkinModel::usedElementOptions() const
{
    return m_usedElementOptions;
}

QVariantList
Lr2SkinModel::conditionOptions() const
{
    return m_conditionOptions;
}

QVariantList
Lr2SkinModel::barLampVariants() const
{
    return m_barLampVariants;
}

QVariantList
Lr2SkinModel::barLevelVariants() const
{
    return m_barLevelVariants;
}

QVariantList
Lr2SkinModel::barRows() const
{
    return m_barRows;
}

QVariantList
Lr2SkinModel::barBodyTypes() const
{
    return m_barBodyTypes;
}

QVariantList
Lr2SkinModel::barTitleTypes() const
{
    return m_barTitleTypes;
}

QVariantList
Lr2SkinModel::helpFiles() const
{
    return m_helpFiles;
}

QVariantMap
Lr2SkinModel::mouseCursor() const
{
    return m_mouseCursor;
}

bool
Lr2SkinModel::hasMouseHover() const
{
    return m_hasMouseHover;
}

int
Lr2SkinModel::scratchRotationSides() const
{
    return m_scratchRotationSides;
}

QString
Lr2SkinModel::transColor() const
{
    return m_transColor;
}

bool
Lr2SkinModel::hasTransColor() const
{
    return m_hasTransColor;
}

QString
Lr2SkinModel::laneCoverSource() const
{
    return m_laneCoverSource;
}

bool
Lr2SkinModel::reloadBanner() const
{
    return m_reloadBanner;
}

bool
Lr2SkinModel::usesStageFileSource() const
{
    return m_usesStageFileSource;
}

bool
Lr2SkinModel::usesBackBmpSource() const
{
    return m_usesBackBmpSource;
}

bool
Lr2SkinModel::usesBannerSource() const
{
    return m_usesBannerSource;
}

bool
Lr2SkinModel::usesSelectChartRenderer() const
{
    return m_usesSelectChartRenderer;
}

bool
Lr2SkinModel::usesSelectDifficultySource() const
{
    return m_usesSelectDifficultySource;
}

bool
Lr2SkinModel::hasNowJudgeMaxGaugeVariant1() const
{
    return m_hasNowJudgeMaxGaugeVariant1;
}

bool
Lr2SkinModel::hasNowJudgeMaxGaugeVariant2() const
{
    return m_hasNowJudgeMaxGaugeVariant2;
}

int
Lr2SkinModel::barCenter() const
{
    return m_barCenter;
}

int
Lr2SkinModel::barAvailableStart() const
{
    return m_barAvailableStart;
}

int
Lr2SkinModel::barAvailableEnd() const
{
    return m_barAvailableEnd;
}

QVariantList
Lr2SkinModel::noteSources() const
{
    return m_noteSources;
}

QVariantList
Lr2SkinModel::mineSources() const
{
    return m_mineSources;
}

QVariantList
Lr2SkinModel::lnStartSources() const
{
    return m_lnStartSources;
}

QVariantList
Lr2SkinModel::lnEndSources() const
{
    return m_lnEndSources;
}

QVariantList
Lr2SkinModel::lnBodySources() const
{
    return m_lnBodySources;
}

QVariantList
Lr2SkinModel::lnBodyActiveSources() const
{
    return m_lnBodyActiveSources;
}

QVariantList
Lr2SkinModel::autoNoteSources() const
{
    return m_autoNoteSources;
}

QVariantList
Lr2SkinModel::autoMineSources() const
{
    return m_autoMineSources;
}

QVariantList
Lr2SkinModel::autoLnStartSources() const
{
    return m_autoLnStartSources;
}

QVariantList
Lr2SkinModel::autoLnEndSources() const
{
    return m_autoLnEndSources;
}

QVariantList
Lr2SkinModel::autoLnBodySources() const
{
    return m_autoLnBodySources;
}

QVariantList
Lr2SkinModel::autoLnBodyActiveSources() const
{
    return m_autoLnBodyActiveSources;
}

QVariantList
Lr2SkinModel::noteDsts() const
{
    return m_noteDsts;
}

QVariantList
Lr2SkinModel::lineSources() const
{
    return m_lineSources;
}

QVariantList
Lr2SkinModel::lineDsts() const
{
    return m_lineDsts;
}

void
Lr2SkinModel::setCsvPath(const QString& path)
{
    if (m_csvPath == path)
        return;
    m_csvPath = path;
    emit csvPathChanged();
    loadSkin();
}

void
Lr2SkinModel::setSettingValues(const QVariantMap& values)
{
    if (m_settingValues == values)
        return;
    m_settingValues = values;
    emit settingValuesChanged();
    if (m_hasLoadedSkin) {
        requestAsyncLoad();
    } else {
        loadSkin();
    }
}

void
Lr2SkinModel::setActiveOptions(const QVariantList& options)
{
    if (m_activeOptions == options)
        return;
    const bool needsReload =
      !m_hasLoadedSkin || activeOptionChangeAffectsStructure(
                            m_activeOptions, options, m_conditionOptions);
    m_activeOptions = options;
    emit activeOptionsChanged();
    if (needsReload || m_loadActive) {
        if (m_hasLoadedSkin) {
            requestAsyncLoad();
        } else {
            loadSkin();
        }
    }
}

void
Lr2SkinModel::loadSkin()
{
    ++m_requestedLoadGeneration;
    m_reloadPending = false;
    m_definition = {};

    if (m_csvPath.isEmpty()) {
        beginResetModel();
        m_elements.clear();
        m_hasLoadedSkin = false;
        const bool metadataChanged =
          !m_effectiveActiveOptions.isEmpty() || !m_usedOptions.isEmpty() ||
          !m_usedElementOptions.isEmpty() || !m_conditionOptions.isEmpty() ||
          !m_barLampVariants.isEmpty() || !m_barLevelVariants.isEmpty() ||
          !m_barRows.isEmpty() || !m_barBodyTypes.isEmpty() ||
          !m_barTitleTypes.isEmpty() || m_startInput != 0 || m_sceneTime != 0 ||
          m_loadStart != 0 || m_loadEnd != 0 || m_playStart != 2000 ||
          m_fadeOut != 0 || m_finishMargin != 0 || m_skip != 0 ||
          m_skinWidth != 640 || m_skinHeight != 480 || !m_helpFiles.isEmpty() ||
          !m_mouseCursor.isEmpty() || m_hasMouseHover ||
          m_scratchRotationSides != 0 || m_transColor != "#000000" ||
          m_hasTransColor || m_reloadBanner || m_usesStageFileSource ||
          m_usesBackBmpSource || m_usesBannerSource ||
          m_usesSelectChartRenderer || m_usesSelectDifficultySource ||
          m_hasNowJudgeMaxGaugeVariant1 || m_hasNowJudgeMaxGaugeVariant2 ||
          m_barCenter != 0 || m_barAvailableStart != 0 ||
          m_barAvailableEnd != -1 || !m_noteSources.isEmpty() ||
          !m_mineSources.isEmpty() || !m_lnStartSources.isEmpty() ||
          !m_lnEndSources.isEmpty() || !m_lnBodySources.isEmpty() ||
          !m_lnBodyActiveSources.isEmpty() || !m_autoNoteSources.isEmpty() ||
          !m_autoMineSources.isEmpty() || !m_autoLnStartSources.isEmpty() ||
          !m_autoLnEndSources.isEmpty() || !m_autoLnBodySources.isEmpty() ||
          !m_autoLnBodyActiveSources.isEmpty() || !m_noteDsts.isEmpty() ||
          !m_lineSources.isEmpty() || !m_lineDsts.isEmpty();
        m_effectiveActiveOptions.clear();
        m_usedOptions.clear();
        m_usedElementOptions.clear();
        m_conditionOptions.clear();
        m_barLampVariants.clear();
        m_barLevelVariants.clear();
        m_barRows.clear();
        m_barBodyTypes.clear();
        m_barTitleTypes.clear();
        m_helpFiles.clear();
        m_mouseCursor.clear();
        m_hasMouseHover = false;
        m_scratchRotationSides = 0;
        m_transColor = "#000000";
        m_hasTransColor = false;
        m_laneCoverSource.clear();
        m_reloadBanner = false;
        m_usesStageFileSource = false;
        m_usesBackBmpSource = false;
        m_usesBannerSource = false;
        m_usesSelectChartRenderer = false;
        m_usesSelectDifficultySource = false;
        m_hasNowJudgeMaxGaugeVariant1 = false;
        m_hasNowJudgeMaxGaugeVariant2 = false;
        m_startInput = 0;
        m_sceneTime = 0;
        m_loadStart = 0;
        m_loadEnd = 0;
        m_playStart = 2000;
        m_fadeOut = 0;
        m_finishMargin = 0;
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

    m_definition = Lr2SkinParser::compile(m_csvPath);
    applySkinData(Lr2SkinParser::materialize(
      m_definition, m_settingValues, m_activeOptions));
}

void
Lr2SkinModel::requestAsyncLoad()
{
    ++m_requestedLoadGeneration;
    if (!m_definition.isValid()) {
        loadSkin();
        return;
    }
    if (m_loadActive) {
        m_reloadPending = true;
        return;
    }
    startAsyncLoad();
}

void
Lr2SkinModel::startAsyncLoad()
{
    if (!m_definition.isValid()) {
        return;
    }

    m_reloadPending = false;
    m_runningLoadGeneration = m_requestedLoadGeneration;
    m_loadActive = true;
    const auto definition = m_definition;
    const auto settingValues = m_settingValues;
    const auto activeOptions = m_activeOptions;
    m_loadWatcher.setFuture(
      QtConcurrent::run([definition, settingValues, activeOptions] {
          return Lr2SkinParser::materialize(
            definition, settingValues, activeOptions);
      }));
}

void
Lr2SkinModel::handleAsyncLoadFinished()
{
    auto skinData = m_loadWatcher.result();
    m_loadActive = false;
    if (m_runningLoadGeneration == m_requestedLoadGeneration) {
        applySkinData(std::move(skinData));
    }
    if (m_reloadPending) {
        startAsyncLoad();
    }
}

void
Lr2SkinModel::applySkinData(Lr2SkinData skinData)
{
    auto elements = skinData.elements;
    sortElementsByDrawOrder(elements, skinData.noteDsts);
    const auto mouseCursor = findMouseCursorElement(skinData.elements);
    const bool hasMouseHover = hasMouseHoverElement(skinData.elements);
    const int scratchRotationSides =
      scratchRotationSidesForElements(skinData.elements);
    const ChartAssetSourceUsage chartAssetUsage =
      chartAssetSourceUsageForElements(skinData.elements);
    const bool usesSelectChartRenderer =
      hasSelectChartRendererElement(skinData.elements);
    const bool usesSelectDifficultySource =
      hasSelectDifficultySourceElement(skinData.elements);
    const bool hasNowJudgeMaxGaugeVariant1 =
      hasNowJudgeMaxGaugeVariant(skinData.elements, 1);
    const bool hasNowJudgeMaxGaugeVariant2 =
      hasNowJudgeMaxGaugeVariant(skinData.elements, 2);
    const bool metadataChanged =
      m_startInput != skinData.startInput ||
      m_sceneTime != skinData.sceneTime || m_loadStart != skinData.loadStart ||
      m_loadEnd != skinData.loadEnd || m_playStart != skinData.playStart ||
      m_fadeOut != skinData.fadeOut ||
      m_finishMargin != skinData.finishMargin || m_skip != skinData.skip ||
      m_skinWidth != skinData.skinWidth ||
      m_skinHeight != skinData.skinHeight ||
      m_effectiveActiveOptions != skinData.activeOptions ||
      m_usedOptions != skinData.usedOptions ||
      m_usedElementOptions != skinData.usedElementOptions ||
      m_conditionOptions != skinData.conditionOptions ||
      m_barLampVariants != skinData.barLampVariants ||
      m_barLevelVariants != skinData.barLevelVariants ||
      m_barRows != skinData.barRows ||
      m_barBodyTypes != skinData.barBodyTypes ||
      m_barTitleTypes != skinData.barTitleTypes ||
      m_helpFiles != skinData.helpFiles || m_mouseCursor != mouseCursor ||
      m_hasMouseHover != hasMouseHover ||
      m_scratchRotationSides != scratchRotationSides ||
      m_transColor != skinData.transColor ||
      m_hasTransColor != skinData.hasTransColor ||
      m_laneCoverSource != skinData.laneCoverSource ||
      m_reloadBanner != skinData.reloadBanner ||
      m_usesStageFileSource != chartAssetUsage.stageFile ||
      m_usesBackBmpSource != chartAssetUsage.backBmp ||
      m_usesBannerSource != chartAssetUsage.banner ||
      m_usesSelectChartRenderer != usesSelectChartRenderer ||
      m_usesSelectDifficultySource != usesSelectDifficultySource ||
      m_hasNowJudgeMaxGaugeVariant1 != hasNowJudgeMaxGaugeVariant1 ||
      m_hasNowJudgeMaxGaugeVariant2 != hasNowJudgeMaxGaugeVariant2 ||
      m_barCenter != skinData.barCenter ||
      m_barAvailableStart != skinData.barAvailableStart ||
      m_barAvailableEnd != skinData.barAvailableEnd ||
      m_noteSources != skinData.noteSources ||
      m_mineSources != skinData.mineSources ||
      m_lnStartSources != skinData.lnStartSources ||
      m_lnEndSources != skinData.lnEndSources ||
      m_lnBodySources != skinData.lnBodySources ||
      m_lnBodyActiveSources != skinData.lnBodyActiveSources ||
      m_autoNoteSources != skinData.autoNoteSources ||
      m_autoMineSources != skinData.autoMineSources ||
      m_autoLnStartSources != skinData.autoLnStartSources ||
      m_autoLnEndSources != skinData.autoLnEndSources ||
      m_autoLnBodySources != skinData.autoLnBodySources ||
      m_autoLnBodyActiveSources != skinData.autoLnBodyActiveSources ||
      m_noteDsts != skinData.noteDsts ||
      m_lineSources != skinData.lineSources || m_lineDsts != skinData.lineDsts;
    m_startInput = skinData.startInput;
    m_sceneTime = skinData.sceneTime;
    m_loadStart = skinData.loadStart;
    m_loadEnd = skinData.loadEnd;
    m_playStart = skinData.playStart;
    m_fadeOut = skinData.fadeOut;
    m_finishMargin = skinData.finishMargin;
    m_skip = skinData.skip;
    m_skinWidth = skinData.skinWidth;
    m_skinHeight = skinData.skinHeight;
    m_effectiveActiveOptions = skinData.activeOptions;
    m_usedOptions = skinData.usedOptions;
    m_usedElementOptions = skinData.usedElementOptions;
    m_conditionOptions = skinData.conditionOptions;
    m_barLampVariants = skinData.barLampVariants;
    m_barLevelVariants = skinData.barLevelVariants;
    m_barRows = skinData.barRows;
    m_barBodyTypes = skinData.barBodyTypes;
    m_barTitleTypes = skinData.barTitleTypes;
    m_helpFiles = skinData.helpFiles;
    m_mouseCursor = mouseCursor;
    m_hasMouseHover = hasMouseHover;
    m_scratchRotationSides = scratchRotationSides;
    m_transColor = skinData.transColor;
    m_hasTransColor = skinData.hasTransColor;
    m_laneCoverSource = skinData.laneCoverSource;
    m_reloadBanner = skinData.reloadBanner;
    m_usesStageFileSource = chartAssetUsage.stageFile;
    m_usesBackBmpSource = chartAssetUsage.backBmp;
    m_usesBannerSource = chartAssetUsage.banner;
    m_usesSelectChartRenderer = usesSelectChartRenderer;
    m_usesSelectDifficultySource = usesSelectDifficultySource;
    m_hasNowJudgeMaxGaugeVariant1 = hasNowJudgeMaxGaugeVariant1;
    m_hasNowJudgeMaxGaugeVariant2 = hasNowJudgeMaxGaugeVariant2;
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
    applyElements(std::move(elements));
    if (metadataChanged) {
        emit skinMetadataChanged();
    }
    m_hasLoadedSkin = true;
    emit skinLoaded();
}

void
Lr2SkinModel::applyElements(QList<Lr2Element> elements)
{
    const int oldSize = static_cast<int>(m_elements.size());
    const int newSize = static_cast<int>(elements.size());
    const int commonSize = std::min(oldSize, newSize);
    int firstChangedRow = -1;
    int lastChangedRow = -1;

    for (int row = 0; row < commonSize; ++row) {
        if (m_elements[row] == elements[row]) {
            continue;
        }
        m_elements[row] = std::move(elements[row]);
        if (firstChangedRow < 0) {
            firstChangedRow = row;
        }
        lastChangedRow = row;
    }
    if (firstChangedRow >= 0) {
        emit dataChanged(index(firstChangedRow),
                         index(lastChangedRow),
                         { TypeRole, SrcRole, DstsRole });
    }

    if (newSize < oldSize) {
        beginRemoveRows({}, newSize, oldSize - 1);
        m_elements.erase(m_elements.begin() + newSize, m_elements.end());
        endRemoveRows();
    } else if (newSize > oldSize) {
        beginInsertRows({}, oldSize, newSize - 1);
        m_elements.reserve(newSize);
        for (int row = oldSize; row < newSize; ++row) {
            m_elements.append(std::move(elements[row]));
        }
        endInsertRows();
    }
}

} // namespace gameplay_logic::lr2_skin
