//
// Created by bobini on 09.12.23.
//

#ifndef GAMEPADINPUTTRANSLATOR_H
#define GAMEPADINPUTTRANSLATOR_H
#include "BmsKeys.h"
#include "GamepadManager.h"

#include <QKeyEvent>
#include <QObject>
#include <QVariant>
#include <magic_enum/magic_enum.hpp>
#include <functional>

namespace db {
class SqliteCppDb;
}
namespace input {
class Key
{
    Q_GADGET
    Q_PROPERTY(QVariant gamepad MEMBER gamepad)
    Q_PROPERTY(Device device MEMBER device)
    Q_PROPERTY(int code MEMBER code)
    Q_PROPERTY(Direction direction MEMBER direction)

  public:
    enum class Device
    {
        Keyboard,
        Button,
        Axis
    };
    Q_ENUM(Device)
    enum class Direction
    {
        None,
        Up,
        Down
    };
    Q_ENUM(Direction)

    QVariant gamepad = QVariant::fromValue(nullptr);
    Device device;
    int code;
    Direction direction{};

    friend auto operator>>(QDataStream& stream, Key& key) -> QDataStream&;
    friend auto operator<<(QDataStream& stream, const Key& key) -> QDataStream&;

    auto operator<=>(const Key& key) const -> std::weak_ordering;
    auto operator==(const Key& key) const -> bool;
    auto operator!=(const Key& key) const -> bool;
};
} // namespace input

template<>
struct std::hash<input::Key>
{
    std::size_t operator()(const input::Key& s) const noexcept
    {
        auto gp = s.gamepad.canConvert<input::Gamepad>()
                    ? std::optional{ s.gamepad.value<input::Gamepad>() }
                    : std::nullopt;
        return std::hash<std::optional<input::Gamepad>>{}(gp) ^
               std::hash<int>{}(s.code) ^
               std::hash<int>{}(static_cast<int>(s.device)) ^
               std::hash<int>{}(static_cast<int>(s.direction));
    }
};
namespace input {
class Mapping
{
    Q_GADGET
    Q_PROPERTY(Key key MEMBER key)
    Q_PROPERTY(BmsKey button MEMBER button)

  public:
    Key key;
    BmsKey button;

    auto operator<=>(const Mapping&) const = default;

    friend auto operator<<(QDataStream& stream, const Mapping& mapping)
      -> QDataStream&;
    friend auto operator>>(QDataStream& stream, Mapping& mapping)
      -> QDataStream&;
};

class AnalogAxisConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double triggerThreshold READ getTriggerThreshold WRITE
                 setTriggerThreshold NOTIFY triggerThresholdChanged)
    Q_PROPERTY(double releaseThreshold READ getReleaseThreshold WRITE
                 setReleaseThreshold NOTIFY releaseThresholdChanged)
    Q_PROPERTY(uint timeout READ getTimeout WRITE setTimeout NOTIFY timeoutChanged)
    Q_PROPERTY(ScratchAlgorithm scratchAlgorithm READ getScratchAlgorithm WRITE
                 setScratchAlgorithm NOTIFY scratchAlgorithmChanged)
  public:
    enum ScratchAlgorithm
    {
        ScratchAlgorithmAnalog,
        ScratchAlgorithmClassic
    };
    Q_ENUM(ScratchAlgorithm)
  private:
    double triggerThreshold = 0.008;
    double releaseThreshold = 0.004;
    uint timeout = 100;
    ScratchAlgorithm algorithm = ScratchAlgorithmAnalog;

  public:
    auto getTriggerThreshold() const -> double;
    void setTriggerThreshold(double value);
    auto getReleaseThreshold() const -> double;
    void setReleaseThreshold(double value);
    auto getTimeout() const -> uint;
    void setTimeout(uint value);
    auto getScratchAlgorithm() const -> ScratchAlgorithm;
    void setScratchAlgorithm(ScratchAlgorithm value);
    explicit AnalogAxisConfig(QObject* parent = nullptr);
    signals:
    void triggerThresholdChanged();
    void releaseThresholdChanged();
    void timeoutChanged();
    void scratchAlgorithmChanged();
};

