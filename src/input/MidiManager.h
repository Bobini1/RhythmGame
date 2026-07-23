//
// Created by Codex on 23.07.26.
//

#ifndef MIDIMANAGER_H
#define MIDIMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDataStream>
#include <libremidi/libremidi.hpp>
#include <memory>
#include <unordered_map>
#include <utility>

namespace input {
class MidiDevice
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(int index MEMBER index)

  public:
    QString name;
    int index{};

    auto operator<=>(const MidiDevice& device) const
    {
        return std::tie(name, index) <=>
               std::tie(device.name, device.index);
    }

    auto operator==(const MidiDevice& device) const -> bool;

    friend auto operator>>(QDataStream& stream, MidiDevice& device)
      -> QDataStream&;
    friend auto operator<<(QDataStream& stream, const MidiDevice& device)
      -> QDataStream&;
};

} // namespace input

template<>
struct std::hash<input::MidiDevice>
{
    auto operator()(const input::MidiDevice& s) const noexcept -> std::size_t;
};

namespace input {

class MidiManager : public QObject
{
    Q_OBJECT

    struct InputPort
    {
        std::unique_ptr<libremidi::midi_in> input;
        unsigned int portNumber{};
    };

    QTimer refreshTimer;
    std::unordered_map<MidiDevice, InputPort> inputs;

    void refreshPorts();
    void addInput(MidiDevice device, unsigned int portNumber);
    void processMessage(const MidiDevice& device,
                        const libremidi::message& message);

  public:
    explicit MidiManager(QObject* parent = nullptr);
    ~MidiManager() override;

  signals:
    void noteChanged(MidiDevice device,
                     int channel,
                     int note,
                     int velocity,
                     int64_t time);
    void controlChanged(MidiDevice device,
                        int channel,
                        int control,
                        int value,
                        int64_t time);
    void pitchBendChanged(MidiDevice device,
                          int channel,
                          int value,
                          int64_t time);
    void deviceRemoved(MidiDevice device, int64_t time);
};

} // namespace input

#endif // MIDIMANAGER_H
