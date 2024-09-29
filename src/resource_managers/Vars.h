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
    Q_PROPERTY(QString noteOrderAlgorithm READ getNoteOrderAlgorithm WRITE
                 setNoteOrderAlgorithm NOTIFY noteOrderAlgorithmChanged)
    Q_PROPERTY(QString noteOrderAlgorithmP2 READ getNoteOrderAlgorithmP2 WRITE
                 setNoteOrderAlgorithmP2 NOTIFY noteOrderAlgorithmP2Changed)
    Q_PROPERTY(QString hiSpeedFix READ getHiSpeedFix WRITE setHiSpeedFix NOTIFY
                 hiSpeedFixChanged)
    Q_PROPERTY(QString dpOptions READ getDpOptions WRITE setDpOptions NOTIFY
                 dpOptionsChanged)
    Q_PROPERTY(QString gaugeType READ getGaugeType WRITE setGaugeType NOTIFY
                 gaugeTypeChanged)
    Q_PROPERTY(QString gaugeMode READ getGaugeMode WRITE setGaugeMode NOTIFY
                 gaugeModeChanged)
    Q_PROPERTY(QString bottomShiftableGauge READ getBottomShiftableGauge WRITE
                 setBottomShiftableGauge NOTIFY bottomShiftableGaugeChanged)
    int noteScreenTimeMillis = 1000;
    bool laneCoverOn = false;
    double laneCoverRatio = 0.1;
    bool liftOn = false;
    double liftRatio = 0.1;
    bool hiddenOn = false;
    double hiddenRatio = 0.1;
    bool bgaOn = true;
    QString noteOrderAlgorithm = "NORMAL";
    QString noteOrderAlgorithmP2 = "NORMAL";
    QString hiSpeedFix = "MAIN";
    QString dpOptions = "OFF";
    QString gaugeType = "HAZARD";
    QString gaugeMode = "SELECT_TO_UNDER";
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
    auto getNoteOrderAlgorithm() const -> QString;
    void setNoteOrderAlgorithm(QString value);
    auto getNoteOrderAlgorithmP2() const -> QString;
    void setNoteOrderAlgorithmP2(QString value);
    auto getHiSpeedFix() const -> QString;
    void setHiSpeedFix(QString value);
    auto getDpOptions() const -> QString;
    void setDpOptions(QString value);
    auto getGaugeType() const -> QString;
    void setGaugeType(QString value);
    auto getGaugeMode() const -> QString;
    void setGaugeMode(QString value);
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