class InputTranslator final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool configuring READ isConfiguring NOTIFY configuringChanged)
    Q_PROPERTY(QVariant configuredButton READ getConfiguredButton WRITE
                 setConfiguredButton NOTIFY configuredButtonChanged)
    Q_PROPERTY(QList<Mapping> keyConfig READ getKeyConfig WRITE setKeyConfig
                 NOTIFY keyConfigModified)
    Q_PROPERTY(bool col11 READ col11 NOTIFY col11Changed)
    Q_PROPERTY(bool col12 READ col12 NOTIFY col12Changed)
    Q_PROPERTY(bool col13 READ col13 NOTIFY col13Changed)
    Q_PROPERTY(bool col14 READ col14 NOTIFY col14Changed)
    Q_PROPERTY(bool col15 READ col15 NOTIFY col15Changed)
    Q_PROPERTY(bool col16 READ col16 NOTIFY col16Changed)
    Q_PROPERTY(bool col17 READ col17 NOTIFY col17Changed)
    Q_PROPERTY(bool col1sUp READ col1sUp NOTIFY col1sUpChanged)
    Q_PROPERTY(bool col1sDown READ col1sDown NOTIFY col1sDownChanged)
    Q_PROPERTY(bool col21 READ col21 NOTIFY col21Changed)
    Q_PROPERTY(bool col22 READ col22 NOTIFY col22Changed)
    Q_PROPERTY(bool col23 READ col23 NOTIFY col23Changed)
    Q_PROPERTY(bool col24 READ col24 NOTIFY col24Changed)
    Q_PROPERTY(bool col25 READ col25 NOTIFY col25Changed)
    Q_PROPERTY(bool col26 READ col26 NOTIFY col26Changed)
    Q_PROPERTY(bool col27 READ col27 NOTIFY col27Changed)
    Q_PROPERTY(bool col2sUp READ col2sUp NOTIFY col2sUpChanged)
    Q_PROPERTY(bool col2sDown READ col2sDown NOTIFY col2sDownChanged)
    Q_PROPERTY(bool start1 READ start1 NOTIFY start1Changed)
    Q_PROPERTY(bool select READ select1 NOTIFY select1Changed)
    Q_PROPERTY(bool start2 READ start2 NOTIFY start2Changed)
    Q_PROPERTY(bool select2 READ select2 NOTIFY select2Changed)
    Q_PROPERTY(input::AnalogAxisConfig* analogAxisConfig1 READ getAnalogAxisConfig1 NOTIFY
                 analogAxisConfig1Changed)
    Q_PROPERTY(input::AnalogAxisConfig* analogAxisConfig2 READ getAnalogAxisConfig2 NOTIFY
                 analogAxisConfig2Changed)

  public:
    struct Scratch
    {
        std::unique_ptr<QTimer> timer = std::make_unique<QTimer>();
        double delta = 0;
        Key::Direction direction = Key::Direction::None;
        double value = std::numeric_limits<double>::quiet_NaN();
    };

  private:
    struct PairHash
    {
        template<typename T, typename U>
        auto operator()(const std::pair<T, U>& x) const -> std::size_t
        {
            return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
        }
    };
    std::unordered_map<std::pair<Gamepad, uint8_t>, Scratch, PairHash>
      scratches;
    std::optional<BmsKey> configuredButton;
    std::unordered_map<std::pair<Gamepad, uint8_t>, AnalogAxisConfig*, PairHash>
      axisConfig;
    QHash<Key, BmsKey> config;
    db::SqliteCppDb* db;
    std::array<bool, magic_enum::enum_count<BmsKey>()> buttons{};
    std::optional<std::pair<Gamepad, uint8_t>> scratchAxis1;
    std::optional<std::pair<Gamepad, uint8_t>> scratchAxis2;

    void pressButton(BmsKey button, uint64_t time);
    void releaseButton(BmsKey button, uint64_t time);
    void unpressAndUnbind(const Key& key, uint64_t time);
    void saveKeyConfig() const;
    void saveAnalogAxisConfig() const;
    void handleAxisChange(Gamepad gamepad, Uint8 axis, int64_t time);
    void checkAnalogAxisStatus();
    void autoReleaseScratch(const std::pair<Gamepad, uint8_t>& scratchKey,
                            int64_t time);

  public:
    void handleAxis(Gamepad gamepad, Uint8 axis, double value, int64_t time);
    void handlePress(Gamepad gamepad, Uint8 button, int64_t time);
    void handleRelease(Gamepad gamepad, Uint8 button, int64_t time);

    void loadKeyConfig(db::SqliteCppDb* db);
    void loadAnalogAxisConfig(db::SqliteCppDb* db);
    explicit InputTranslator(db::SqliteCppDb* db, QObject* parent = nullptr);
    void setConfiguredButton(const QVariant& button);
    auto getConfiguredButton() const -> QVariant;
    auto isConfiguring() const -> bool;
    void setKeyConfig(const QList<Mapping>& config);
    auto getKeyConfig() -> QList<Mapping>;
    void setKeyConfig(const QHash<Key, BmsKey>& config);
    auto getKeyConfigHash() -> QHash<Key, BmsKey>;
    Q_INVOKABLE void resetButton(BmsKey key);
    auto col11() const -> bool;
    auto col12() const -> bool;
    auto col13() const -> bool;
    auto col14() const -> bool;
    auto col15() const -> bool;
    auto col16() const -> bool;
    auto col17() const -> bool;
    auto col1sUp() const -> bool;
    auto col1sDown() const -> bool;
    auto col21() const -> bool;
    auto col22() const -> bool;
    auto col23() const -> bool;
    auto col24() const -> bool;
    auto col25() const -> bool;
    auto col26() const -> bool;
    auto col27() const -> bool;
    auto col2sUp() const -> bool;
    auto col2sDown() const -> bool;
    auto start1() const -> bool;
    auto select1() const -> bool;
    auto start2() const -> bool;
    auto select2() const -> bool;
    auto getAnalogAxisConfig1() -> AnalogAxisConfig*;
    auto getAnalogAxisConfig2() -> AnalogAxisConfig*;
    auto eventFilter(QObject* watched, QEvent* event) -> bool override;
    Q_INVOKABLE static QString scancodeToString(int scancode);

  signals:
    void buttonPressed(BmsKey button, int64_t time);
    void buttonReleased(BmsKey button, int64_t time);
    void keyConfigModified();
    void configuringChanged();
    void configuredButtonChanged();
    void col11Changed();
    void col12Changed();
    void col13Changed();
    void col14Changed();
    void col15Changed();
    void col16Changed();
    void col17Changed();
    void col1sUpChanged();
    void col1sDownChanged();
    void col21Changed();
    void col22Changed();
    void col23Changed();
    void col24Changed();
    void col25Changed();
    void col26Changed();
    void col27Changed();
    void col2sUpChanged();
    void col2sDownChanged();
    void start1Changed();
    void select1Changed();
    void start2Changed();
    void select2Changed();
    void analogAxisConfig1Changed();
    void analogAxisConfig2Changed();
};

} // namespace input

#endif // GAMEPADINPUTTRANSLATOR_H
