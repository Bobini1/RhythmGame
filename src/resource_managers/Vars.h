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

/**
 * @brief The general variables for the game that all screens and the engine
 * should know about and respect. Profile-specific.
 * @details GeneralVars are saved to generalVars.json in the profile directory.
 * All modifications to the variables cause the file to be rewritten.
 */
class GeneralVars final : public QObject
{
    Q_OBJECT
    /**
     * @brief The time in milliseconds a note takes to travel from the top of
     * the screen to the judgement line.
     */
    Q_PROPERTY(double noteScreenTimeMillis READ getNoteScreenTimeMillis WRITE
                 setNoteScreenTimeMillis NOTIFY noteScreenTimeMillisChanged
                   RESET resetNoteScreenTimeMillis)
    /**
     * @brief Whether lane cover is enabled.
     * @details Lane cover makes notes hidden at the beginning of its travel
     * down the playfield.
     * @see laneCoverRatio
     */
    Q_PROPERTY(bool laneCoverOn READ getLaneCoverOn WRITE setLaneCoverOn NOTIFY
                 laneCoverOnChanged RESET resetLaneCoverOn)
    /**
     * @brief The ratio of the screen that is covered by lane cover (0-1).
     * @details If laneCoverOn is false, this has no effect.
     * @note Modifying this should not affect how long a note is visible.
     * @see laneCoverOn
     */
    Q_PROPERTY(
      double laneCoverRatio READ getLaneCoverRatio WRITE setLaneCoverRatio
        NOTIFY laneCoverRatioChanged RESET resetLaneCoverRatio)
    /**
     * @brief Whether lift is enabled.
     * @details Lifts the judgement line up from the bottom of the playfield.
     * @see liftRatio
     */
    Q_PROPERTY(bool liftOn READ getLiftOn WRITE setLiftOn NOTIFY liftOnChanged
                 RESET resetLiftOn)
    /**
     * @brief The ratio of the screen that is lifted (0-1).
     * @details If liftOn is false, this has no effect.
     * note Modifying this should not affect how long a note is visible.
     * @see liftOn
     */
    Q_PROPERTY(double liftRatio READ getLiftRatio WRITE setLiftRatio NOTIFY
                 liftRatioChanged RESET resetLiftRatio)
    /**
     * @brief Whether hidden is enabled.
     * @details Hidden makes notes invisible at the end of its travel down the
     * playfield.
     * @see hiddenRatio
     */
    Q_PROPERTY(bool hiddenOn READ getHiddenOn WRITE setHiddenOn NOTIFY
                 hiddenOnChanged RESET resetHiddenOn)
    /**
     * @brief The ratio of the screen that is covered by hidden (0-1).
     * @details If hiddenOn is false, this has no effect.
     * @note Modifying this **should** affect how long a note is visible, in
     * addition to noteScreenTimeMillis. If you cover 50% of the screen with
     * hidden, a note should be visible for half of noteScreenTimeMillis.
     * @see hiddenOn
     */
    Q_PROPERTY(double hiddenRatio READ getHiddenRatio WRITE setHiddenRatio
                 NOTIFY hiddenRatioChanged RESET resetHiddenRatio)
    /**
     * @brief Whether BGA is enabled.
     */
    Q_PROPERTY(bool bgaOn READ getBgaOn WRITE setBgaOn NOTIFY bgaOnChanged RESET
                 resetBgaOn)
    /**
     * @brief The note order algorithm used for reordering notes in charts.
     */
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithm READ getNoteOrderAlgorithm WRITE
                   setNoteOrderAlgorithm NOTIFY noteOrderAlgorithmChanged RESET
                     resetNoteOrderAlgorithm)
    /**
     * @brief The note order algorithm used for player 2 side in DP charts.
     * @see noteOrderAlgorithm
     */
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithmP2 READ getNoteOrderAlgorithmP2 WRITE
                   setNoteOrderAlgorithmP2 NOTIFY noteOrderAlgorithmP2Changed
                     RESET resetNoteOrderAlgorithmP2)
    /**
     * @brief The hi-speed fix mode.
     */
    Q_PROPERTY(
      resource_managers::hi_speed_fix::HiSpeedFix hiSpeedFix READ getHiSpeedFix
        WRITE setHiSpeedFix NOTIFY hiSpeedFixChanged RESET resetHiSpeedFix)
    /**
     * @brief The enabled DP options.
     */
    Q_PROPERTY(
      resource_managers::dp_options::DpOptions dpOptions READ getDpOptions WRITE
        setDpOptions NOTIFY dpOptionsChanged RESET resetDpOptions)
    /**
     * @brief The gauge type.
     * @details How it works depends on gaugeMode.
     */
    Q_PROPERTY(QString gaugeType READ getGaugeType WRITE setGaugeType NOTIFY
                 gaugeTypeChanged RESET resetGaugeType)
    /**
     * @brief The gauge mode.
     * @details Determines the set of gauges that the player will play with.
     */
    Q_PROPERTY(
      resource_managers::gauge_mode::GaugeMode gaugeMode READ getGaugeMode WRITE
        setGaugeMode NOTIFY gaugeModeChanged RESET resetGaugeMode)
    /**
     * @brief The avatar picture of the user.
     * @details Combine it with qml_components::ProgramSettings::avatarPath to
     * get the full path to the avatar picture.
     */
    Q_PROPERTY(QString avatar READ getAvatar WRITE setAvatar NOTIFY
                 avatarChanged RESET resetAvatar)
    /**
     * @brief The display name of the player.
     */
    Q_PROPERTY(QString name READ getName WRITE setName NOTIFY nameChanged RESET
                 resetName)
    /**
     * @brief The language/locale selected by the player.
     */
    Q_PROPERTY(QString language READ getLanguage WRITE setLanguage NOTIFY
                 languageChanged RESET resetLanguage)
    /**
     * @brief The visual offset in milliseconds to apply during gameplay.
     */
    Q_PROPERTY(
    double offset READ getOffset WRITE setOffset NOTIFY offsetChanged RESET
      resetOffset)
    // ^ remember to use full namespace for enums for reflection
    double noteScreenTimeMillis = 1000;
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
    auto getNoteScreenTimeMillis() const -> double;
    void setNoteScreenTimeMillis(double value);
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
    /**
     * @brief The general variables management object of the profile.
     */
    Q_PROPERTY(
      GeneralVars* generalVars READ getGeneralVars NOTIFY generalVarsChanged)
    /**
     * @brief The theme variables for all loaded themes.
     * @details This is a dynamic object that contains a map of maps of maps.
     * In QML, you can access vars like this:
     * themeVars[screen][themeName].varName
     * @see qml_components::QmlUtils::themeName
     */
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
