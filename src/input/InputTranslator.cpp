//
// Created by bobini on 09.12.23.
//

#include "InputTranslator.h"

#include "GamepadManager.h"

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
        return gamepad.value<Gamepad>() <=> key.gamepad.value<Gamepad>();
    }
    if (gp1) {
        return std::strong_ordering::less;
    }
    if (gp2) {
        return std::strong_ordering::greater;
    }
    return code <=> key.code;
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
    emit buttonPressed(button, value, time);
    auto& state = buttons[static_cast<int>(button)];
    switch (button) {
        case BmsKey::Col11:
            if (!state) {
                emit col11Changed();
            }
            state = true;
            break;
        case BmsKey::Col12:
            if (!state) {
                emit col12Changed();
            }
            state = true;
            break;

        case BmsKey::Col13:
            if (!state) {
                emit col13Changed();
            }
            state = true;
            break;

        case BmsKey::Col14:
            if (!state) {
                emit col14Changed();
            }
            state = true;
            break;

        case BmsKey::Col15:
            if (!state) {
                emit col15Changed();
            }
            state = true;
            break;

        case BmsKey::Col16:
            if (!state) {
                emit col16Changed();
            }
            state = true;
            break;

        case BmsKey::Col17:
            if (!state) {
                emit col17Changed();
            }
            state = true;
            break;

        case BmsKey::Col1s:
            if (!state) {
                emit col1sChanged();
            }
            state = true;
            break;

        case BmsKey::Col21:
            if (!state) {
                emit col21Changed();
            }
            state = true;
            break;

        case BmsKey::Col22:
            if (!state) {
                emit col22Changed();
            }
            state = true;
            break;

        case BmsKey::Col23:
            if (!state) {
                emit col23Changed();
            }
            state = true;
            break;

        case BmsKey::Col24:
            if (!state) {
                emit col24Changed();
            }
            state = true;
            break;

        case BmsKey::Col25:
            if (!state) {
                emit col25Changed();
            }
            state = true;
            break;

        case BmsKey::Col26:
            if (!state) {
                emit col26Changed();
            }
            state = true;
            break;

        case BmsKey::Col27:
            if (!state) {
                emit col27Changed();
            }
            state = true;
            break;

        case BmsKey::Col2s:
            if (!state) {
                emit col2sChanged();
            }
            state = true;
            break;

        case BmsKey::Start:
            if (!state) {
                emit startChanged();
            }
            state = true;
            break;

        case BmsKey::Select:
            if (!state) {
                emit selectChanged();
            }
            state = true;
            break;
    }
}
void
InputTranslator::releaseButton(BmsKey button, uint32_t time)
{
    emit buttonReleased(button, time);
    auto& state = buttons[static_cast<int>(button)];
    switch (button) {
        case BmsKey::Col11:
            if (state) {
                emit col11Changed();
            }
            state = false;
            break;
        case BmsKey::Col12:
            if (state) {
                emit col12Changed();
            }
            state = false;
            break;

        case BmsKey::Col13:
            if (state) {
                emit col13Changed();
            }
            state = false;
            break;

        case BmsKey::Col14:
            if (state) {
                emit col14Changed();
            }
            state = false;
            break;

        case BmsKey::Col15:
            if (state) {
                emit col15Changed();
            }
            state = false;
            break;

        case BmsKey::Col16:
            if (state) {
                emit col16Changed();
            }
            state = false;
            break;

        case BmsKey::Col17:
            if (state) {
                emit col17Changed();
            }
            state = false;
            break;

        case BmsKey::Col1s:
            if (state) {
                emit col1sChanged();
            }
            state = false;
            break;

        case BmsKey::Col21:
            if (state) {
                emit col21Changed();
            }
            state = false;
            break;

        case BmsKey::Col22:
            if (state) {
                emit col22Changed();
            }
            state = false;
            break;

        case BmsKey::Col23:
            if (state) {
                emit col23Changed();
            }
            state = false;
            break;

        case BmsKey::Col24:
            if (state) {
                emit col24Changed();
            }
            state = false;
            break;

        case BmsKey::Col25:
            if (state) {
                emit col25Changed();
            }
            state = false;
            break;

        case BmsKey::Col26:
            if (state) {
                emit col26Changed();
            }
            state = false;
            break;

        case BmsKey::Col27:
            if (state) {
                emit col27Changed();
            }
            state = false;
            break;

        case BmsKey::Col2s:
            if (state) {
                emit col2sChanged();
            }
            state = false;
            break;

        case BmsKey::Start:
            if (state) {
                emit startChanged();
            }
            state = false;
            break;

        case BmsKey::Select:
            if (state) {
                emit selectChanged();
            }
            state = false;
            break;
    }
}
void
InputTranslator::unpressCurrentKey(const Key& key, uint32_t time)
{
    if (auto found = config.find(key); found != config.end()) {
        releaseButton(*found, time);
    }
}
InputTranslator::
InputTranslator(const GamepadManager* source, QObject* parent)
  : QObject(parent)
{
    connect(
      source,
      &GamepadManager::axisMoved,
      [this](Gamepad gamepad, Uint8 axis, double value, uint32_t time) {
          auto scratchKey = std::pair{ gamepad, axis };
          auto& scratch = scratches[scratchKey];
          if (std::abs(scratch.value - value) < scratchSensitivity) {
              return;
          }
          auto direction =
            value > scratch.value ? Key::Direction::Up : Key::Direction::Down;
          if (1 - std::abs(scratch.value - value) > 1 - scratchSensitivity) {
              direction = direction == Key::Direction::Up ? Key::Direction::Down
                                                          : Key::Direction::Up;
          }
          auto keyLookup = Key{ QVariant::fromValue(std::move(gamepad)),
                                Key::Device::Axis,
                                axis,
                                direction };
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
              scratch.direction = direction;
              scratch.value = value;
              scratch.timer = std::make_unique<QTimer>();
              scratch.timer->setSingleShot(true);
              scratch.timer->setInterval(100);
              connect(
                scratch.timer.get(),
                &QTimer::timeout,
                [this, key = *key, scratchKey = std::move(scratchKey), time] {
                    auto& scratch = scratches[scratchKey];
                    scratch.direction = Key::Direction::None;
                    releaseButton(key, time + 100);
                });
              scratch.timer->start();
              pressButton(*key, value, time);
              // find key with opposite direction
              auto oppositeKey = config.find(
                Key{ QVariant::fromValue(std::move(gamepad)),
                     Key::Device::Axis,
                     axis,
                     direction == Key::Direction::Up ? Key::Direction::Down
                                                     : Key::Direction::Up });
              if (oppositeKey != config.end()) {
                  releaseButton(*oppositeKey, time);
              }
          }
      });
    connect(source,
            &GamepadManager::buttonPressed,
            [this](Gamepad gamepad, Uint8 button, double x, Uint32 time) {
                if (isConfiguring()) {
                    auto keyLookup =
                      Key{ QVariant::fromValue(std::move(gamepad)),
                           Key::Device::Button,
                           button };
                    unpressCurrentKey(keyLookup, time);
                    config[keyLookup] = *configuredButton;
                    emit keyConfigModified();
                    setConfiguredButton({});
                } else {
                    auto key =
                      config.find(Key{ QVariant::fromValue(std::move(gamepad)),
                                       Key::Device::Button,
                                       button });
                    if (key != config.end()) {
                        pressButton(*key, 1.0, time);
                    }
                }
            });
    connect(source,
            &GamepadManager::buttonReleased,
            [this](Gamepad gamepad, Uint8 button, Uint32 time) {
                auto key =
                  config.find(Key{ QVariant::fromValue(std::move(gamepad)),
                                   Key::Device::Button,
                                   button });
                if (key != config.end()) {
                    releaseButton(*key, time);
                }
            });
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

} // namespace input