#include "db/SqliteCppDb.h"
#include "input/InputTranslator.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <utility>
#include <vector>

namespace {
void
ensureCoreApplication()
{
    if (QCoreApplication::instance() != nullptr) {
        return;
    }
    static auto argc = 1;
    static char appName[] = "RhythmGame_test";
    static char* argv[] = { appName, nullptr };
    static auto application = std::make_unique<QCoreApplication>(argc, argv);
}

void
runEventLoopFor(int milliseconds)
{
    auto loop = QEventLoop{};
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

class InputTranslatorHarness
{
    db::SqliteCppDb db{ ":memory:" };

  public:
    std::unique_ptr<input::InputTranslator> translator;

    InputTranslatorHarness()
    {
        using input::BmsKey;
        using input::Key;
        using input::Mapping;

        ensureCoreApplication();
        db.execute("CREATE TABLE properties (key TEXT PRIMARY KEY, value)");
        translator = std::make_unique<input::InputTranslator>(&db);
        translator->setKeyConfig(QList<Mapping>{
          Mapping{ Key{ QVariant::fromValue(nullptr),
                        Key::Device::Keyboard,
                        42,
                        Key::Direction::None },
                   BmsKey::Col11 },
          Mapping{ Key{ QVariant::fromValue(nullptr),
                        Key::Device::Keyboard,
                        43,
                        Key::Direction::None },
                   BmsKey::Col12 },
        });
    }
};

auto
testMidiDevice() -> input::MidiDevice
{
    return input::MidiDevice{ QStringLiteral("Test MIDI"), 0 };
}

using ButtonEvent = std::pair<input::BmsKey, int64_t>;

void
recordButtonEvents(input::InputTranslator& translator,
                   std::vector<ButtonEvent>& presses,
                   std::vector<ButtonEvent>& releases)
{
    QObject::connect(&translator,
                     &input::InputTranslator::buttonPressed,
                     [&presses](input::BmsKey button, int64_t time) {
                         presses.emplace_back(button, time);
                     });
    QObject::connect(&translator,
                     &input::InputTranslator::buttonReleased,
                     [&releases](input::BmsKey button, int64_t time) {
                         releases.emplace_back(button, time);
                     });
}
}

TEST_CASE("beatoraja analog scratch ticks use wrapped 0.009 axis quanta")
{
    using input::InputTranslator;

    CHECK(InputTranslator::computeAnalogScratchTicks(0.0, 0.0) == 0);
    CHECK(InputTranslator::computeAnalogScratchTicks(0.0, 0.009) == 1);
    CHECK(InputTranslator::computeAnalogScratchTicks(0.0, -0.009) == -1);
    CHECK(InputTranslator::computeAnalogScratchTicks(0.0, 0.010) == 2);
    CHECK(InputTranslator::computeAnalogScratchTicks(0.0, -0.010) == -2);

    CHECK(InputTranslator::computeAnalogScratchTicks(0.99, -0.99) == 3);
    CHECK(InputTranslator::computeAnalogScratchTicks(-0.99, 0.99) == -3);
}

TEST_CASE("analog scratch auto-release uses the latest movement timestamp",
          "[input][analog-scratch]")
{
    using input::BmsKey;
    using input::Gamepad;
    using input::Key;
    using input::Mapping;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    const auto gamepad = Gamepad{ "Test controller", "test-guid", 0 };
    constexpr auto axis = Uint8{ 1 };
    translator.setKeyConfig(QList<Mapping>{
      Mapping{ Key{ QVariant::fromValue(gamepad),
                    Key::Device::Axis,
                    axis,
                    Key::Direction::Up },
               BmsKey::Col1sUp },
      Mapping{ Key{ QVariant::fromValue(gamepad),
                    Key::Device::Axis,
                    axis,
                    Key::Direction::Down },
               BmsKey::Col1sDown },
    });
    auto* const analogConfig = translator.getAnalogAxisConfig1();
    REQUIRE(analogConfig != nullptr);
    analogConfig->setTimeout(10);

    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleAxis(gamepad, axis, 0.0, 1'000);
    translator.handleAxis(gamepad, axis, 0.010, 1'010);
    translator.handleAxis(gamepad, axis, 0.020, 2'000);
    runEventLoopFor(25);

    REQUIRE(presses.size() == 1);
    CHECK(presses.front() == ButtonEvent{ BmsKey::Col1sUp, 1'010 });
    REQUIRE(releases.size() == 1);
    CHECK(releases.front() == ButtonEvent{ BmsKey::Col1sUp, 2'010 });
}

TEST_CASE("debounce preserves a held key across press chatter",
          "[input][debounce]")
{
    using input::BmsKey;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    translator.setDebounceMs(10.0);
    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleKeyEvent(42, true, 1'000);
    translator.handleKeyEvent(42, false, 1'001);

    CHECK(translator.col11());
    CHECK(releases.empty());

    translator.handleKeyEvent(42, true, 1'002);
    runEventLoopFor(25);

    CHECK(translator.col11());
    REQUIRE(presses.size() == 1);
    CHECK(presses.front() == ButtonEvent{ BmsKey::Col11, 1'000 });
    CHECK(releases.empty());

    translator.handleKeyEvent(42, false, 1'020);
    CHECK(translator.col11());
    runEventLoopFor(25);

    CHECK_FALSE(translator.col11());
    REQUIRE(releases.size() == 1);
    CHECK(releases.front() == ButtonEvent{ BmsKey::Col11, 1'020 });
}

TEST_CASE("debounce commits only the stable release edge", "[input][debounce]")
{
    using input::BmsKey;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    translator.setDebounceMs(10.0);
    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleKeyEvent(42, true, 1'000);
    translator.handleKeyEvent(42, false, 1'001);
    translator.handleKeyEvent(42, true, 1'002);
    translator.handleKeyEvent(42, false, 1'003);
    runEventLoopFor(25);

    CHECK_FALSE(translator.col11());
    REQUIRE(presses.size() == 1);
    CHECK(presses.front() == ButtonEvent{ BmsKey::Col11, 1'000 });
    REQUIRE(releases.size() == 1);
    CHECK(releases.front() == ButtonEvent{ BmsKey::Col11, 1'003 });
}

TEST_CASE("a press at the debounce deadline starts a new hit",
          "[input][debounce]")
{
    using input::BmsKey;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    translator.setDebounceMs(10.0);
    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleKeyEvent(42, true, 1'000);
    translator.handleKeyEvent(42, false, 1'001);
    translator.handleKeyEvent(42, true, 1'011);

    CHECK(translator.col11());
    REQUIRE(presses.size() == 2);
    CHECK(presses.back() == ButtonEvent{ BmsKey::Col11, 1'011 });
    REQUIRE(releases.size() == 1);
    CHECK(releases.front() == ButtonEvent{ BmsKey::Col11, 1'001 });
}

TEST_CASE("debounce state and timers are independent for every key",
          "[input][debounce]")
{
    using input::BmsKey;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    translator.setDebounceMs(10.0);
    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleKeyEvent(42, true, 2'000);
    translator.handleKeyEvent(43, true, 2'000);
    translator.handleKeyEvent(42, false, 2'001);
    translator.handleKeyEvent(43, false, 2'002);
    translator.handleKeyEvent(42, true, 2'003);
    runEventLoopFor(25);

    CHECK(translator.col11());
    CHECK_FALSE(translator.col12());
    REQUIRE(presses.size() == 2);
    REQUIRE(releases.size() == 1);
    CHECK(releases.front() == ButtonEvent{ BmsKey::Col12, 2'002 });

    translator.handleKeyEvent(42, false, 2'030);
    runEventLoopFor(25);

    CHECK_FALSE(translator.col11());
    REQUIRE(releases.size() == 2);
    CHECK(releases.back() == ButtonEvent{ BmsKey::Col11, 2'030 });
}

TEST_CASE("zero debounce releases immediately", "[input][debounce]")
{
    using input::BmsKey;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    translator.setDebounceMs(0.0);
    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleKeyEvent(42, true, 3'000);
    translator.handleKeyEvent(42, false, 3'001);

    CHECK_FALSE(translator.col11());
    REQUIRE(presses.size() == 1);
    REQUIRE(releases.size() == 1);
    CHECK(releases.front() == ButtonEvent{ BmsKey::Col11, 3'001 });
}

TEST_CASE("setting debounce to zero commits pending releases",
          "[input][debounce]")
{
    using input::BmsKey;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    translator.setDebounceMs(10.0);
    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleKeyEvent(42, true, 4'000);
    translator.handleKeyEvent(42, false, 4'001);
    REQUIRE(translator.col11());
    REQUIRE(releases.empty());

    translator.setDebounceMs(0.0);

    CHECK_FALSE(translator.col11());
    REQUIRE(releases.size() == 1);
    CHECK(releases.front() == ButtonEvent{ BmsKey::Col11, 4'001 });
}

TEST_CASE("changing positive debounce restarts each pending key deadline",
          "[input][debounce]")
{
    using input::BmsKey;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    translator.setDebounceMs(30.0);
    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleKeyEvent(42, true, 5'000);
    translator.handleKeyEvent(42, false, 5'001);
    translator.setDebounceMs(10.0);
    translator.handleKeyEvent(42, true, 5'020);
    runEventLoopFor(25);

    CHECK(translator.col11());
    REQUIRE(presses.size() == 1);
    CHECK(presses.front() == ButtonEvent{ BmsKey::Col11, 5'000 });
    CHECK(releases.empty());

    translator.handleKeyEvent(42, false, 5'030);
    runEventLoopFor(25);

    CHECK_FALSE(translator.col11());
    REQUIRE(releases.size() == 1);
    CHECK(releases.front() == ButtonEvent{ BmsKey::Col11, 5'030 });
}

TEST_CASE("debounce property resets to the five millisecond default",
          "[input][debounce]")
{
    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;

    translator.setDebounceMs(20.0);
    translator.resetDebounceMs();

    CHECK(translator.getDebounceMs() == 5.0);
}

TEST_CASE("MIDI note input follows note on and note off", "[input][midi]")
{
    using input::BmsKey;
    using input::Key;
    using input::Mapping;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    translator.setDebounceMs(0.0);
    auto device = testMidiDevice();
    translator.setKeyConfig(QList<Mapping>{
      Mapping{ Key{ QVariant::fromValue(device),
                    Key::Device::MidiNote,
                    (0 << 8) | 60,
                    Key::Direction::None },
               BmsKey::Col11 },
    });
    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleMidiNote(device, 0, 60, 100, 6'000);
    translator.handleMidiNote(device, 0, 60, 0, 6'010);

    REQUIRE(presses.size() == 1);
    CHECK(presses.front() == ButtonEvent{ BmsKey::Col11, 6'000 });
    REQUIRE(releases.size() == 1);
    CHECK(releases.front() == ButtonEvent{ BmsKey::Col11, 6'010 });
}

TEST_CASE("MIDI sustain pedal uses OpenLR2 digital threshold", "[input][midi]")
{
    using input::BmsKey;
    using input::Key;
    using input::Mapping;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    translator.setDebounceMs(0.0);
    auto device = testMidiDevice();
    translator.setKeyConfig(QList<Mapping>{
      Mapping{ Key{ QVariant::fromValue(device),
                    Key::Device::MidiControl,
                    (0 << 8) | 64,
                    Key::Direction::None },
               BmsKey::Col12 },
    });
    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleMidiControl(device, 0, 64, 63, 7'000);
    translator.handleMidiControl(device, 0, 64, 64, 7'010);
    translator.handleMidiControl(device, 0, 64, 0, 7'020);

    REQUIRE(presses.size() == 1);
    CHECK(presses.front() == ButtonEvent{ BmsKey::Col12, 7'010 });
    REQUIRE(releases.size() == 1);
    CHECK(releases.front() == ButtonEvent{ BmsKey::Col12, 7'020 });
}

TEST_CASE("MIDI pitch bend exposes positive and negative digital directions",
          "[input][midi]")
{
    using input::BmsKey;
    using input::Key;
    using input::Mapping;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    auto device = testMidiDevice();
    translator.setKeyConfig(QList<Mapping>{
      Mapping{ Key{ QVariant::fromValue(device),
                    Key::Device::MidiPitchBend,
                    0 << 8,
                    Key::Direction::Up },
               BmsKey::Col1sUp },
      Mapping{ Key{ QVariant::fromValue(device),
                    Key::Device::MidiPitchBend,
                    0 << 8,
                    Key::Direction::Down },
               BmsKey::Col1sDown },
    });
    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleMidiPitchBend(device, 0, 9'000, 8'000);
    translator.handleMidiPitchBend(device, 0, 7'000, 8'010);
    translator.handleMidiPitchBend(device, 0, 8'192, 8'020);

    REQUIRE(presses.size() == 2);
    CHECK(presses[0] == ButtonEvent{ BmsKey::Col1sUp, 8'000 });
    CHECK(presses[1] == ButtonEvent{ BmsKey::Col1sDown, 8'010 });
    REQUIRE(releases.size() == 2);
    CHECK(releases[0] == ButtonEvent{ BmsKey::Col1sUp, 8'010 });
    CHECK(releases[1] == ButtonEvent{ BmsKey::Col1sDown, 8'020 });
}

TEST_CASE("MIDI device removal releases mapped held keys", "[input][midi]")
{
    using input::BmsKey;
    using input::Key;
    using input::Mapping;

    auto harness = InputTranslatorHarness{};
    auto& translator = *harness.translator;
    auto device = testMidiDevice();
    translator.setKeyConfig(QList<Mapping>{
      Mapping{ Key{ QVariant::fromValue(device),
                    Key::Device::MidiNote,
                    (0 << 8) | 60,
                    Key::Direction::None },
               BmsKey::Col11 },
    });
    auto presses = std::vector<ButtonEvent>{};
    auto releases = std::vector<ButtonEvent>{};
    recordButtonEvents(translator, presses, releases);

    translator.handleMidiNote(device, 0, 60, 100, 9'000);
    translator.handleMidiDeviceRemoved(device, 9'050);

    REQUIRE(presses.size() == 1);
    REQUIRE(releases.size() == 1);
    CHECK(releases.front() == ButtonEvent{ BmsKey::Col11, 9'050 });
}
