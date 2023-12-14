//
// Created by bobini on 09.12.23.
//

#include "InputTranslator.h"

#include "GamepadManager.h"

#include <QKeyEvent>
#include <QVariant>

namespace input {
auto
Key::toVariantMap() const -> QVariantMap
{
    if (gamepad.isNull()) {
        return { { "gamepad", QVariant() },
                 { "device",
                   QString::fromStdString(
                     std::string(magic_enum::enum_name(device))) },
                 { "code", code },
                 { "direction",
                   QString::fromStdString(
                     std::string(magic_enum::enum_name(direction))) } };
    }
    auto gp = gamepad.value<Gamepad>();
    return { { "gamepad", gp.toVariantMap() },
             { "device",
               QString::fromStdString(
                 std::string(magic_enum::enum_name(device))) },
             { "code", code },
             { "direction",
               QString::fromStdString(
                 std::string(magic_enum::enum_name(direction))) } };
}
auto
Key::fromVariantMap(const QVariantMap& map) -> Key
{
    auto key = map["device"].toString().toStdString();
    auto keyEnum = magic_enum::enum_cast<Device>(key);
    auto dir = map["direction"].toString().toStdString();
    auto dirEnum = magic_enum::enum_cast<Direction>(dir);
    auto gp = map["gamepad"];
    if (gp.isNull()) {
        return { QVariant(),
                 keyEnum ? *keyEnum : Device::Keyboard,
                 map["code"].toInt(),
                 dirEnum ? *dirEnum : Direction::None };
    }
    return { QVariant::fromValue(Gamepad::fromVariantMap(gp.toMap())),
             keyEnum ? *keyEnum : Device::Keyboard,
             map["code"].toInt(),
             dirEnum ? *dirEnum : Direction::None };
}
auto
Key::operator<=>(const Key& key) const -> std::weak_ordering
{
    auto gp1 = gamepad.canConvert<Gamepad>();
    auto gp2 = key.gamepad.canConvert<Gamepad>();
    if (gp1 && gp2) {
        auto gp1obj = gamepad.value<Gamepad>();
        auto gp2obj = key.gamepad.value<Gamepad>();
        return std::tie(gp1obj, device, code, direction) <=>
               std::tie(gp2obj, key.device, key.code, key.direction);
    }
    if (gp1) {
        return std::strong_ordering::less;
    }
    if (gp2) {
        return std::strong_ordering::greater;
    }
    return std::tie(device, code, direction) <=>
           std::tie(key.device, key.code, key.direction);
}

auto
Key::operator==(const Key& key) const -> bool
{
    return (*this <=> key) == std::strong_ordering::equal;
}
auto
Key::operator!=(const Key& key) const -> bool
{
    return !(*this == key);
}
auto
Mapping::toVariantMap() const -> QVariantMap
{
    return { { "key", key.toVariantMap() },
             { "button",
               QString::fromStdString(
                 std::string(magic_enum::enum_name(button))) } };
}
auto
Mapping::fromVariantMap(const QVariantMap& map) -> Mapping
{
    return { Key::fromVariantMap(map["key"].toMap()),
             magic_enum::enum_cast<BmsKey>(
               map["button"].toString().toStdString())
               .value() };
}
void
InputTranslator::pressButton(BmsKey button, double value, uint32_t time)
{
    auto& state = buttons[static_cast<int>(button)];
    const auto oldState = state;
    switch (button) {
        case BmsKey::Col11:
            state = true;
            if (!oldState) {
                emit col11Changed();
            }
            break;
        case BmsKey::Col12:
            state = true;
            if (!oldState) {
                emit col12Changed();
            }
            break;

        case BmsKey::Col13:
            state = true;
            if (!oldState) {
                emit col13Changed();
            }
            break;

        case BmsKey::Col14:
            state = true;
            if (!oldState) {
                emit col14Changed();
            }
            break;

        case BmsKey::Col15:
            state = true;
            if (!oldState) {
                emit col15Changed();
            }
            break;

        case BmsKey::Col16:
            state = true;
            if (!oldState) {
                emit col16Changed();
            }
            break;

        case BmsKey::Col17:
            state = true;
            if (!oldState) {
                emit col17Changed();
            }
            break;

        case BmsKey::Col1s:
            state = true;
            if (!oldState) {
                emit col1sChanged();
            }
            break;

        case BmsKey::Col21:
            state = true;
            if (!oldState) {
                emit col21Changed();
            }
            break;

        case BmsKey::Col22:
            state = true;
            if (!oldState) {
                emit col22Changed();
            }
            break;

        case BmsKey::Col23:
            state = true;
            if (!oldState) {
                emit col23Changed();
            }
            break;

        case BmsKey::Col24:
            if (!oldState) {
                emit col24Changed();
            }
            state = true;
            break;

        case BmsKey::Col25:
            state = true;
            if (!oldState) {
                emit col25Changed();
            }
            break;

        case BmsKey::Col26:
            state = true;
            if (!oldState) {
                emit col26Changed();
            }
            break;

        case BmsKey::Col27:
            state = true;
            if (!oldState) {
                emit col27Changed();
            }
            break;

        case BmsKey::Col2s:
            state = true;
            if (!oldState) {
                emit col2sChanged();
            }
            break;

        case BmsKey::Start:
            state = true;
            if (!oldState) {
                emit startChanged();
            }
            break;

        case BmsKey::Select:
            state = true;
            if (!oldState) {
                emit selectChanged();
            }
            break;
    }
    emit buttonPressed(button, value, time);
}
void
InputTranslator::releaseButton(BmsKey button, uint32_t time)
{
    auto& state = buttons[static_cast<int>(button)];
    const auto oldState = state;
    switch (button) {
        case BmsKey::Col11:
            state = false;
            if (oldState) {
                emit col11Changed();
            }
            break;
        case BmsKey::Col12:
            state = false;
            if (oldState) {
                emit col12Changed();
            }
            break;

        case BmsKey::Col13:
            state = false;
            if (oldState) {
                emit col13Changed();
            }
            break;

        case BmsKey::Col14:
            state = false;
            if (oldState) {
                emit col14Changed();
            }
            break;

        case BmsKey::Col15:
            state = false;
            if (oldState) {
                emit col15Changed();
            }
            break;

        case BmsKey::Col16:
            state = false;
            if (oldState) {
                emit col16Changed();
            }
            break;

        case BmsKey::Col17:
            state = false;
            if (oldState) {
                emit col17Changed();
            }
            break;

        case BmsKey::Col1s:
            state = false;
            if (oldState) {
                emit col1sChanged();
            }
            break;

        case BmsKey::Col21:
            state = false;
            if (oldState) {
                emit col21Changed();
            }
            break;

        case BmsKey::Col22:
            state = false;
            if (oldState) {
                emit col22Changed();
            }
            break;

        case BmsKey::Col23:
            state = false;
            if (oldState) {
                emit col23Changed();
            }
            break;

        case BmsKey::Col24:
            state = false;
            if (oldState) {
                emit col24Changed();
            }
            break;

        case BmsKey::Col25:
            state = false;
            if (oldState) {
                emit col25Changed();
            }
            break;

        case BmsKey::Col26:
            state = false;
            if (oldState) {
                emit col26Changed();
            }
            break;

        case BmsKey::Col27:
            state = false;
            if (oldState) {
                emit col27Changed();
            }
            break;

        case BmsKey::Col2s:
            state = false;
            if (oldState) {
                emit col2sChanged();
            }
            break;

        case BmsKey::Start:
            state = false;
            if (oldState) {
                emit startChanged();
            }
            break;

        case BmsKey::Select:
            state = false;
            if (oldState) {
                emit selectChanged();
            }
            break;
    }
    emit buttonReleased(button, time);
}
void
InputTranslator::unpressCurrentKey(const Key& key, uint32_t time)
{
    if (auto found = config.find(key); found != config.end()) {
        releaseButton(*found, time);
    }
    // go through config to find configuredButton
    for (auto it = config.begin(); it != config.end(); it++) {
        if (it.value() == *configuredButton) {
            config.erase(it);
            break;
        }
    }
}
void
InputTranslator::handleAxis(Gamepad gamepad,
                            Uint8 axis,
                            double value,
                            uint32_t time)
{
    auto scratchKey = std::pair{ gamepad, axis };
    auto& scratch = scratches[scratchKey];
    if (std::abs(scratch.value - value) < scratchSensitivity) {
        return;
    }
    scratch.value = value;
    auto direction =
      value > scratch.value ? Key::Direction::Up : Key::Direction::Down;
    if (2 - std::abs(scratch.value - value) > 2 - scratchSensitivity) {
        direction = direction == Key::Direction::Up ? Key::Direction::Down
                                                    : Key::Direction::Up;
    }
    auto keyLookup =
      Key{ QVariant::fromValue(gamepad), Key::Device::Axis, axis, direction };
    if (isConfiguring()) {
        unpressCurrentKey(keyLookup, time);
        config[keyLookup] = *configuredButton;
        emit keyConfigModified();
        setConfiguredButton({});
    } else {
        auto key = config.find(keyLookup);
        if (key == config.end()) {
            return;
        }
        pressButton(*key, value, time);
        scratch.timer = std::make_unique<QTimer>();
        scratch.timer->setSingleShot(true);
        scratch.timer->setInterval(100);
        connect(
          scratch.timer.get(), &QTimer::timeout, [this, key = *key, time] {
              int i;
              releaseButton(key, time + 100);
          });
        scratch.timer->start();
        // find key with opposite direction
        const auto oppositeKey = config.find(
          Key{ QVariant::fromValue(std::move(gamepad)),
               Key::Device::Axis,
               axis,
               direction == Key::Direction::Up ? Key::Direction::Down
                                               : Key::Direction::Up });
        if (oppositeKey != config.end()) {
            releaseButton(*oppositeKey, time);
        }
    }
}
void
InputTranslator::handlePress(Gamepad gamepad,
                             Uint8 button,
                             double x,
                             Uint32 time)
{
    if (isConfiguring()) {
        auto keyLookup = Key{ QVariant::fromValue(std::move(gamepad)),
                              Key::Device::Button,
                              button };
        unpressCurrentKey(keyLookup, time);
        config[keyLookup] = *configuredButton;
        emit keyConfigModified();
        setConfiguredButton({});
    } else {
        auto key = config.find(Key{ QVariant::fromValue(std::move(gamepad)),
                                    Key::Device::Button,
                                    button });
        if (key != config.end()) {
            pressButton(*key, 1.0, time);
        }
    }
}
void
InputTranslator::handleRelease(Gamepad gamepad, Uint8 button, Uint32 time)
{
    auto key = config.find(Key{
      QVariant::fromValue(std::move(gamepad)), Key::Device::Button, button });
    if (key != config.end()) {
        releaseButton(*key, time);
    }
}
InputTranslator::
InputTranslator(const GamepadManager* source, QObject* parent)
  : QObject(parent)
{
    connect(
      source, &GamepadManager::axisMoved, this, &InputTranslator::handleAxis);
    connect(source,
            &GamepadManager::buttonPressed,
            this,
            &InputTranslator::handlePress);
    connect(source,
            &GamepadManager::buttonReleased,
            this,
            &InputTranslator::handleRelease);
}
void
InputTranslator::setConfiguredButton(QVariant button)
{
    const auto old = configuredButton;
    const auto oldIsConfiguring = isConfiguring();
    configuredButton = button.canConvert<BmsKey>()
                         ? std::optional{ button.value<BmsKey>() }
                         : std::nullopt;
    if (old != configuredButton) {
        emit configuredButtonChanged();
    }
    if (oldIsConfiguring != isConfiguring()) {
        emit configuringChanged();
    }
}
auto
InputTranslator::getConfiguredButton() const -> QVariant
{
    return configuredButton.has_value() ? QVariant::fromValue(*configuredButton)
                                        : QVariant();
}
auto
InputTranslator::isConfiguring() const -> bool
{
    return configuredButton.has_value();
}
void
InputTranslator::setKeyConfig(const QList<Mapping>& newConfig)
{
    config.clear();
    for (const auto& mapping : newConfig) {
        config[mapping.key] = mapping.button;
    }
    emit keyConfigModified();
}
auto
InputTranslator::getKeyConfig() -> QList<Mapping>
{
    auto result = QList<Mapping>();
    for (const auto& [key, button] : config.asKeyValueRange()) {
        result.append({ key, button });
    }
    return result;
}
auto
InputTranslator::col11() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col11)];
}
auto
InputTranslator::col12() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col12)];
}

