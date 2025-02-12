//
// Created by bobini on 09.12.23.
//

#include "InputTranslator.h"

#include "GamepadManager.h"
#include "db/SqliteCppDb.h"
#include "support/Compress.h"

#include <QKeyEvent>
#include <QVariant>
#ifdef _WIN32
#include <windows.h>
#endif

namespace input {
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

void
InputTranslator::pressButton(BmsKey button, double value, uint64_t time)
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

        case BmsKey::Col1sUp:
            state = true;
            if (!oldState) {
                emit col1sUpChanged();
            }
            break;

        case BmsKey::Col1sDown:
            state = true;
            if (!oldState) {
                emit col1sDownChanged();
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

        case BmsKey::Col2sUp:
            state = true;
            if (!oldState) {
                emit col2sUpChanged();
            }
            break;

        case BmsKey::Col2sDown:
            state = true;
            if (!oldState) {
                emit col2sDownChanged();
            }
            break;

        case BmsKey::Start1:
            state = true;
            if (!oldState) {
                emit start1Changed();
            }
            break;

        case BmsKey::Select1:
            state = true;
            if (!oldState) {
                emit select1Changed();
            }
            break;

        case BmsKey::Start2:
            state = true;
            if (!oldState) {
                emit start2Changed();
            }
            break;

        case BmsKey::Select2:
            state = true;
            if (!oldState) {
                emit select2Changed();
            }
            break;
    }
    if (!oldState) {
        emit buttonPressed(button, value, time);
    }
}
void
InputTranslator::releaseButton(BmsKey button, uint64_t time)
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

        case BmsKey::Col1sUp:
            state = false;
            if (oldState) {
                emit col1sUpChanged();
            }
            break;

        case BmsKey::Col1sDown:
            state = false;
            if (oldState) {
                emit col1sDownChanged();
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

        case BmsKey::Col2sUp:
            state = false;
            if (oldState) {
                emit col2sUpChanged();
            }
            break;

        case BmsKey::Col2sDown:
            state = false;
            if (oldState) {
                emit col2sDownChanged();
            }
            break;

        case BmsKey::Start1:
            state = false;
            if (oldState) {
                emit start1Changed();
            }
            break;

        case BmsKey::Select1:
            state = false;
            if (oldState) {
                emit select1Changed();
            }
            break;

        case BmsKey::Start2:
            state = false;
            if (oldState) {
                emit start2Changed();
            }
            break;

        case BmsKey::Select2:
            state = false;
            if (oldState) {
                emit select2Changed();
            }
            break;
    }
    if (oldState) {
        emit buttonReleased(button, time);
    }
}
void
InputTranslator::unpressCurrentKey(const Key& key, uint64_t time)
{
    if (auto found = config.find(key); found != config.end()) {
        releaseButton(*found, time);
    }
    resetButton(*configuredButton);
}
void
InputTranslator::saveKeyConfig() const
{
    auto statement =
      db->createStatement("INSERT OR REPLACE INTO properties (key, value) "
                          "VALUES ('key_config', ?)");
    const auto data = support::compress(config);
    statement.bind(1, data.data(), data.size());
    statement.execute();
}
void
InputTranslator::handleAxis(Gamepad gamepad,
                            Uint8 axis,
                            double value,
                            int64_t time)
{
    auto scratchKey = std::pair{ gamepad, axis };
    auto& scratch = scratches[scratchKey];
    if (std::abs(scratch.value - value) < scratchSensitivity) {
        return;
    }
    auto direction =
      value > scratch.value ? Key::Direction::Up : Key::Direction::Down;
    if (2 - std::abs(scratch.value - value) > 2 - scratchSensitivity) {
        direction = direction == Key::Direction::Up ? Key::Direction::Down
                                                    : Key::Direction::Up;
    }
    scratch.value = value;
    auto keyLookup =
      Key{ QVariant::fromValue(gamepad), Key::Device::Axis, axis, direction };
    if (isConfiguring()) {
        unpressCurrentKey(keyLookup, time);
        config[keyLookup] = *configuredButton;
        emit keyConfigModified();
        setConfiguredButton({});
    } else {
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
        auto key = config.find(keyLookup);
        if (key == config.end()) {
            return;
        }
        pressButton(*key, value, time);
        scratch.timer = std::make_unique<QTimer>();
        scratch.timer->setSingleShot(true);
        scratch.timer->setInterval(150);
        connect(scratch.timer.get(),
                &QTimer::timeout,
                [this, key = *key, time] { releaseButton(key, time + 150); });
        scratch.timer->start();
    }
}
void
InputTranslator::handlePress(Gamepad gamepad, Uint8 button, int64_t time)
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
            pressButton(*key, 0.0, time);
        }
    }
}
void
InputTranslator::handleRelease(Gamepad gamepad, Uint8 button, int64_t time)
{
    auto key = config.find(Key{
      QVariant::fromValue(std::move(gamepad)), Key::Device::Button, button });
    if (key != config.end()) {
        releaseButton(*key, time);
    }
}
auto
toSystem(std::chrono::steady_clock::time_point tp)
  -> std::chrono::system_clock::time_point
{
    using namespace std::chrono;
    const auto systemNow = system_clock::now();
    const auto steadyNow = steady_clock::now();
    return time_point_cast<system_clock::duration>(tp - steadyNow + systemNow);
}

