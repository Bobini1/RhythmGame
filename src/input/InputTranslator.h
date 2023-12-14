//
// Created by bobini on 09.12.23.
//

#ifndef GAMEPADINPUTTRANSLATOR_H
#define GAMEPADINPUTTRANSLATOR_H
#include "BmsKeys.h"
#include "GamepadManager.h"

#include <QObject>
#include <QMap>
#include <QVariant>
#include <magic_enum.hpp>
#include <functional>

namespace input {
class Key
{
    Q_GADGET
    Q_PROPERTY(QVariant gamepad MEMBER gamepad)
    Q_PROPERTY(Device device MEMBER device)
    Q_PROPERTY(int code MEMBER code)

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

    QVariant gamepad;
    Device device;
    int code;
    Direction direction{};

    auto toVariantMap() const -> QVariantMap;

    static auto fromVariantMap(const QVariantMap& map) -> Key;

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

    auto toVariantMap() const -> QVariantMap;
    static auto fromVariantMap(const QVariantMap& map) -> Mapping;
    auto operator<=>(const Mapping&) const = default;
};

class InputTranslator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool configuring READ isConfiguring NOTIFY configuringChanged)
    Q_PROPERTY(QVariant configuredButton READ getConfiguredButton WRITE
                 setConfiguredButton NOTIFY configuredButtonChanged)
    Q_PROPERTY(QList<Mapping> keyConfig READ getKeyConfig WRITE setKeyConfig
                 NOTIFY keyConfigModified)
    Q_PROPERTY(bool Col11 READ col11 NOTIFY col11Changed)
    Q_PROPERTY(bool Col12 READ col12 NOTIFY col12Changed)
    Q_PROPERTY(bool Col13 READ col13 NOTIFY col13Changed)
    Q_PROPERTY(bool Col14 READ col14 NOTIFY col14Changed)
    Q_PROPERTY(bool Col15 READ col15 NOTIFY col15Changed)
    Q_PROPERTY(bool Col16 READ col16 NOTIFY col16Changed)
    Q_PROPERTY(bool Col17 READ col17 NOTIFY col17Changed)
    Q_PROPERTY(bool Col1s READ col1s NOTIFY col1sChanged)
    Q_PROPERTY(bool Col21 READ col21 NOTIFY col21Changed)
    Q_PROPERTY(bool Col22 READ col22 NOTIFY col22Changed)
    Q_PROPERTY(bool Col23 READ col23 NOTIFY col23Changed)
    Q_PROPERTY(bool Col24 READ col24 NOTIFY col24Changed)
    Q_PROPERTY(bool Col25 READ col25 NOTIFY col25Changed)
    Q_PROPERTY(bool Col26 READ col26 NOTIFY col26Changed)
    Q_PROPERTY(bool Col27 READ col27 NOTIFY col27Changed)
    Q_PROPERTY(bool Col2s READ col2s NOTIFY col2sChanged)
    Q_PROPERTY(bool Start READ start NOTIFY startChanged)
    Q_PROPERTY(bool Select READ select NOTIFY selectChanged)

  public:
    struct Scratch
    {
        std::unique_ptr<QTimer> timer;
        double value{};
    };

  private:
    struct PairHash
    {
        template<typename T, typename U>
        std::size_t operator()(const std::pair<T, U>& x) const
        {
            return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
        }
    };
    std::unordered_map<std::pair<Gamepad, uint8_t>, Scratch, PairHash>
      scratches;
    std::optional<BmsKey> configuredButton;
    QHash<Key, BmsKey> config;
    std::array<bool, magic_enum::enum_count<BmsKey>()> buttons{};
    void pressButton(BmsKey button, double value, uint32_t time);
    void releaseButton(BmsKey button, uint32_t time);
    void unpressCurrentKey(const Key& key, uint32_t time);

    static constexpr auto scratchSensitivity = 0.1;

  public:
    explicit InputTranslator(const GamepadManager* source,
                             QObject* parent = nullptr);
    void setConfiguredButton(QVariant button);
    auto getConfiguredButton() const -> QVariant;
    auto isConfiguring() const -> bool;
    void setKeyConfig(const QList<Mapping>& config);
    auto getKeyConfig() -> QList<Mapping>;
    auto col11() const -> bool;
    auto col12() const -> bool;
    auto col13() const -> bool;
    auto col14() const -> bool;
    auto col15() const -> bool;
    auto col16() const -> bool;
    auto col17() const -> bool;
    auto col1s() const -> bool;
    auto col21() const -> bool;
    auto col22() const -> bool;
    auto col23() const -> bool;
    auto col24() const -> bool;
    auto col25() const -> bool;
    auto col26() const -> bool;
    auto col27() const -> bool;
    auto col2s() const -> bool;
    auto start() const -> bool;
    auto select() const -> bool;

    bool eventFilter(QObject* watched, QEvent* event) override;

  signals:
    void buttonPressed(BmsKey button, double value, uint32_t time);
    void buttonReleased(BmsKey button, uint32_t time);
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
    void col1sChanged();
    void col21Changed();
    void col22Changed();
    void col23Changed();
    void col24Changed();
    void col25Changed();
    void col26Changed();
    void col27Changed();
    void col2sChanged();
    void startChanged();
    void selectChanged();
};

} // namespace input

#endif // GAMEPADINPUTTRANSLATOR_H
