#include "MidiManager.h"

#include <QHash>
#include <QList>
#include <chrono>
#include <spdlog/spdlog.h>

namespace input {
namespace {
auto
nowMs() -> int64_t
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

auto
deviceName(const libremidi::input_port& port) -> QString
{
    if (!port.display_name.empty()) {
        return QString::fromStdString(port.display_name);
    }
    if (!port.port_name.empty()) {
        return QString::fromStdString(port.port_name);
    }
    if (!port.device_name.empty()) {
        return QString::fromStdString(port.device_name);
    }
    return QStringLiteral("MIDI Input");
}

auto
deviceList(const std::vector<libremidi::input_port>& ports)
  -> QList<std::pair<MidiDevice, libremidi::input_port>>
{
    auto result = QList<std::pair<MidiDevice, libremidi::input_port>>{};
    auto seenNames = QHash<QString, int>{};
    for (const auto& port : ports) {
        auto name = deviceName(port);
        auto index = seenNames.value(name, 0);
        seenNames[name] = index + 1;
        result.append({ MidiDevice{ name, index }, port });
    }
    return result;
}
} // namespace

MidiManager::MidiManager(QObject* parent)
  : QObject(parent)
{
    connect(&refreshTimer, &QTimer::timeout, this, &MidiManager::refreshPorts);
    refreshTimer.setInterval(1000);
    refreshTimer.start();
    refreshPorts();
}

MidiManager::~MidiManager()
{
    refreshTimer.stop();
    inputs.clear();
}

void
MidiManager::refreshPorts()
{
    auto observer = std::unique_ptr<libremidi::observer>{};
    try {
        observer = std::make_unique<libremidi::observer>();
    } catch (const std::exception& error) {
        spdlog::warn("Could not initialize MIDI input: {}", error.what());
        return;
    }

    const auto devices = deviceList(observer->get_input_ports());
    auto present = QList<MidiDevice>{};
    for (const auto& [device, port] : devices) {
        present.append(device);
        if (!inputs.contains(device)) {
            addInput(device, port);
        } else {
            inputs[device].port = port;
        }
    }

    for (auto it = inputs.begin(); it != inputs.end();) {
        if (!present.contains(it->first)) {
            const auto device = it->first;
            it = inputs.erase(it);
            emit deviceRemoved(device, nowMs());
        } else {
            ++it;
        }
    }
}

void
MidiManager::addInput(MidiDevice device, libremidi::input_port port)
{
    try {
        auto input = std::make_unique<libremidi::midi_in>(
          libremidi::input_configuration{
            .on_message = [this, device](libremidi::message&& message) {
              processMessage(device, message);
            },
            .ignore_sysex = true,
            .ignore_timing = true,
            .ignore_sensing = true,
          });
        const auto error = input->open_port(port, "RhythmGame MIDI Input");
        if (error != stdx::error{}) {
            spdlog::warn("Could not open MIDI input port {}",
                         device.name.toStdString());
            return;
        }
        inputs.emplace(std::move(device),
                       InputPort{ std::move(input), std::move(port) });
    } catch (const std::exception& error) {
        spdlog::warn("Could not open MIDI input port {}: {}",
                     device.name.toStdString(),
                     error.what());
    }
}

void
MidiManager::processMessage(const MidiDevice& device,
                            const libremidi::message& message)
{
    if (message.size() < 2) {
        return;
    }
    const auto status = static_cast<unsigned char>(message[0]);
    const auto type = status & 0xf0;
    const auto channel = status & 0x0f;
    const auto data1 = static_cast<int>(message[1]);
    const auto data2 = message.size() >= 3 ? static_cast<int>(message[2]) : 0;
    const auto time = nowMs();
    switch (type) {
        case 0x80:
            emit noteChanged(device, channel, data1, 0, time);
            break;
        case 0x90:
            emit noteChanged(device, channel, data1, data2, time);
            break;
        case 0xb0:
            if (data1 == 64) {
                emit controlChanged(device, channel, data1, data2, time);
            }
            break;
        case 0xe0:
            emit pitchBendChanged(device,
                                  channel,
                                  (data2 << 7) | data1,
                                  time);
            break;
        default:
            break;
    }
}

auto
MidiDevice::operator==(const MidiDevice& device) const -> bool
{
    return std::tie(name, index) == std::tie(device.name, device.index);
}

auto
operator>>(QDataStream& stream, MidiDevice& device) -> QDataStream&
{
    return stream >> device.name >> device.index;
}

auto
operator<<(QDataStream& stream, const MidiDevice& device) -> QDataStream&
{
    return stream << device.name << device.index;
}
} // namespace input

auto
std::hash<input::MidiDevice>::operator()(const input::MidiDevice& s) const
  noexcept -> std::size_t
{
    return std::hash<QString>{}(s.name) ^ std::hash<int>{}(s.index);
}
