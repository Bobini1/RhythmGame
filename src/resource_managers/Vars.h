//
// Created by bobini on 05.04.24.
//

#ifndef RHYTHMGAME_VARS_H
#define RHYTHMGAME_VARS_H

#include "qml_components/ThemeFamily.h"

#include <QObject>
#include <QQmlPropertyMap>
#include <filesystem>

namespace qml_components {
class ProfileList;
} // namespace qml_components

namespace resource_managers {
class Profile;

namespace note_order_algorithm {
Q_NAMESPACE
enum class NoteOrderAlgorithm
{
    Normal,
    Mirror,
    Random,
    SRandom,
    HRandom,
    RRandom,
    RandomPlus,
    SRandomPlus
};
Q_ENUM_NS(NoteOrderAlgorithm)
} // namespace note_order_algorithm
using namespace note_order_algorithm;

namespace hi_speed_fix {
Q_NAMESPACE
enum class HiSpeedFix
{
    Off,
    Main,
    Start,
    Min,
    Max
};
Q_ENUM_NS(HiSpeedFix)
} // namespace hi_speed_fix
using namespace hi_speed_fix;

namespace dp_options {
Q_NAMESPACE
enum class DpOptions
{
    Off,
    Flip,
    Battle,
    BattleAs
};
Q_ENUM_NS(DpOptions)
} // namespace dp_options
using namespace dp_options;

namespace gauge_mode {
Q_NAMESPACE
enum class GaugeMode
{
    Exclusive,
    Best,
    SelectToUnder
};
Q_ENUM_NS(GaugeMode)
} // namespace gauge_mode
using namespace gauge_mode;

class GlobalVars final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int noteScreenTimeMillis READ getNoteScreenTimeMillis WRITE
                 setNoteScreenTimeMillis NOTIFY noteScreenTimeMillisChanged)
    Q_PROPERTY(bool laneCoverOn READ getLaneCoverOn WRITE setLaneCoverOn NOTIFY
                 laneCoverOnChanged)
    Q_PROPERTY(double laneCoverRatio READ getLaneCoverRatio WRITE
                 setLaneCoverRatio NOTIFY laneCoverRatioChanged)
    Q_PROPERTY(bool liftOn READ getLiftOn WRITE setLiftOn NOTIFY liftOnChanged)
    Q_PROPERTY(double liftRatio READ getLiftRatio WRITE setLiftRatio NOTIFY
                 liftRatioChanged)
    Q_PROPERTY(
      bool hiddenOn READ getHiddenOn WRITE setHiddenOn NOTIFY hiddenOnChanged)
    Q_PROPERTY(double hiddenRatio READ getHiddenRatio WRITE setHiddenRatio
                 NOTIFY hiddenRatioChanged)
    Q_PROPERTY(bool bgaOn READ getBgaOn WRITE setBgaOn NOTIFY bgaOnChanged)
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithm READ getNoteOrderAlgorithm WRITE
                   setNoteOrderAlgorithm NOTIFY noteOrderAlgorithmChanged)
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithmP2 READ getNoteOrderAlgorithmP2 WRITE
                   setNoteOrderAlgorithmP2 NOTIFY noteOrderAlgorithmP2Changed)
    Q_PROPERTY(resource_managers::hi_speed_fix::HiSpeedFix hiSpeedFix READ
                 getHiSpeedFix WRITE setHiSpeedFix NOTIFY hiSpeedFixChanged)
    Q_PROPERTY(resource_managers::dp_options::DpOptions dpOptions READ
                 getDpOptions WRITE setDpOptions NOTIFY dpOptionsChanged)
    Q_PROPERTY(QString gaugeType READ getGaugeType WRITE setGaugeType NOTIFY
                 gaugeTypeChanged)
    Q_PROPERTY(resource_managers::gauge_mode::GaugeMode gaugeMode READ
                 getGaugeMode WRITE setGaugeMode NOTIFY gaugeModeChanged)
    Q_PROPERTY(QString bottomShiftableGauge READ getBottomShiftableGauge WRITE
                 setBottomShiftableGauge NOTIFY bottomShiftableGaugeChanged)
    // ^ remember to use full namespace for enums for reflection
    int noteScreenTimeMillis = 1000;
    bool laneCoverOn = false;
    double laneCoverRatio = 0.1;
    bool liftOn = false;
    double liftRatio = 0.1;
    bool hiddenOn = false;
    double hiddenRatio = 0.1;
    bool bgaOn = true;
    NoteOrderAlgorithm noteOrderAlgorithm = NoteOrderAlgorithm::Normal;
    NoteOrderAlgorithm noteOrderAlgorithmP2 = NoteOrderAlgorithm::Normal;
    HiSpeedFix hiSpeedFix = HiSpeedFix::Main;
    DpOptions dpOptions = DpOptions::Off;
    QString gaugeType = "HAZARD";
    GaugeMode gaugeMode = GaugeMode::SelectToUnder;
    QString bottomShiftableGauge = "AEASY";

  public:
    auto getNoteScreenTimeMillis() const -> int;
    void setNoteScreenTimeMillis(int value);
    auto getLaneCoverOn() const -> bool;
    void setLaneCoverOn(bool value);
    auto getLaneCoverRatio() const -> double;
    void setLaneCoverRatio(double value);
    auto getLiftOn() const -> bool;
    void setLiftOn(bool value);
    auto getLiftRatio() const -> double;
    void setLiftRatio(double value);
    auto getHiddenOn() const -> bool;
    void setHiddenOn(bool value);
    auto getHiddenRatio() const -> double;
    void setHiddenRatio(double value);
    auto getBgaOn() const -> bool;
    void setBgaOn(bool value);
    auto getNoteOrderAlgorithm() const -> NoteOrderAlgorithm;
    void setNoteOrderAlgorithm(NoteOrderAlgorithm value);
    auto getNoteOrderAlgorithmP2() const -> NoteOrderAlgorithm;
    void setNoteOrderAlgorithmP2(NoteOrderAlgorithm value);
    auto getHiSpeedFix() const -> HiSpeedFix;
    void setHiSpeedFix(HiSpeedFix value);
    auto getDpOptions() const -> DpOptions;
    void setDpOptions(DpOptions value);
    auto getGaugeType() const -> QString;
    void setGaugeType(QString value);
    auto getGaugeMode() const -> GaugeMode;
    void setGaugeMode(GaugeMode value);
    auto getBottomShiftableGauge() const -> QString;
    void setBottomShiftableGauge(QString value);

  signals:
    void noteScreenTimeMillisChanged();
    void laneCoverOnChanged();
    void laneCoverRatioChanged();
    void liftOnChanged();
    void liftRatioChanged();
    void hiddenOnChanged();
    void hiddenRatioChanged();
    void bgaOnChanged();
    void noteOrderAlgorithmChanged();
    void noteOrderAlgorithmP2Changed();
    void hiSpeedFixChanged();
    void dpOptionsChanged();
    void gaugeTypeChanged();
    void gaugeModeChanged();
    void bottomShiftableGaugeChanged();
};

