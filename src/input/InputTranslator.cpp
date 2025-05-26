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

auto
isScratch(const BmsKey key) -> bool
{
    return key == BmsKey::Col1sUp || key == BmsKey::Col1sDown ||
           key == BmsKey::Col2sUp || key == BmsKey::Col2sDown;
}

auto
invertScratch(const BmsKey key) -> BmsKey
{
    switch (key) {
        case BmsKey::Col1sUp:
            return BmsKey::Col1sDown;
        case BmsKey::Col1sDown:
            return BmsKey::Col1sUp;
        case BmsKey::Col2sUp:
            return BmsKey::Col2sDown;
        case BmsKey::Col2sDown:
            return BmsKey::Col2sUp;
        default:
            return key;
    }
}

void
InputTranslator::pressButton(BmsKey button, uint64_t time)
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
            state = true;
            if (!oldState) {
                emit col24Changed();
            }
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
        emit buttonPressed(button, time);
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
InputTranslator::unpressAndUnbind(const Key& key, uint64_t time)
{
    if (auto found = config.find(key); found != config.end()) {
        releaseButton(*found, time);
    }
    if (configuredButton.has_value()) {
        resetButton(*configuredButton);
    }
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

struct GamepadAxisConfig
{
    Gamepad gamepad;
    Uint8 axis{};
    AnalogAxisConfig config;

    friend auto operator<<(QDataStream& stream, const GamepadAxisConfig& config)
      -> QDataStream&
    {
        return stream << config.gamepad << config.axis
                      << config.config.sensitivity << config.config.timeout
                      << static_cast<int>(config.config.algorithm);
    }
    friend auto operator>>(QDataStream& stream, GamepadAxisConfig& config)
      -> QDataStream&
    {
        int algorithm;
        stream >> config.gamepad >> config.axis >> config.config.sensitivity >>
          config.config.timeout >> algorithm;
        config.config.algorithm =
          static_cast<AnalogAxisConfig::ScratchAlgorithm>(algorithm);
        return stream;
    }
};

void
InputTranslator::saveAnalogAxisConfig() const
{
    auto statement =
      db->createStatement("INSERT OR REPLACE INTO properties (key, value) "
                          "VALUES ('analog_axis_config', ?)");
    auto array = QList<GamepadAxisConfig>{};
    for (const auto& [key, config] : axisConfig) {
        array.append(GamepadAxisConfig{ key.first, key.second, config });
    }
    auto configData = support::compress(array);
    statement.bind(1, configData.data(), configData.size());
    statement.execute();
}
void
InputTranslator::handleAxisChange(Gamepad gamepad, Uint8 axis, int64_t time)
{
    auto scratchKey = std::pair{ gamepad, axis };
    auto& scratch = scratches[scratchKey];
    auto keyLookup = Key{
        QVariant::fromValue(gamepad), Key::Device::Axis, axis, scratch.direction
    };
    if (isConfiguring() && isScratch(*configuredButton) &&
        keyLookup.direction != Key::Direction::None) {
        auto button = *configuredButton;
        unpressAndUnbind(keyLookup, time);
        setConfiguredButton({});
        config[keyLookup] = button;
        keyLookup.direction = keyLookup.direction == Key::Direction::Up
                                ? Key::Direction::Down
                                : Key::Direction::Up;
        unpressAndUnbind(keyLookup, time);
        config[keyLookup] = invertScratch(button);
        emit keyConfigModified();
    } else {
        auto unpressBoth = keyLookup.direction == Key::Direction::None;
        if (unpressBoth) {
            keyLookup.direction = Key::Direction::Up;
        }
        auto oppositeKeyLookup = keyLookup;
        oppositeKeyLookup.direction = scratch.direction == Key::Direction::Up
                                        ? Key::Direction::Down
                                        : Key::Direction::Up;
        // find the key with the opposite direction
        if (const auto oppositeButton = config.find(oppositeKeyLookup);
            oppositeButton != config.end()) {
            releaseButton(*oppositeButton, time);
        }
        if (const auto button = config.find(keyLookup);
            button != config.end()) {
            if (unpressBoth) {
                releaseButton(*button, time);
            } else {
                pressButton(*button, time);
            }
        }
    }
}
void
InputTranslator::checkAnalogAxisStatus()
{
    auto axisLookup1 = std::optional<std::pair<Gamepad, uint8_t>>{};
    auto axisLookup2 = std::optional<std::pair<Gamepad, uint8_t>>{};
    for (const auto& [key, button] : this->config.asKeyValueRange()) {
        if (key.device == Key::Device::Axis &&
            (button == BmsKey::Col1sUp || button == BmsKey::Col1sDown)) {
            axisLookup1 = std::pair{ key.gamepad.value<Gamepad>(), key.code };
        }
        if (key.device == Key::Device::Axis &&
            (button == BmsKey::Col2sUp || button == BmsKey::Col2sDown)) {
            axisLookup2 = std::pair{ key.gamepad.value<Gamepad>(), key.code };
        }
    }
    if (scratchAxis1 != axisLookup1) {
        scratchAxis1 = axisLookup1;
        emit analogAxisConfig1Changed();
    }
    if (scratchAxis2 != axisLookup2) {
        scratchAxis2 = axisLookup2;
        emit analogAxisConfig2Changed();
    }
}
void
InputTranslator::autoReleaseScratch(
  const std::pair<Gamepad, uint8_t>& scratchKey,
  int64_t time)
{
    scratches[scratchKey].direction = Key::Direction::None;
    const auto keyLookup = Key{ QVariant::fromValue(scratchKey.first),
                                Key::Device::Axis,
                                scratchKey.second,
                                Key::Direction::Up };
    auto oppositeKeyLookup = keyLookup;
    oppositeKeyLookup.direction = Key::Direction::Down;
    if (const auto oppositeButton = config.find(oppositeKeyLookup);
        oppositeButton != config.end()) {
        releaseButton(*oppositeButton, time);
    }
    if (const auto button = config.find(keyLookup); button != config.end()) {
        releaseButton(*button, time);
    }
}

void
InputTranslator::handleAxis(Gamepad gamepad,
                            Uint8 axis,
                            double value,
                            int64_t time)
{
    const auto scratchKey = std::pair{ gamepad, axis };
    auto& scratch = scratches[scratchKey];
    if (std::isnan(scratch.value)) {
        scratch.value = value;
    }

    const auto analogConfig = axisConfig[scratchKey];
    if (analogConfig.algorithm == AnalogAxisConfig::ScratchAlgorithmClassic) {
        if (value > 0.9) {
            scratch.direction = Key::Direction::Up;
        } else if (value < -0.9) {
            scratch.direction = Key::Direction::Down;
        } else {
            scratch.direction = Key::Direction::None;
        }
        handleAxisChange(gamepad, axis, time);
        return;
    }

    if (value == scratch.value) {
        return;
    }

    double curDelta = value - scratch.value;
    if (curDelta > 1) {
        curDelta = value + scratch.value;
    }
    if (curDelta < -1) {
        curDelta = value + scratch.value;
    }
    scratch.value = value;

    scratch.delta += curDelta;

    auto setScratchDirection = [&](Key::Direction dir) {
        scratch.delta = 0;
        scratch.direction = dir;

        scratch.timer->setSingleShot(true);
        scratch.timer->setInterval(analogConfig.timeout);
        connect(scratch.timer.get(),
                &QTimer::timeout,
                this,
                [this, scratchKey, time, timeout = analogConfig.timeout] {
                    autoReleaseScratch(scratchKey, time + timeout);
                });
        scratch.timer->start();
    };

    if (curDelta > 0 && scratch.delta >= analogConfig.sensitivity) {
        setScratchDirection(Key::Direction::Up);
    } else if (curDelta < 0 && -scratch.delta >= analogConfig.sensitivity) {
        setScratchDirection(Key::Direction::Down);
    } else {
        return;
    }

    handleAxisChange(gamepad, axis, time);
}
void
InputTranslator::handlePress(Gamepad gamepad, Uint8 button, int64_t time)
{
    if (isConfiguring()) {
        auto keyLookup = Key{ QVariant::fromValue(std::move(gamepad)),
                              Key::Device::Button,
                              button };
        unpressAndUnbind(keyLookup, time);
        config[keyLookup] = *configuredButton;
        emit keyConfigModified();
        setConfiguredButton({});
    } else {
        auto key = config.find(Key{ QVariant::fromValue(std::move(gamepad)),
                                    Key::Device::Button,
                                    button });
        if (key != config.end()) {
            pressButton(*key, time);
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
getTime(const QKeyEvent& event) -> int64_t
{
    const auto timestampQint = event.timestamp();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
             toSystem(std::chrono::steady_clock::time_point{
                        std::chrono::milliseconds{ timestampQint } })
               .time_since_epoch())
      .count();
}

void
InputTranslator::loadKeyConfig(db::SqliteCppDb* db)
{
    auto statement = db->createStatement(
      "SELECT value FROM properties WHERE key = 'key_config'");
    if (const auto keyConfig = statement.executeAndGet<std::string>()) {
        const auto array = QByteArray::fromStdString(keyConfig.value());
        config = support::decompress<QHash<Key, BmsKey>>(array);
    }
}
void
InputTranslator::loadAnalogAxisConfig(db::SqliteCppDb* db)
{
    auto axisStatement = db->createStatement(
      "SELECT value FROM properties WHERE key = 'analog_axis_config'");
    if (const auto axisConfigData =
          axisStatement.executeAndGet<std::string>()) {
        const auto array = QByteArray::fromStdString(axisConfigData.value());
        auto axisConfigs = support::decompress<QList<GamepadAxisConfig>>(array);
        for (const auto& config : axisConfigs) {
            axisConfig[{ config.gamepad, config.axis }] = config.config;
        }
    }
    checkAnalogAxisStatus();
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
    loadKeyConfig(db);
    loadAnalogAxisConfig(db);
    connect(this,
            &InputTranslator::analogAxisConfig1Changed,
            this,
            &InputTranslator::saveAnalogAxisConfig);
    connect(this,
            &InputTranslator::analogAxisConfig2Changed,
            this,
            &InputTranslator::saveAnalogAxisConfig);
    connect(this,
            &InputTranslator::keyConfigModified,
            this,
            &InputTranslator::checkAnalogAxisStatus);
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

auto
InputTranslator::getAnalogAxisConfig1() const -> QVariant
{
    if (!scratchAxis1.has_value()) {
        return QVariant::fromValue(nullptr);
    }
    if (const auto found = axisConfig.find(*scratchAxis1);
        found != axisConfig.end()) {
        return QVariant::fromValue(found->second);
    }
    return QVariant::fromValue(AnalogAxisConfig{});
}

void
InputTranslator::setAnalogAxisConfig1(QVariant config)
{
    if (config.typeId() != qMetaTypeId<AnalogAxisConfig>()) {
        return;
    }
    auto configUnpacked = config.value<AnalogAxisConfig>();

    if (!scratchAxis1.has_value()) {
        return;
    }
    auto& axisConfig = this->axisConfig[*scratchAxis1];
    if (axisConfig == configUnpacked) {
        return;
    }
    axisConfig = configUnpacked;
    emit analogAxisConfig1Changed();
}

auto
InputTranslator::getAnalogAxisConfig2() const -> QVariant
{
    if (!scratchAxis2.has_value()) {
        return QVariant::fromValue(nullptr);
    }
    if (const auto found = axisConfig.find(*scratchAxis2);
        found != axisConfig.end()) {
        return QVariant::fromValue(found->second);
    }
    return QVariant::fromValue(AnalogAxisConfig{});
}

void
InputTranslator::setAnalogAxisConfig2(QVariant config)
{
    if (!config.canConvert<AnalogAxisConfig>()) {
        return;
    }
    auto configUnpacked = config.value<AnalogAxisConfig>();

    if (!scratchAxis2.has_value()) {
        return;
    }
    auto& axisConfig = this->axisConfig[*scratchAxis2];
    if (axisConfig == configUnpacked) {
        return;
    }
    axisConfig = configUnpacked;
    emit analogAxisConfig2Changed();
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
        const auto keyLookup = Key{ QVariant::fromValue(nullptr),
                                    Key::Device::Keyboard,
                                    key->key(),
                                    Key::Direction::None };
        if (isConfiguring()) {
            unpressAndUnbind(keyLookup, key->timestamp());
            config[keyLookup] = *configuredButton;
            emit keyConfigModified();
            setConfiguredButton({});
        } else {
            if (const auto found = config.find(keyLookup);
                found != config.end()) {
                pressButton(*found, time);
            }
        }
    } else if (event->type() == QEvent::KeyRelease) {
        const auto* const key = static_cast<QKeyEvent*>(event);
        if (key->isAutoRepeat()) {
            return false;
        }
        const auto time = getTime(*key);
        const auto keyLookup = Key{ QVariant::fromValue(nullptr),
                                    Key::Device::Keyboard,
                                    key->key(),
                                    Key::Direction::None };
        if (const auto found = config.find(keyLookup); found != config.end()) {
            releaseButton(*found, time);
        }
    }
    return false;
}
QString
InputTranslator::scancodeToString(const int scancode)
{
    return QKeySequence(scancode).toString(QKeySequence::NativeText);
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
