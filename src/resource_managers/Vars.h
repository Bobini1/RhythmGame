//
// Created by bobini on 05.04.24.
//

#ifndef RHYTHMGAME_VARS_H
#define RHYTHMGAME_VARS_H

#include "qml_components/ThemeFamily.h"

#include <QObject>
#include <QQmlPropertyMap>
#include <filesystem>
#include <QLocale>

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
    Max,
    Avg
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
    Battle
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

class GeneralVars final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int noteScreenTimeMillis READ getNoteScreenTimeMillis WRITE
                 setNoteScreenTimeMillis NOTIFY noteScreenTimeMillisChanged
                   RESET resetNoteScreenTimeMillis)
    Q_PROPERTY(bool laneCoverOn READ getLaneCoverOn WRITE setLaneCoverOn NOTIFY
                 laneCoverOnChanged RESET resetLaneCoverOn)
    Q_PROPERTY(
      double laneCoverRatio READ getLaneCoverRatio WRITE setLaneCoverRatio
        NOTIFY laneCoverRatioChanged RESET resetLaneCoverRatio)
    Q_PROPERTY(bool liftOn READ getLiftOn WRITE setLiftOn NOTIFY liftOnChanged
                 RESET resetLiftOn)
    Q_PROPERTY(double liftRatio READ getLiftRatio WRITE setLiftRatio NOTIFY
                 liftRatioChanged RESET resetLiftRatio)
    Q_PROPERTY(bool hiddenOn READ getHiddenOn WRITE setHiddenOn NOTIFY
                 hiddenOnChanged RESET resetHiddenOn)
    Q_PROPERTY(double hiddenRatio READ getHiddenRatio WRITE setHiddenRatio
                 NOTIFY hiddenRatioChanged RESET resetHiddenRatio)
    Q_PROPERTY(bool bgaOn READ getBgaOn WRITE setBgaOn NOTIFY bgaOnChanged RESET
                 resetBgaOn)
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithm READ getNoteOrderAlgorithm WRITE
                   setNoteOrderAlgorithm NOTIFY noteOrderAlgorithmChanged RESET
                     resetNoteOrderAlgorithm)
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithmP2 READ getNoteOrderAlgorithmP2 WRITE
                   setNoteOrderAlgorithmP2 NOTIFY noteOrderAlgorithmP2Changed
                     RESET resetNoteOrderAlgorithmP2)
    Q_PROPERTY(
      resource_managers::hi_speed_fix::HiSpeedFix hiSpeedFix READ getHiSpeedFix
        WRITE setHiSpeedFix NOTIFY hiSpeedFixChanged RESET resetHiSpeedFix)
    Q_PROPERTY(
      resource_managers::dp_options::DpOptions dpOptions READ getDpOptions WRITE
        setDpOptions NOTIFY dpOptionsChanged RESET resetDpOptions)
    Q_PROPERTY(QString gaugeType READ getGaugeType WRITE setGaugeType NOTIFY
                 gaugeTypeChanged RESET resetGaugeType)
    Q_PROPERTY(
      resource_managers::gauge_mode::GaugeMode gaugeMode READ getGaugeMode WRITE
        setGaugeMode NOTIFY gaugeModeChanged RESET resetGaugeMode)
    Q_PROPERTY(QString bottomShiftableGauge READ getBottomShiftableGauge WRITE
                 setBottomShiftableGauge NOTIFY bottomShiftableGaugeChanged
                   RESET resetBottomShiftableGauge)
    Q_PROPERTY(QString avatar READ getAvatar WRITE setAvatar NOTIFY
                 avatarChanged RESET resetAvatar)
    Q_PROPERTY(QString name READ getName WRITE setName NOTIFY nameChanged RESET
                 resetName)
    Q_PROPERTY(QString language READ getLanguage WRITE setLanguage NOTIFY
                 languageChanged RESET resetLanguage)
    Q_PROPERTY(
      double offset READ getOffset WRITE setOffset NOTIFY offsetChanged)
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
    QString gaugeType = "FC";
    GaugeMode gaugeMode = GaugeMode::SelectToUnder;
    QString bottomShiftableGauge = "AEASY";
    QString avatar = "mascot.png";
    QString name = "Default";
    QString language = QLocale::system().name();
    double offset = 0.0; // Offset in milliseconds

    QString avatarPath;

  public:
    explicit GeneralVars(QString avatarPath, QObject* parent = nullptr);
    auto getNoteScreenTimeMillis() const -> int;
    void setNoteScreenTimeMillis(int value);
    void resetNoteScreenTimeMillis();
    auto getLaneCoverOn() const -> bool;
    void setLaneCoverOn(bool value);
    void resetLaneCoverOn();
    auto getLaneCoverRatio() const -> double;
    void setLaneCoverRatio(double value);
    void resetLaneCoverRatio();
    auto getLiftOn() const -> bool;
    void setLiftOn(bool value);
    void resetLiftOn();
    auto getLiftRatio() const -> double;
    void setLiftRatio(double value);
    void resetLiftRatio();
    auto getHiddenOn() const -> bool;
    void setHiddenOn(bool value);
    void resetHiddenOn();
    auto getHiddenRatio() const -> double;
    void setHiddenRatio(double value);
    void resetHiddenRatio();
    auto getBgaOn() const -> bool;
    void setBgaOn(bool value);
    void resetBgaOn();
    auto getNoteOrderAlgorithm() const -> NoteOrderAlgorithm;
    void setNoteOrderAlgorithm(NoteOrderAlgorithm value);
    void resetNoteOrderAlgorithm();
    auto getNoteOrderAlgorithmP2() const -> NoteOrderAlgorithm;
    void setNoteOrderAlgorithmP2(NoteOrderAlgorithm value);
    void resetNoteOrderAlgorithmP2();
    auto getHiSpeedFix() const -> HiSpeedFix;
    void setHiSpeedFix(HiSpeedFix value);
    void resetHiSpeedFix();
    auto getDpOptions() const -> DpOptions;
    void setDpOptions(DpOptions value);
    void resetDpOptions();
    auto getGaugeType() const -> QString;
    void setGaugeType(QString value);
    void resetGaugeType();
    auto getGaugeMode() const -> GaugeMode;
    void setGaugeMode(GaugeMode value);
    void resetGaugeMode();
    auto getBottomShiftableGauge() const -> QString;
    void setBottomShiftableGauge(QString value);
    void resetBottomShiftableGauge();
    auto getAvatar() const -> QString;
    void setAvatar(QString value);
    void resetAvatar();
    auto getName() const -> QString;
    void setName(QString value);
    void resetName();
    auto getLanguage() const -> QString;
    void setLanguage(QString value);
    void resetLanguage();
    auto getOffset() const -> double;
    void setOffset(double value);
    void resetOffset();

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
    void avatarChanged();
    void nameChanged();
    void languageChanged();
    void offsetChanged();
};

class Vars final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
      GeneralVars* generalVars READ getGeneralVars NOTIFY generalVarsChanged)
    Q_PROPERTY(
      QQmlPropertyMap* themeVars READ getThemeVars NOTIFY themeVarsChanged)
    GeneralVars generalVars;
    QQmlPropertyMap themeVars;
    const Profile* profile;
    QMap<QString, qml_components::ThemeFamily> availableThemeFamilies;
    QHash<QString, QHash<QString, QHash<QString, QVariant>>> loadedThemeVars;

    void populateThemePropertyMap(
      QQmlPropertyMap& themeVars,
      QHash<QString, QHash<QString, QHash<QString, QVariant>>> themeVarsData,
      const std::filesystem::path& themeVarsPath);
    Q_SLOT void writeGeneralVars() const;

  public:
    explicit Vars(
      const Profile* profile,
      QMap<QString, qml_components::ThemeFamily> availableThemeFamilies,
      QString avatarPath,
      QObject* parent = nullptr);
    auto getGeneralVars() -> GeneralVars*;
    auto getThemeVars() -> QQmlPropertyMap*;

  signals:
    void generalVarsChanged();
    void themeVarsChanged();
};
} // namespace resource_managers

#endif // RHYTHMGAME_VARS_H