class Vars final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
      GlobalVars* globalVars READ getGlobalVars NOTIFY globalVarsChanged)
    Q_PROPERTY(
      QQmlPropertyMap* themeVars READ getThemeVars NOTIFY themeVarsChanged)
    GlobalVars globalVars{};
    QQmlPropertyMap themeVars;
    const Profile* profile;
    QMap<QString, qml_components::ThemeFamily> availableThemeFamilies;
    QHash<QString, QHash<QString, QHash<QString, QVariant>>> loadedThemeVars;

    void onThemeConfigChanged(const QString& key, const QVariant& value);
    void populateThemePropertyMap(
      QQmlPropertyMap& themeVars,
      QHash<QString, QHash<QString, QHash<QString, QVariant>>> themeVarsData,
      const std::filesystem::path& themeVarsPath,
      const QQmlPropertyMap& themeConfig);
    Q_SLOT void writeGlobalVars() const;

  public:
    explicit Vars(
      const Profile* profile,
      QMap<QString, qml_components::ThemeFamily> availableThemeFamilies,
      QObject* parent = nullptr);
    auto getGlobalVars() -> GlobalVars*;
    auto getThemeVars() -> QQmlPropertyMap*;

  signals:
    void globalVarsChanged();
    void themeVarsChanged();
};
} // namespace resource_managers

#endif // RHYTHMGAME_VARS_H
