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
deviceList(libremidi::midi_in& probe) -> QList<std::pair<MidiDevice, unsigned>>
{
    auto result = QList<std::pair<MidiDevice, unsigned>>{};
    auto seenNames = QHash<QString, int>{};
    const auto portCount = probe.get_port_count();
    for (auto port = 0u; port < portCount; ++port) {
        auto name = QString::fromStdString(probe.get_port_name(port));
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
    auto probe = std::unique_ptr<libremidi::midi_in>{};
    try {
        probe = std::make_unique<libremidi::midi_in>(
          libremidi::API::UNSPECIFIED, "RhythmGame MIDI Probe");
    } catch (const std::exception& error) {
        spdlog::warn("Could not initialize MIDI input: {}", error.what());
        return;
    }

    const auto devices = deviceList(*probe);
    auto present = QList<MidiDevice>{};
    for (const auto& [device, portNumber] : devices) {
        present.append(device);
        if (!inputs.contains(device)) {
            addInput(device, portNumber);
        } else {
            inputs[device].portNumber = portNumber;
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
MidiManager::addInput(MidiDevice device, unsigned int portNumber)
{
    try {
        auto input = std::make_unique<libremidi::midi_in>(
          libremidi::API::UNSPECIFIED, "RhythmGame MIDI Input");
        input->ignore_types(true, true, true);
        input->set_callback(
          [this, device](const libremidi::message& message) {
              processMessage(device, message);
          });
        input->open_port(portNumber, "RhythmGame MIDI Input");
        inputs.emplace(std::move(device), InputPort{ std::move(input),
                                                    portNumber });
    } catch (const std::exception& error) {
        spdlog::warn("Could not open MIDI input port {}: {}",
                     portNumber,
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