auto
toSystem(std::chrono::file_clock::time_point tp)
  -> std::chrono::system_clock::time_point
{
    using namespace std::chrono;
    const auto systemNow = system_clock::now();
    const auto steadyNow = file_clock ::now();
    return time_point_cast<system_clock::duration>(tp - steadyNow + systemNow);
}

auto
InputTranslator::getTime(const QKeyEvent& event) -> int64_t
{
    auto timestampQint = event.timestamp();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
             toSystem(std::chrono::steady_clock::time_point{
                        std::chrono::milliseconds{ timestampQint } })
               .time_since_epoch())
      .count();
}

InputTranslator::InputTranslator(db::SqliteCppDb* db, QObject* parent)
  : QObject(parent)
  , db(db)
{
    connect(this,
            &InputTranslator::keyConfigModified,
            this,
            &InputTranslator::saveKeyConfig);
    // load key config
    auto statement = db->createStatement(
      "SELECT value FROM properties WHERE key = 'key_config'");
    if (const auto keyConfig = statement.executeAndGet<std::string>()) {
        const auto array = QByteArray::fromStdString(keyConfig.value());
        config = support::decompress<QHash<Key, BmsKey>>(array);
    }
}
void
InputTranslator::setConfiguredButton(const QVariant& button)
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
                                        : QVariant::fromValue(nullptr);
}
auto
InputTranslator::isConfiguring() const -> bool
{
    return configuredButton.has_value();
}
void
InputTranslator::setKeyConfig(const QList<Mapping>& newConfig)
{
    auto config = QHash<Key, BmsKey>{};
    for (const auto& mapping : newConfig) {
        config[mapping.key] = mapping.button;
    }
    setKeyConfig(config);
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
void
InputTranslator::setKeyConfig(const QHash<Key, BmsKey>& config)
{
    if (this->config == config) {
        return;
    }
    this->config = config;
    emit keyConfigModified();
}
auto
InputTranslator::getKeyConfigHash() -> QHash<Key, BmsKey>
{
    return config;
}
void
InputTranslator::resetButton(BmsKey key)
{
    for (auto it = config.begin(); it != config.end(); it++) {
        if (it.value() == key) {
            config.erase(it);
            emit keyConfigModified();
            break;
        }
    }
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
InputTranslator::col1sUp() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col1sUp)];
}
auto
InputTranslator::col1sDown() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col1sDown)];
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
InputTranslator::col2sUp() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col2sUp)];
}

auto
InputTranslator::col2sDown() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Col2sDown)];
}
auto
InputTranslator::start1() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Start1)];
}

auto
InputTranslator::start2() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Start2)];
}

auto
InputTranslator::select1() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Select1)];
}

auto
InputTranslator::select2() const -> bool
{
    return buttons[static_cast<int>(BmsKey::Select2)];
}

bool
InputTranslator::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        const auto* const key = static_cast<QKeyEvent*>(event);
        if (key->isAutoRepeat()) {
            return false;
        }
        const auto time = getTime(*key);
        const auto keyLookup = Key{
            QVariant::fromValue(nullptr), Key::Device::Keyboard, key->key(), Key::Direction::None
        };
        if (isConfiguring()) {
            unpressCurrentKey(keyLookup, key->timestamp());
            config[keyLookup] = *configuredButton;
            emit keyConfigModified();
            setConfiguredButton({});
        } else {
            if (const auto found = config.find(keyLookup);
                found != config.end()) {
                pressButton(*found, 0.0, time);
            }
        }
    } else if (event->type() == QEvent::KeyRelease) {
        const auto* const key = static_cast<QKeyEvent*>(event);
        if (key->isAutoRepeat()) {
            return false;
        }
        const auto time = getTime(*key);
        const auto keyLookup = Key{
            QVariant::fromValue(nullptr), Key::Device::Keyboard, key->key(), Key::Direction::None
        };
        if (const auto found = config.find(keyLookup); found != config.end()) {
            releaseButton(*found, time);
        }
    }
    return false;
}

auto
operator>>(QDataStream& stream, Key& key) -> QDataStream&
{
    return stream >> key.gamepad >> key.device >> key.code >> key.direction;
}
auto
operator<<(QDataStream& stream, const Key& key) -> QDataStream&
{
    return stream << key.gamepad << key.device << key.code << key.direction;
}
auto
operator<<(QDataStream& stream, const Mapping& mapping) -> QDataStream&
{
    return stream << mapping.key << mapping.button;
}
auto
operator>>(QDataStream& stream, Mapping& mapping) -> QDataStream&
{
    return stream >> mapping.key >> mapping.button;
}
} // namespace input