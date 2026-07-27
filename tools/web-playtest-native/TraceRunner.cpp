#include "gameplay_logic/SinglePlayerGameplayCore.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct Arguments
{
    std::filesystem::path chart;
    std::filesystem::path input;
    std::filesystem::path output;
};

auto
arguments(const int argc, char** argv) -> Arguments
{
    auto result = Arguments{};
    for (auto index = 1; index < argc; ++index) {
        const auto option = std::string_view{ argv[index] };
        if (index + 1 >= argc) {
            throw std::invalid_argument("Missing value after " +
                                        std::string{ option });
        }
        if (option == "--chart") {
            result.chart = argv[++index];
        } else if (option == "--input") {
            result.input = argv[++index];
        } else if (option == "--output") {
            result.output = argv[++index];
        } else {
            throw std::invalid_argument("Unknown option: " +
                                        std::string{ option });
        }
    }
    if (result.chart.empty() || result.input.empty() || result.output.empty()) {
        throw std::invalid_argument(
          "Usage: RhythmGame_web_playtest_trace --chart PATH --input PATH "
          "--output PATH");
    }
    return result;
}

auto
readBytes(const std::filesystem::path& path) -> std::string
{
    auto stream = std::ifstream{ path, std::ios::binary };
    if (!stream) {
        throw std::runtime_error("Unable to read " + path.string());
    }
    return { std::istreambuf_iterator<char>{ stream },
             std::istreambuf_iterator<char>{} };
}

auto
readInput(const std::filesystem::path& path) -> QJsonObject
{
    const auto bytes = readBytes(path);
    auto error = QJsonParseError{};
    const auto document =
      QJsonDocument::fromJson(QByteArray{ bytes.data(),
                                         static_cast<qsizetype>(bytes.size()) },
                              &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        throw std::invalid_argument("Invalid input JSON: " +
                                    error.errorString().toStdString());
    }
    return document.object();
}

auto
parseConfig(const QJsonObject& input) -> gameplay_logic::GameplayCoreConfig
{
    auto randomSequence = QList<qint64>{};
    for (const auto value : input.value("randomSequence").toArray()) {
        randomSequence.push_back(value.toInteger());
    }
    return {
        .play = {
          .randomSequence = std::move(randomSequence),
          .noteOrderP1 =
            static_cast<resource_managers::NoteOrderAlgorithm>(
              input.value("noteOrderP1").toInt()),
          .noteOrderP2 =
            static_cast<resource_managers::NoteOrderAlgorithm>(
              input.value("noteOrderP2").toInt()),
          .dpMode = static_cast<resource_managers::DpOptions>(
            input.value("dpMode").toInt()),
          .laneSeed =
            static_cast<std::uint64_t>(input.value("laneSeed").toInteger()),
        },
        .savedTimestampSeconds =
          input.value("savedTimestampSeconds").toInteger(),
        .scoreGuid = input.value("scoreGuid").toString(),
        .maxHitValue = input.value("maxHitValue").toDouble(2.0),
    };
}

void
writeBytes(const std::filesystem::path& path, const QByteArray& bytes)
{
    auto stream = std::ofstream{ path, std::ios::binary | std::ios::trunc };
    if (!stream) {
        throw std::runtime_error("Unable to write " + path.string());
    }
    stream.write(bytes.constData(), bytes.size());
    if (!stream) {
        throw std::runtime_error("Failed while writing " + path.string());
    }
}

} // namespace

int
main(int argc, char** argv)
{
    auto application = QCoreApplication{ argc, argv };
    try {
        const auto options = arguments(argc, argv);
        const auto input = readInput(options.input);
        const auto chartBytes = readBytes(options.chart);
        auto core = gameplay_logic::SinglePlayerGameplayCore::create(
          chartBytes, options.chart, parseConfig(input), {});
        if (input.value("preScheduleBgm").toBool()) {
            core->preScheduleBgm();
        }
        for (const auto value : input.value("events").toArray()) {
            const auto event = value.toObject();
            const auto time =
              std::chrono::nanoseconds{ event.value("timeNs").toInteger() };
            if (event.value("type").toString() == QStringLiteral("advance")) {
                core->advanceTo(time);
                continue;
            }
            const auto action =
              event.value("type").toString() == QStringLiteral("press")
                ? gameplay_logic::GameplayKeyAction::Press
                : gameplay_logic::GameplayKeyAction::Release;
            core->passKey(
              static_cast<input::BmsKey>(event.value("key").toInt()),
              action,
              time);
        }
        if (input.contains("finishTimeNs")) {
            core->advanceTo(std::chrono::nanoseconds{
              input.value("finishTimeNs").toInteger() });
        }
        writeBytes(options.output, core->finishTrace());
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