auto
InputTranslator::col13() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col13)];
}

auto
InputTranslator::col14() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col14)];
}

auto
InputTranslator::col15() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col15)];
}

auto
InputTranslator::col16() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col16)];
}

auto
InputTranslator::col17() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col17)];
}

auto
InputTranslator::col1s() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col1s)];
}

auto
InputTranslator::col21() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col21)];
}

auto
InputTranslator::col22() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col22)];
}

auto
InputTranslator::col23() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col23)];
}

auto
InputTranslator::col24() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col24)];
}

auto
InputTranslator::col25() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col25)];
}

auto
InputTranslator::col26() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col26)];
}

auto
InputTranslator::col27() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col27)];
}

auto
InputTranslator::col2s() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col2s)];
}

auto
InputTranslator::start() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Start)];
}

auto
InputTranslator::select() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Select)];
}
bool
InputTranslator::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        const auto key = static_cast<QKeyEvent*>(event);
        const auto keyLookup = Key{
            QVariant(), Key::Device::Keyboard, key->key(), Key::Direction::None
        };
        if (isConfiguring()) {
            unpressCurrentKey(keyLookup, key->timestamp());
            config[keyLookup] = *configuredButton;
            emit keyConfigModified();
            setConfiguredButton({});
        } else {
            if (const auto found = config.find(keyLookup);
                found != config.end()) {
                pressButton(*found, 1.0, key->timestamp());
            }
        }
    } else if (event->type() == QEvent::KeyRelease) {
        const auto key = static_cast<QKeyEvent*>(event);
        const auto keyLookup = Key{
            QVariant(), Key::Device::Keyboard, key->key(), Key::Direction::None
        };
        if (const auto found = config.find(keyLookup); found != config.end()) {
            releaseButton(*found, key->timestamp());
        }
    }
    return false;
}

} // namespace input