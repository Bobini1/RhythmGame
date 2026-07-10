#include "ArenaProtocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <type_traits>
#include <utility>

namespace arena {
namespace {

struct DecodeFailure
{
    ProtocolFailureCode code;
};

[[noreturn]] void
fail(ProtocolFailureCode code = ProtocolFailureCode::MalformedMessage)
{
    throw DecodeFailure{ code };
}

auto
keySet(std::initializer_list<const char*> keys) -> QSet<QString>
{
    QSet<QString> result;
    for (const auto* key : keys) {
        result.insert(QString::fromLatin1(key));
    }
    return result;
}

void
requireExactKeys(const QJsonObject& object,
                 std::initializer_list<const char*> required,
                 std::initializer_list<const char*> optional = {})
{
    const auto requiredKeys = keySet(required);
    auto allowedKeys = requiredKeys;
    allowedKeys.unite(keySet(optional));
    for (const auto& key : requiredKeys) {
        if (!object.contains(key)) {
            fail();
        }
    }
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (!allowedKeys.contains(it.key())) {
            fail();
        }
    }
}

auto
hasExactKeys(const QJsonObject& object, std::initializer_list<const char*> keys)
  -> bool
{
    QSet<QString> actual;
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        actual.insert(it.key());
    }
    return actual == keySet(keys);
}

auto
requiredString(const QJsonObject& object, const char* key) -> QString
{
    const auto value = object.value(QString::fromLatin1(key));
    if (!value.isString()) {
        fail();
    }
    return value.toString();
}

auto
requiredObject(const QJsonObject& object, const char* key) -> QJsonObject
{
    const auto value = object.value(QString::fromLatin1(key));
    if (!value.isObject()) {
        fail();
    }
    return value.toObject();
}

auto
requiredArray(const QJsonObject& object, const char* key) -> QJsonArray
{
    const auto value = object.value(QString::fromLatin1(key));
    if (!value.isArray()) {
        fail();
    }
    return value.toArray();
}

auto
requiredBool(const QJsonObject& object, const char* key) -> bool
{
    const auto value = object.value(QString::fromLatin1(key));
    if (!value.isBool()) {
        fail();
    }
    return value.toBool();
}

auto
nullableString(const QJsonObject& object, const char* key)
  -> std::optional<QString>
{
    const auto value = object.value(QString::fromLatin1(key));
    if (value.isNull()) {
        return std::nullopt;
    }
    if (!value.isString()) {
        fail();
    }
    return value.toString();
}

auto
safeInteger(const QJsonValue& value, bool positive) -> qint64
{
    if (!value.isDouble()) {
        fail();
    }
    const auto number = value.toDouble();
    const auto minimum = positive ? 1.0 : 0.0;
    if (!std::isfinite(number) || std::trunc(number) != number ||
        number < minimum || number > static_cast<double>(MaxJsonSafeInteger)) {
        fail();
    }
    return static_cast<qint64>(number);
}

auto
requiredSafeInteger(const QJsonObject& object,
                    const char* key,
                    bool positive = false) -> qint64
{
    return safeInteger(object.value(QString::fromLatin1(key)), positive);
}

auto
unicodeCodePointCount(QStringView value) -> std::optional<int>
{
    int count = 0;
    for (qsizetype i = 0; i < value.size(); ++i) {
        const auto current = value[i];
        if (current.isHighSurrogate()) {
            if (i + 1 >= value.size() || !value[i + 1].isLowSurrogate()) {
                return std::nullopt;
            }
            ++i;
        } else if (current.isLowSurrogate()) {
            return std::nullopt;
        }
        ++count;
    }
    return count;
}

auto
validCodePointString(QStringView value, int maximum) -> bool
{
    const auto count = unicodeCodePointCount(value);
    return count && *count >= 1 && *count <= maximum;
}

auto
validPossiblyEmptyCodePointString(QStringView value, int maximum) -> bool
{
    const auto count = unicodeCodePointCount(value);
    return count && *count <= maximum;
}

auto
isLowerHex(QStringView value, qsizetype expectedSize) -> bool
{
    return value.size() == expectedSize &&
           std::all_of(value.begin(), value.end(), [](QChar ch) {
               const auto c = ch.unicode();
               return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
           });
}

auto
isTransferId(QStringView value) -> bool
{
    return value.size() == TransferIdCharacters &&
           std::all_of(value.begin(), value.end(), [](QChar ch) {
               const auto c = ch.unicode();
               return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                      (c >= '0' && c <= '9') || c == '_' || c == '-';
           });
}

auto
isEcmaWhitespace(QChar ch) -> bool
{
    const auto c = ch.unicode();
    return (c >= 0x0009 && c <= 0x000d) || c == 0x0020 || c == 0x00a0 ||
           c == 0x1680 || (c >= 0x2000 && c <= 0x200a) || c == 0x2028 ||
           c == 0x2029 || c == 0x202f || c == 0x205f || c == 0x3000 ||
           c == 0xfeff;
}

auto
ecmaTrim(QString value) -> QString
{
    qsizetype begin = 0;
    qsizetype end = value.size();
    while (begin < end && isEcmaWhitespace(value[begin])) {
        ++begin;
    }
    while (end > begin && isEcmaWhitespace(value[end - 1])) {
        --end;
    }
    return value.mid(begin, end - begin);
}

void
requireValidUnicode(QStringView value)
{
    if (!unicodeCodePointCount(value)) {
        fail();
    }
}

auto
isSafeIdentifier(QStringView value, int maximum) -> bool
{
    if (value.isEmpty() || value.size() > maximum) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](QChar ch) {
        const auto c = ch.unicode();
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '.' || c == '_' || c == ':' ||
               c == '-';
    });
}

auto
isCapability(QStringView value) -> bool
{
    if (value.isEmpty() || value.size() > MaxCapabilityCharacters) {
        return false;
    }
    for (qsizetype i = 0; i < value.size(); ++i) {
        const auto c = value[i].unicode();
        const auto alphaNumeric = (c >= 'A' && c <= 'Z') ||
                                  (c >= 'a' && c <= 'z') ||
                                  (c >= '0' && c <= '9');
        if (!alphaNumeric && (i == 0 || (c != '.' && c != '_' && c != '-'))) {
            return false;
        }
    }
    return true;
}

void
validateOpaqueId(QStringView id);
void
validatePositiveGeneration(qint64 value);

auto
parseCapabilities(const QJsonArray& array, bool server, int protocolMinor)
  -> QStringList
{
    if (array.isEmpty() || (!server && array.size() > MaxCapabilities)) {
        fail();
    }
    QStringList result;
    QSet<QString> seen;
    for (const auto& value : array) {
        if (!value.isString() || !isCapability(value.toString())) {
            fail();
        }
        const auto capability = value.toString();
        if (seen.contains(capability)) {
            fail();
        }
        seen.insert(capability);
        result.push_back(capability);
    }
    if (!result.contains(QString::fromLatin1(RoomsCapability))) {
        fail(ProtocolFailureCode::CapabilityRequired);
    }
    const auto hasRounds =
      result.contains(QString::fromLatin1(RoundsCapability));
    const auto hasCompetition =
      result.contains(QString::fromLatin1(CompetitionCapability));
    if (hasCompetition && !hasRounds) {
        fail();
    }
    if (server) {
        const auto roomsOnly =
          QStringList{ QString::fromLatin1(RoomsCapability) };
        const auto roomsAndRounds =
          QStringList{ QString::fromLatin1(RoomsCapability),
                       QString::fromLatin1(RoundsCapability) };
        const auto allCapabilities =
          QStringList{ QString::fromLatin1(RoomsCapability),
                       QString::fromLatin1(RoundsCapability),
                       QString::fromLatin1(CompetitionCapability) };
        if (result != roomsOnly && result != roomsAndRounds &&
            result != allCapabilities) {
            fail();
        }
        if (protocolMinor == LegacyProtocolMinor && result != roomsOnly) {
            fail();
        }
        if (protocolMinor == RoundsProtocolMinor && hasCompetition) {
            fail();
        }
    }
    return result;
}

auto
parseNoteOrder(QStringView value) -> NoteOrder
{
    static const std::pair<QStringView, NoteOrder> entries[]{
        { u"normal", NoteOrder::Normal },
        { u"mirror", NoteOrder::Mirror },
        { u"random", NoteOrder::Random },
        { u"s_random", NoteOrder::SRandom },
        { u"r_random", NoteOrder::RRandom },
        { u"random_plus", NoteOrder::RandomPlus },
        { u"s_random_plus", NoteOrder::SRandomPlus },
        { u"beatoraja_random", NoteOrder::BeatorajaRandom },
        { u"beatoraja_random_ex", NoteOrder::BeatorajaRandomEx },
        { u"lr2_random", NoteOrder::Lr2Random },
        { u"lr2_random_ex", NoteOrder::Lr2RandomEx },
    };
    for (const auto& [spelling, order] : entries) {
        if (value == spelling) {
            return order;
        }
    }
    fail();
}

auto
noteOrderString(NoteOrder value) -> QString
{
    switch (value) {
        case NoteOrder::Normal:
            return QStringLiteral("normal");
        case NoteOrder::Mirror:
            return QStringLiteral("mirror");
        case NoteOrder::Random:
            return QStringLiteral("random");
        case NoteOrder::SRandom:
            return QStringLiteral("s_random");
        case NoteOrder::RRandom:
            return QStringLiteral("r_random");
        case NoteOrder::RandomPlus:
            return QStringLiteral("random_plus");
        case NoteOrder::SRandomPlus:
            return QStringLiteral("s_random_plus");
        case NoteOrder::BeatorajaRandom:
            return QStringLiteral("beatoraja_random");
        case NoteOrder::BeatorajaRandomEx:
            return QStringLiteral("beatoraja_random_ex");
        case NoteOrder::Lr2Random:
            return QStringLiteral("lr2_random");
        case NoteOrder::Lr2RandomEx:
            return QStringLiteral("lr2_random_ex");
    }
    fail();
}

auto
parseDpMode(QStringView value) -> DpMode
{
    if (value == QStringLiteral("off")) {
        return DpMode::Off;
    }
    if (value == QStringLiteral("flip")) {
        return DpMode::Flip;
    }
    if (value == QStringLiteral("lr2_flip")) {
        return DpMode::Lr2Flip;
    }
    if (value == QStringLiteral("battle")) {
        return DpMode::Battle;
    }
    fail();
}

auto
dpModeString(DpMode value) -> QString
{
    switch (value) {
        case DpMode::Off:
            return QStringLiteral("off");
        case DpMode::Flip:
            return QStringLiteral("flip");
        case DpMode::Lr2Flip:
            return QStringLiteral("lr2_flip");
        case DpMode::Battle:
            return QStringLiteral("battle");
    }
    fail();
}

auto
parseGaugeType(QStringView value) -> GaugeType
{
    static const std::pair<QStringView, GaugeType> entries[]{
        { u"fc", GaugeType::Fc },     { u"exhard", GaugeType::ExHard },
        { u"hard", GaugeType::Hard }, { u"normal", GaugeType::Normal },
        { u"easy", GaugeType::Easy }, { u"aeasy", GaugeType::AssistEasy },
    };
    for (const auto& [spelling, type] : entries) {
        if (value == spelling) {
            return type;
        }
    }
    fail();
}

auto
gaugeTypeString(GaugeType value) -> QString
{
    switch (value) {
        case GaugeType::Fc:
            return QStringLiteral("fc");
        case GaugeType::ExHard:
            return QStringLiteral("exhard");
        case GaugeType::Hard:
            return QStringLiteral("hard");
        case GaugeType::Normal:
            return QStringLiteral("normal");
        case GaugeType::Easy:
            return QStringLiteral("easy");
        case GaugeType::AssistEasy:
            return QStringLiteral("aeasy");
    }
    fail();
}

auto
parseClearType(QStringView value) -> ClearType
{
    static const std::pair<QStringView, ClearType> entries[]{
        { u"max", ClearType::Max },       { u"perfect", ClearType::Perfect },
        { u"fc", ClearType::FullCombo },  { u"exhard", ClearType::ExHard },
        { u"hard", ClearType::Hard },     { u"normal", ClearType::Normal },
        { u"easy", ClearType::Easy },     { u"aeasy", ClearType::AssistEasy },
        { u"failed", ClearType::Failed },
    };
    for (const auto& [spelling, type] : entries) {
        if (value == spelling) {
            return type;
        }
    }
    fail();
}

auto
clearTypeString(ClearType value) -> QString
{
    switch (value) {
        case ClearType::Max:
            return QStringLiteral("max");
        case ClearType::Perfect:
            return QStringLiteral("perfect");
        case ClearType::FullCombo:
            return QStringLiteral("fc");
        case ClearType::ExHard:
            return QStringLiteral("exhard");
        case ClearType::Hard:
            return QStringLiteral("hard");
        case ClearType::Normal:
            return QStringLiteral("normal");
        case ClearType::Easy:
            return QStringLiteral("easy");
        case ClearType::AssistEasy:
            return QStringLiteral("aeasy");
        case ClearType::Failed:
            return QStringLiteral("failed");
    }
    fail();
}

auto
parseDnfReason(QStringView value) -> DnfReason
{
    static const std::pair<QStringView, DnfReason> entries[]{
        { u"aborted", DnfReason::Aborted },
        { u"result_unavailable", DnfReason::ResultUnavailable },
        { u"left", DnfReason::Left },
        { u"kicked", DnfReason::Kicked },
        { u"grace_expired", DnfReason::GraceExpired },
        { u"play_deadline", DnfReason::PlayDeadline },
    };
    for (const auto& [spelling, reason] : entries) {
        if (value == spelling) {
            return reason;
        }
    }
    fail();
}

auto
dnfReasonString(DnfReason value) -> QString
{
    switch (value) {
        case DnfReason::Aborted:
            return QStringLiteral("aborted");
        case DnfReason::ResultUnavailable:
            return QStringLiteral("result_unavailable");
        case DnfReason::Left:
            return QStringLiteral("left");
        case DnfReason::Kicked:
            return QStringLiteral("kicked");
        case DnfReason::GraceExpired:
            return QStringLiteral("grace_expired");
        case DnfReason::PlayDeadline:
            return QStringLiteral("play_deadline");
    }
    fail();
}

auto
scoreCounter(const QJsonObject& object, const char* key) -> qint64
{
    const auto value = requiredSafeInteger(object, key);
    if (value > MaxScoreCounter) {
        fail();
    }
    return value;
}

void
validateScoreCounter(qint64 value)
{
    if (value < 0 || value > MaxScoreCounter) {
        fail();
    }
}

auto
parseJudgements(const QJsonObject& object) -> ArenaJudgements
{
    requireExactKeys(
      object, { "perfect", "great", "good", "bad", "poor", "emptyPoor" });
    return { .perfect = scoreCounter(object, "perfect"),
             .great = scoreCounter(object, "great"),
             .good = scoreCounter(object, "good"),
             .bad = scoreCounter(object, "bad"),
             .poor = scoreCounter(object, "poor"),
             .emptyPoor = scoreCounter(object, "emptyPoor") };
}

auto
encodeJudgements(const ArenaJudgements& value) -> QJsonObject
{
    validateScoreCounter(value.perfect);
    validateScoreCounter(value.great);
    validateScoreCounter(value.good);
    validateScoreCounter(value.bad);
    validateScoreCounter(value.poor);
    validateScoreCounter(value.emptyPoor);
    return { { QStringLiteral("perfect"), value.perfect },
             { QStringLiteral("great"), value.great },
             { QStringLiteral("good"), value.good },
             { QStringLiteral("bad"), value.bad },
             { QStringLiteral("poor"), value.poor },
             { QStringLiteral("emptyPoor"), value.emptyPoor } };
}

auto
parseGauge(const QJsonObject& object) -> GaugeSnapshot
{
    requireExactKeys(object, { "type", "valueMilli" });
    const auto valueMilli = requiredSafeInteger(object, "valueMilli");
    if (valueMilli > 100'000) {
        fail();
    }
    return { .type = parseGaugeType(requiredString(object, "type")),
             .valueMilli = valueMilli };
}

auto
encodeGauge(const GaugeSnapshot& value) -> QJsonObject
{
    if (value.valueMilli < 0 || value.valueMilli > 100'000) {
        fail();
    }
    return { { QStringLiteral("type"), gaugeTypeString(value.type) },
             { QStringLiteral("valueMilli"), value.valueMilli } };
}

void
validateCompetitionScore(qint64 exScore,
                         qint64 badPoorCount,
                         const ArenaJudgements& judgements)
{
    validateScoreCounter(exScore);
    validateScoreCounter(badPoorCount);
    if (exScore != 2 * judgements.perfect + judgements.great ||
        badPoorCount !=
          judgements.bad + judgements.poor + judgements.emptyPoor) {
        fail();
    }
}

auto
parseTelemetry(const QJsonObject& object) -> TelemetrySnapshot
{
    requireExactKeys(object,
                     { "sequence",
                       "exScore",
                       "progressPermille",
                       "maxCombo",
                       "badPoorCount",
                       "judgements",
                       "gauge",
                       "playStatus" });
    const auto sequence = requiredSafeInteger(object, "sequence", true);
    const auto progress = requiredSafeInteger(object, "progressPermille");
    if (sequence > MaxUInt32 || progress > 1'000 ||
        requiredString(object, "playStatus") != QStringLiteral("playing")) {
        fail();
    }
    TelemetrySnapshot result{
        .sequence = sequence,
        .exScore = scoreCounter(object, "exScore"),
        .progressPermille = progress,
        .maxCombo = scoreCounter(object, "maxCombo"),
        .badPoorCount = scoreCounter(object, "badPoorCount"),
        .judgements = parseJudgements(requiredObject(object, "judgements")),
        .gauge = parseGauge(requiredObject(object, "gauge")),
    };
    validateCompetitionScore(
      result.exScore, result.badPoorCount, result.judgements);
    return result;
}

auto
encodeTelemetry(const TelemetrySnapshot& value) -> QJsonObject
{
    if (value.sequence < 1 || value.sequence > MaxUInt32 ||
        value.progressPermille < 0 || value.progressPermille > 1'000) {
        fail();
    }
    validateScoreCounter(value.maxCombo);
    const auto judgements = encodeJudgements(value.judgements);
    validateCompetitionScore(
      value.exScore, value.badPoorCount, value.judgements);
    return { { QStringLiteral("sequence"), value.sequence },
             { QStringLiteral("exScore"), value.exScore },
             { QStringLiteral("progressPermille"), value.progressPermille },
             { QStringLiteral("maxCombo"), value.maxCombo },
             { QStringLiteral("badPoorCount"), value.badPoorCount },
             { QStringLiteral("judgements"), judgements },
             { QStringLiteral("gauge"), encodeGauge(value.gauge) },
             { QStringLiteral("playStatus"), QStringLiteral("playing") } };
}

auto
parseFinalResult(const QJsonObject& object) -> FinalResult
{
    requireExactKeys(object,
                     { "exScore",
                       "maxCombo",
                       "badPoorCount",
                       "judgements",
                       "clearType",
                       "finalGauge" });
    FinalResult result{
        .exScore = scoreCounter(object, "exScore"),
        .maxCombo = scoreCounter(object, "maxCombo"),
        .badPoorCount = scoreCounter(object, "badPoorCount"),
        .judgements = parseJudgements(requiredObject(object, "judgements")),
        .clearType = parseClearType(requiredString(object, "clearType")),
        .finalGauge = parseGauge(requiredObject(object, "finalGauge")),
    };
    validateCompetitionScore(
      result.exScore, result.badPoorCount, result.judgements);
    return result;
}

auto
encodeFinalResult(const FinalResult& value) -> QJsonObject
{
    validateScoreCounter(value.maxCombo);
    const auto judgements = encodeJudgements(value.judgements);
    validateCompetitionScore(
      value.exScore, value.badPoorCount, value.judgements);
    return { { QStringLiteral("exScore"), value.exScore },
             { QStringLiteral("maxCombo"), value.maxCombo },
             { QStringLiteral("badPoorCount"), value.badPoorCount },
             { QStringLiteral("judgements"), judgements },
             { QStringLiteral("clearType"), clearTypeString(value.clearType) },
             { QStringLiteral("finalGauge"), encodeGauge(value.finalGauge) } };
}

auto
parseRoomPhase(QStringView value) -> RoomPhase
{
    if (value == QStringLiteral("selecting")) {
        return RoomPhase::Selecting;
    }
    if (value == QStringLiteral("loading")) {
        return RoomPhase::Loading;
    }
    if (value == QStringLiteral("playing")) {
        return RoomPhase::Playing;
    }
    fail();
}

auto
parseInventoryState(QStringView value) -> InventoryState
{
    if (value == QStringLiteral("missing")) {
        return InventoryState::Missing;
    }
    if (value == QStringLiteral("syncing")) {
        return InventoryState::Syncing;
    }
    if (value == QStringLiteral("ready")) {
        return InventoryState::Ready;
    }
    fail();
}

auto
parseMemberRoundState(QStringView value) -> MemberRoundState
{
    if (value == QStringLiteral("eligible")) {
        return MemberRoundState::Eligible;
    }
    if (value == QStringLiteral("waiting")) {
        return MemberRoundState::Waiting;
    }
    if (value == QStringLiteral("probing")) {
        return MemberRoundState::Probing;
    }
    if (value == QStringLiteral("loading")) {
        return MemberRoundState::Loading;
    }
    if (value == QStringLiteral("loaded")) {
        return MemberRoundState::Loaded;
    }
    if (value == QStringLiteral("playing")) {
        return MemberRoundState::Playing;
    }
    fail();
}

auto
parseFrozenRoundStage(QStringView value) -> FrozenRoundStage
{
    if (value == QStringLiteral("probing")) {
        return FrozenRoundStage::Probing;
    }
    if (value == QStringLiteral("loading")) {
        return FrozenRoundStage::Loading;
    }
    if (value == QStringLiteral("scheduled")) {
        return FrozenRoundStage::Scheduled;
    }
    if (value == QStringLiteral("playing")) {
        return FrozenRoundStage::Playing;
    }
    fail();
}

auto
parseSelection(const QJsonObject& object) -> SelectionSnapshot
{
    requireExactKeys(object,
                     { "sha256",
                       "title",
                       "subtitle",
                       "artist",
                       "keyMode",
                       "randomSequence",
                       "noteOrderP1",
                       "noteOrderP2",
                       "dpMode",
                       "laneSeed",
                       "randomizationVersion" },
                     { "md5" });
    SelectionSnapshot result;
    result.sha256 = requiredString(object, "sha256");
    if (!isLowerHex(result.sha256, Sha256Characters)) {
        fail();
    }
    if (object.contains(QStringLiteral("md5"))) {
        const auto md5 = requiredString(object, "md5");
        if (!isLowerHex(md5, Md5Characters)) {
            fail();
        }
        result.md5 = md5;
    }
    result.title = requiredString(object, "title");
    result.subtitle = requiredString(object, "subtitle");
    result.artist = requiredString(object, "artist");
    if (!validPossiblyEmptyCodePointString(result.title,
                                           MaxSelectionMetadataCodePoints) ||
        !validPossiblyEmptyCodePointString(result.subtitle,
                                           MaxSelectionMetadataCodePoints) ||
        !validPossiblyEmptyCodePointString(result.artist,
                                           MaxSelectionMetadataCodePoints)) {
        fail();
    }
    const auto keyMode = requiredSafeInteger(object, "keyMode", true);
    if (keyMode != 5 && keyMode != 7 && keyMode != 10 && keyMode != 14) {
        fail();
    }
    result.keyMode = static_cast<int>(keyMode);
    const auto sequence = requiredArray(object, "randomSequence");
    if (sequence.size() > MaxRandomSequenceEntries) {
        fail();
    }
    for (const auto& value : sequence) {
        result.randomSequence.push_back(safeInteger(value, true));
    }
    result.noteOrderP1 = parseNoteOrder(requiredString(object, "noteOrderP1"));
    result.noteOrderP2 = parseNoteOrder(requiredString(object, "noteOrderP2"));
    result.dpMode = parseDpMode(requiredString(object, "dpMode"));
    result.laneSeed = requiredString(object, "laneSeed");
    if (!isLowerHex(result.laneSeed, LaneSeedCharacters) ||
        requiredSafeInteger(object, "randomizationVersion", true) != 1) {
        fail();
    }
    result.randomizationVersion = 1;
    return result;
}

auto
encodeSelection(const SelectionSnapshot& selection) -> QJsonObject
{
    if (!isLowerHex(selection.sha256, Sha256Characters) ||
        (selection.md5 && !isLowerHex(*selection.md5, Md5Characters)) ||
        !validPossiblyEmptyCodePointString(selection.title,
                                           MaxSelectionMetadataCodePoints) ||
        !validPossiblyEmptyCodePointString(selection.subtitle,
                                           MaxSelectionMetadataCodePoints) ||
        !validPossiblyEmptyCodePointString(selection.artist,
                                           MaxSelectionMetadataCodePoints) ||
        (selection.keyMode != 5 && selection.keyMode != 7 &&
         selection.keyMode != 10 && selection.keyMode != 14) ||
        selection.randomSequence.size() > MaxRandomSequenceEntries ||
        !isLowerHex(selection.laneSeed, LaneSeedCharacters) ||
        selection.randomizationVersion != 1) {
        fail();
    }
    QJsonArray sequence;
    for (const auto value : selection.randomSequence) {
        validatePositiveGeneration(value);
        sequence.append(value);
    }
    QJsonObject object{
        { QStringLiteral("sha256"), selection.sha256 },
        { QStringLiteral("title"), selection.title },
        { QStringLiteral("subtitle"), selection.subtitle },
        { QStringLiteral("artist"), selection.artist },
        { QStringLiteral("keyMode"), selection.keyMode },
        { QStringLiteral("randomSequence"), sequence },
        { QStringLiteral("noteOrderP1"),
          noteOrderString(selection.noteOrderP1) },
        { QStringLiteral("noteOrderP2"),
          noteOrderString(selection.noteOrderP2) },
        { QStringLiteral("dpMode"), dpModeString(selection.dpMode) },
        { QStringLiteral("laneSeed"), selection.laneSeed },
        { QStringLiteral("randomizationVersion"), 1 },
    };
    if (selection.md5) {
        object.insert(QStringLiteral("md5"), *selection.md5);
    }
    return object;
}

auto
parsePublicIdentity(const QJsonObject& object) -> PublicIdentity
{
    requireExactKeys(object, { "userId", "displayName", "avatarUrl" });
    PublicIdentity result;
    result.userId = requiredString(object, "userId");
    result.displayName = requiredString(object, "displayName");
    if (!isSafeIdentifier(result.userId, MaxOpaqueIdCharacters) ||
        !validCodePointString(result.displayName, MaxDisplayNameCodePoints)) {
        fail();
    }
    const auto avatar = object.value(QStringLiteral("avatarUrl"));
    if (avatar.isNull()) {
        result.avatarUrl = std::nullopt;
    } else if (avatar.isString()) {
        const auto value = avatar.toString();
        const QUrl url(value);
        if (value.isEmpty() || value.size() > MaxAvatarUrlCharacters ||
            !url.isValid() || url.isRelative() || url.scheme().isEmpty()) {
            fail();
        }
        result.avatarUrl = value;
    } else {
        fail();
    }
    return result;
}

auto
parseMemberStatus(QStringView value) -> MemberStatus
{
    if (value == QStringLiteral("connected")) {
        return MemberStatus::Connected;
    }
    if (value == QStringLiteral("reserved")) {
        return MemberStatus::Reserved;
    }
    fail();
}

auto
parseMember(const QJsonObject& object) -> Member
{
    const auto legacy =
      hasExactKeys(object, { "memberId", "identity", "status", "lobbyWins" });
    const auto current = hasExactKeys(object,
                                      { "memberId",
                                        "identity",
                                        "status",
                                        "lobbyWins",
                                        "ready",
                                        "inventoryState",
                                        "inventoryRevision",
                                        "availabilityAppliedRevision",
                                        "roundState" });
    if (!legacy && !current) {
        fail();
    }
    Member result;
    result.memberId = requiredString(object, "memberId");
    validateOpaqueId(result.memberId);
    result.identity = parsePublicIdentity(requiredObject(object, "identity"));
    result.status = parseMemberStatus(requiredString(object, "status"));
    result.lobbyWins = requiredSafeInteger(object, "lobbyWins");
    if (result.lobbyWins > MaxUInt32) {
        fail();
    }
    if (current) {
        result.ready = requiredBool(object, "ready");
        result.inventoryState =
          parseInventoryState(requiredString(object, "inventoryState"));
        result.inventoryRevision =
          requiredSafeInteger(object, "inventoryRevision");
        result.availabilityAppliedRevision =
          requiredSafeInteger(object, "availabilityAppliedRevision");
        result.roundState =
          parseMemberRoundState(requiredString(object, "roundState"));
    }
    return result;
}

auto
parseChatMessage(const QJsonObject& object) -> ChatMessage
{
    requireExactKeys(object,
                     { "messageId",
                       "authorMemberId",
                       "authorDisplayName",
                       "sentAtMs",
                       "text" });
    ChatMessage result;
    result.messageId = requiredString(object, "messageId");
    result.authorMemberId = requiredString(object, "authorMemberId");
    result.authorDisplayName = requiredString(object, "authorDisplayName");
    result.sentAtMs = requiredSafeInteger(object, "sentAtMs");
    result.text = requiredString(object, "text");
    validateOpaqueId(result.messageId);
    validateOpaqueId(result.authorMemberId);
    if (!validCodePointString(result.authorDisplayName,
                              MaxDisplayNameCodePoints) ||
        !validCodePointString(result.text, MaxChatCodePoints)) {
        fail();
    }
    return result;
}

auto
parseRoomSummary(const QJsonObject& object) -> RoomSummary
{
    requireExactKeys(object,
                     { "roomId",
                       "name",
                       "phase",
                       "hasPassword",
                       "connectedCount",
                       "reservedCount",
                       "maxCount" });
    RoomSummary result;
    result.roomId = requiredString(object, "roomId");
    result.name = ecmaTrim(requiredString(object, "name"));
    if (!isSafeIdentifier(result.roomId, MaxOpaqueIdCharacters) ||
        !validCodePointString(result.name, MaxRoomNameCodePoints)) {
        fail();
    }
    result.phase = parseRoomPhase(requiredString(object, "phase"));
    result.hasPassword = requiredBool(object, "hasPassword");
    const auto connected = requiredSafeInteger(object, "connectedCount");
    const auto reserved = requiredSafeInteger(object, "reservedCount");
    const auto maximum = requiredSafeInteger(object, "maxCount");
    if (connected > RoomCapacity || reserved > RoomCapacity ||
        maximum != RoomCapacity || connected + reserved > maximum) {
        fail();
    }
    result.connectedCount = static_cast<int>(connected);
    result.reservedCount = static_cast<int>(reserved);
    result.maxCount = static_cast<int>(maximum);
    return result;
}

auto
parseFrozenParticipant(const QJsonObject& object) -> FrozenParticipant
{
    const auto legacy =
      hasExactKeys(object, { "memberId", "inventoryRevision" });
    const auto competition =
      hasExactKeys(object, { "memberId", "inventoryRevision", "identity" });
    if (!legacy && !competition) {
        fail();
    }
    auto memberId = requiredString(object, "memberId");
    validateOpaqueId(memberId);
    FrozenParticipant result{
        .memberId = std::move(memberId),
        .inventoryRevision =
          requiredSafeInteger(object, "inventoryRevision", true),
    };
    if (competition) {
        result.identity =
          parsePublicIdentity(requiredObject(object, "identity"));
    }
    return result;
}

auto
parseFrozenParticipants(const QJsonArray& array) -> QVector<FrozenParticipant>
{
    if (array.isEmpty() || array.size() > RoomCapacity) {
        fail();
    }
    QVector<FrozenParticipant> result;
    QSet<QString> ids;
    for (const auto& value : array) {
        if (!value.isObject()) {
            fail();
        }
        auto participant = parseFrozenParticipant(value.toObject());
        if (ids.contains(participant.memberId)) {
            fail();
        }
        ids.insert(participant.memberId);
        result.push_back(std::move(participant));
    }
    return result;
}

auto
parseFrozenRound(const QJsonObject& object) -> FrozenRound
{
    const auto baseKeys = keySet({ "roundId",
                                   "launchAttemptId",
                                   "selectionRevision",
                                   "availabilityRevision",
                                   "selection",
                                   "participants",
                                   "stage" });
    const auto keysWithDeadline = keySet({ "roundId",
                                           "launchAttemptId",
                                           "selectionRevision",
                                           "availabilityRevision",
                                           "selection",
                                           "participants",
                                           "stage",
                                           "playDeadlineAtServerMs" });
    QSet<QString> actualKeys;
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        actualKeys.insert(it.key());
    }
    if (actualKeys != baseKeys && actualKeys != keysWithDeadline) {
        fail();
    }
    FrozenRound result;
    result.roundId = requiredString(object, "roundId");
    result.launchAttemptId = requiredString(object, "launchAttemptId");
    validateOpaqueId(result.roundId);
    validateOpaqueId(result.launchAttemptId);
    result.selectionRevision =
      requiredSafeInteger(object, "selectionRevision", true);
    result.availabilityRevision =
      requiredSafeInteger(object, "availabilityRevision", true);
    result.selection = parseSelection(requiredObject(object, "selection"));
    result.participants =
      parseFrozenParticipants(requiredArray(object, "participants"));
    result.stage = parseFrozenRoundStage(requiredString(object, "stage"));
    const auto identityCount =
      std::count_if(result.participants.cbegin(),
                    result.participants.cend(),
                    [](const FrozenParticipant& participant) {
                        return participant.identity.has_value();
                    });
    if (identityCount != 0 && identityCount != result.participants.size()) {
        fail();
    }
    const auto competition = identityCount == result.participants.size();
    const auto requiresDeadline = result.stage == FrozenRoundStage::Scheduled ||
                                  result.stage == FrozenRoundStage::Playing;
    const auto hasDeadline =
      object.contains(QStringLiteral("playDeadlineAtServerMs"));
    if ((competition && requiresDeadline) != hasDeadline ||
        (!competition && hasDeadline)) {
        fail();
    }
    if (hasDeadline) {
        result.playDeadlineAtServerMs =
          requiredSafeInteger(object, "playDeadlineAtServerMs");
    }
    return result;
}

auto
nullableCompetitionRank(const QJsonObject& object, const char* key)
  -> std::optional<int>
{
    const auto value = object.value(QString::fromLatin1(key));
    if (value.isNull()) {
        return std::nullopt;
    }
    const auto rank = safeInteger(value, true);
    if (rank > RoomCapacity) {
        fail();
    }
    return static_cast<int>(rank);
}

auto
parseLiveStandingEntry(const QJsonObject& object) -> LiveStandingEntry
{
    auto memberId = requiredString(object, "memberId");
    validateOpaqueId(memberId);
    LiveStandingEntry entry{
        .memberId = std::move(memberId),
        .connectionStatus =
          parseMemberStatus(requiredString(object, "connectionStatus")),
    };
    const auto competitionState = requiredString(object, "competitionState");
    if (competitionState == QStringLiteral("loading") ||
        competitionState == QStringLiteral("playing")) {
        requireExactKeys(object,
                         { "memberId",
                           "connectionStatus",
                           "competitionState",
                           "rank",
                           "telemetry" });
        LiveActiveStanding active{
            .competitionState = competitionState == QStringLiteral("loading")
                                  ? ActiveCompetitionState::Loading
                                  : ActiveCompetitionState::Playing,
            .rank = nullableCompetitionRank(object, "rank"),
        };
        const auto telemetry = object.value(QStringLiteral("telemetry"));
        if (telemetry.isObject()) {
            active.telemetry = parseTelemetry(telemetry.toObject());
        } else if (!telemetry.isNull()) {
            fail();
        }
        if (active.rank.has_value() != active.telemetry.has_value()) {
            fail();
        }
        entry.state = std::move(active);
        return entry;
    }
    if (competitionState == QStringLiteral("finished")) {
        requireExactKeys(object,
                         { "memberId",
                           "connectionStatus",
                           "competitionState",
                           "rank",
                           "result" });
        const auto rank = nullableCompetitionRank(object, "rank");
        if (!rank) {
            fail();
        }
        entry.state = LiveFinishedStanding{
            .rank = *rank,
            .result = parseFinalResult(requiredObject(object, "result")),
        };
        return entry;
    }
    if (competitionState == QStringLiteral("dnf")) {
        requireExactKeys(object,
                         { "memberId",
                           "connectionStatus",
                           "competitionState",
                           "rank",
                           "dnfReason" });
        if (!object.value(QStringLiteral("rank")).isNull()) {
            fail();
        }
        entry.state = LiveDnfStanding{
            .reason = parseDnfReason(requiredString(object, "dnfReason")),
        };
        return entry;
    }
    fail();
}

auto
parseLiveStandings(const QJsonObject& object) -> LiveStandingsSnapshot
{
    requireExactKeys(object,
                     { "roomId",
                       "roomGeneration",
                       "roundId",
                       "launchAttemptId",
                       "standingsRevision",
                       "entries" });
    LiveStandingsSnapshot result{
        .roomId = requiredString(object, "roomId"),
        .roomGeneration = requiredSafeInteger(object, "roomGeneration", true),
        .roundId = requiredString(object, "roundId"),
        .launchAttemptId = requiredString(object, "launchAttemptId"),
        .standingsRevision =
          requiredSafeInteger(object, "standingsRevision", true),
    };
    validateOpaqueId(result.roomId);
    validateOpaqueId(result.roundId);
    validateOpaqueId(result.launchAttemptId);
    const auto entries = requiredArray(object, "entries");
    if (entries.isEmpty() || entries.size() > RoomCapacity) {
        fail();
    }
    QSet<QString> ids;
    for (const auto& value : entries) {
        if (!value.isObject()) {
            fail();
        }
        auto entry = parseLiveStandingEntry(value.toObject());
        if (ids.contains(entry.memberId)) {
            fail();
        }
        ids.insert(entry.memberId);
        result.entries.push_back(std::move(entry));
    }
    return result;
}

auto
parseFinalStandingEntry(const QJsonObject& object) -> FinalStandingEntry
{
    auto memberId = requiredString(object, "memberId");
    validateOpaqueId(memberId);
    FinalStandingEntry entry{
        .memberId = std::move(memberId),
        .identity = parsePublicIdentity(requiredObject(object, "identity")),
    };
    const auto wins = object.value(QStringLiteral("lobbyWinsAfter"));
    if (wins.isNull()) {
        entry.lobbyWinsAfter = std::nullopt;
    } else {
        const auto value = safeInteger(wins, false);
        if (value > MaxUInt32) {
            fail();
        }
        entry.lobbyWinsAfter = value;
    }
    const auto competitionState = requiredString(object, "competitionState");
    if (competitionState == QStringLiteral("finished")) {
        requireExactKeys(object,
                         { "memberId",
                           "identity",
                           "lobbyWinsAfter",
                           "competitionState",
                           "rank",
                           "result" });
        const auto rank = nullableCompetitionRank(object, "rank");
        if (!rank) {
            fail();
        }
        entry.state = FinalFinishedStanding{
            .rank = *rank,
            .result = parseFinalResult(requiredObject(object, "result")),
        };
        return entry;
    }
    if (competitionState == QStringLiteral("dnf")) {
        requireExactKeys(object,
                         { "memberId",
                           "identity",
                           "lobbyWinsAfter",
                           "competitionState",
                           "rank",
                           "dnfReason" });
        if (!object.value(QStringLiteral("rank")).isNull()) {
            fail();
        }
        entry.state = FinalDnfStanding{
            .reason = parseDnfReason(requiredString(object, "dnfReason")),
        };
        return entry;
    }
    fail();
}

auto
parseRoundResultSnapshot(const QJsonObject& object) -> RoundResultSnapshot
{
    if (QJsonDocument(object).toJson(QJsonDocument::Compact).size() >
        MaxResultSnapshotBytes) {
        fail(ProtocolFailureCode::FrameTooLarge);
    }
    requireExactKeys(object,
                     { "resultRevision",
                       "roundId",
                       "selectionRevision",
                       "finalizedAtServerMs",
                       "participantCount",
                       "selection",
                       "winnerMemberIds",
                       "entries" });
    RoundResultSnapshot result{
        .resultRevision = requiredSafeInteger(object, "resultRevision", true),
        .roundId = requiredString(object, "roundId"),
        .selectionRevision =
          requiredSafeInteger(object, "selectionRevision", true),
        .finalizedAtServerMs =
          requiredSafeInteger(object, "finalizedAtServerMs"),
        .selection = parseSelection(requiredObject(object, "selection")),
    };
    validateOpaqueId(result.roundId);
    const auto participantCount =
      requiredSafeInteger(object, "participantCount", true);
    if (participantCount > RoomCapacity) {
        fail();
    }
    result.participantCount = static_cast<int>(participantCount);

    const auto winners = requiredArray(object, "winnerMemberIds");
    if (winners.size() > RoomCapacity) {
        fail();
    }
    QSet<QString> winnerIds;
    for (const auto& value : winners) {
        if (!value.isString()) {
            fail();
        }
        auto id = value.toString();
        validateOpaqueId(id);
        if (winnerIds.contains(id)) {
            fail();
        }
        winnerIds.insert(id);
        result.winnerMemberIds.push_back(std::move(id));
    }

    const auto entries = requiredArray(object, "entries");
    if (entries.isEmpty() || entries.size() > RoomCapacity ||
        entries.size() != result.participantCount) {
        fail();
    }
    QSet<QString> entryIds;
    QVector<QString> expectedWinners;
    for (const auto& value : entries) {
        if (!value.isObject()) {
            fail();
        }
        auto entry = parseFinalStandingEntry(value.toObject());
        if (entryIds.contains(entry.memberId)) {
            fail();
        }
        entryIds.insert(entry.memberId);
        if (const auto* finished =
              std::get_if<FinalFinishedStanding>(&entry.state);
            finished != nullptr && finished->rank == 1) {
            expectedWinners.push_back(entry.memberId);
        }
        result.entries.push_back(std::move(entry));
    }
    if (result.winnerMemberIds != expectedWinners) {
        fail();
    }
    return result;
}

auto
parseRoomSnapshot(const QJsonObject& object) -> RoomSnapshot
{
    const auto legacy = hasExactKeys(object,
                                     { "roomId",
                                       "roomGeneration",
                                       "name",
                                       "phase",
                                       "hasPassword",
                                       "maxCount",
                                       "ownerMemberId",
                                       "self",
                                       "members",
                                       "chat" });
    const auto currentWithoutRound = hasExactKeys(object,
                                                  { "roomId",
                                                    "roomGeneration",
                                                    "name",
                                                    "phase",
                                                    "hasPassword",
                                                    "maxCount",
                                                    "ownerMemberId",
                                                    "self",
                                                    "members",
                                                    "chat",
                                                    "selection",
                                                    "selectionRevision",
                                                    "availabilityRevision" });
    const auto currentWithRound = hasExactKeys(object,
                                               { "roomId",
                                                 "roomGeneration",
                                                 "name",
                                                 "phase",
                                                 "hasPassword",
                                                 "maxCount",
                                                 "ownerMemberId",
                                                 "self",
                                                 "members",
                                                 "chat",
                                                 "selection",
                                                 "selectionRevision",
                                                 "availabilityRevision",
                                                 "round" });
    const auto competitionWithoutRound = hasExactKeys(object,
                                                      { "roomId",
                                                        "roomGeneration",
                                                        "name",
                                                        "phase",
                                                        "hasPassword",
                                                        "maxCount",
                                                        "ownerMemberId",
                                                        "self",
                                                        "members",
                                                        "chat",
                                                        "selection",
                                                        "selectionRevision",
                                                        "availabilityRevision",
                                                        "liveStandings",
                                                        "lastRoundResult" });
    const auto competitionWithRound = hasExactKeys(object,
                                                   { "roomId",
                                                     "roomGeneration",
                                                     "name",
                                                     "phase",
                                                     "hasPassword",
                                                     "maxCount",
                                                     "ownerMemberId",
                                                     "self",
                                                     "members",
                                                     "chat",
                                                     "selection",
                                                     "selectionRevision",
                                                     "availabilityRevision",
                                                     "round",
                                                     "liveStandings",
                                                     "lastRoundResult" });
    const auto current = currentWithoutRound || currentWithRound;
    const auto competition = competitionWithoutRound || competitionWithRound;
    if (!legacy && !current && !competition) {
        fail();
    }
    RoomSnapshot result;
    result.competitionShape = competition;
    result.roomId = requiredString(object, "roomId");
    validateOpaqueId(result.roomId);
    result.roomGeneration = requiredSafeInteger(object, "roomGeneration", true);
    result.name = ecmaTrim(requiredString(object, "name"));
    if (!validCodePointString(result.name, MaxRoomNameCodePoints)) {
        fail();
    }
    result.phase = parseRoomPhase(requiredString(object, "phase"));
    if (legacy && result.phase != RoomPhase::Selecting) {
        fail();
    }
    result.hasPassword = requiredBool(object, "hasPassword");
    const auto maximum = requiredSafeInteger(object, "maxCount");
    if (maximum != RoomCapacity) {
        fail();
    }
    result.maxCount = static_cast<int>(maximum);
    result.ownerMemberId = nullableString(object, "ownerMemberId");
    if (result.ownerMemberId) {
        validateOpaqueId(*result.ownerMemberId);
    }

    const auto self = requiredObject(object, "self");
    requireExactKeys(self,
                     { "memberId", "connectionGeneration", "resumeToken" });
    result.self.memberId = requiredString(self, "memberId");
    result.self.connectionGeneration =
      requiredSafeInteger(self, "connectionGeneration", true);
    result.self.resumeToken = requiredString(self, "resumeToken");
    validateOpaqueId(result.self.memberId);
    validateOpaqueId(result.self.resumeToken);

    const auto members = requiredArray(object, "members");
    if (members.size() > RoomCapacity) {
        fail();
    }
    QSet<QString> memberIds;
    for (const auto& value : members) {
        if (!value.isObject()) {
            fail();
        }
        const auto memberObject = value.toObject();
        const auto memberIsCurrent =
          memberObject.contains(QStringLiteral("ready"));
        if (memberIsCurrent != (current || competition)) {
            fail();
        }
        auto member = parseMember(memberObject);
        if (memberIds.contains(member.memberId)) {
            fail();
        }
        memberIds.insert(member.memberId);
        result.members.push_back(std::move(member));
    }

    const auto chat = requiredArray(object, "chat");
    if (chat.size() > MaxWireChatBacklog) {
        fail();
    }
    QSet<QString> messageIds;
    for (const auto& value : chat) {
        if (!value.isObject()) {
            fail();
        }
        auto message = parseChatMessage(value.toObject());
        if (messageIds.contains(message.messageId)) {
            fail();
        }
        messageIds.insert(message.messageId);
        result.chat.push_back(std::move(message));
    }
    if (current || competition) {
        const auto selection = object.value(QStringLiteral("selection"));
        if (selection.isNull()) {
            result.selection = std::nullopt;
        } else if (selection.isObject()) {
            result.selection = parseSelection(selection.toObject());
        } else {
            fail();
        }
        result.selectionRevision =
          requiredSafeInteger(object, "selectionRevision");
        result.availabilityRevision =
          requiredSafeInteger(object, "availabilityRevision");
        if (currentWithRound || competitionWithRound) {
            result.round = parseFrozenRound(requiredObject(object, "round"));
        }
    }
    if (competition) {
        if (result.round &&
            std::any_of(result.round->participants.cbegin(),
                        result.round->participants.cend(),
                        [](const FrozenParticipant& participant) {
                            return !participant.identity.has_value();
                        })) {
            fail();
        }
        const auto live = object.value(QStringLiteral("liveStandings"));
        if (live.isObject()) {
            result.liveStandings = parseLiveStandings(live.toObject());
        } else if (!live.isNull()) {
            fail();
        }
        const auto last = object.value(QStringLiteral("lastRoundResult"));
        if (last.isObject()) {
            result.lastRoundResult = parseRoundResultSnapshot(last.toObject());
        } else if (!last.isNull()) {
            fail();
        }
        const auto hasRound = result.round.has_value();
        const auto hasLive = result.liveStandings.has_value();
        if ((result.phase == RoomPhase::Selecting && (hasRound || hasLive)) ||
            (result.phase == RoomPhase::Loading && (!hasRound || hasLive)) ||
            (result.phase == RoomPhase::Playing && (!hasRound || !hasLive))) {
            fail();
        }
    }
    return result;
}

auto
parseMemberLeftReason(QStringView value) -> MemberLeftReason
{
    if (value == QStringLiteral("left")) {
        return MemberLeftReason::Left;
    }
    if (value == QStringLiteral("kicked")) {
        return MemberLeftReason::Kicked;
    }
    if (value == QStringLiteral("grace_expired")) {
        return MemberLeftReason::GraceExpired;
    }
    fail();
}

auto
parseCommandErrorCode(QStringView value) -> CommandErrorCode
{
    static const std::pair<QStringView, CommandErrorCode> entries[]{
        { u"auth_required", CommandErrorCode::AuthRequired },
        { u"already_in_room", CommandErrorCode::AlreadyInRoom },
        { u"not_in_room", CommandErrorCode::NotInRoom },
        { u"room_not_found", CommandErrorCode::RoomNotFound },
        { u"room_password_invalid", CommandErrorCode::RoomPasswordInvalid },
        { u"room_full", CommandErrorCode::RoomFull },
        { u"room_banned", CommandErrorCode::RoomBanned },
        { u"room_duplicate_identity", CommandErrorCode::RoomDuplicateIdentity },
        { u"room_generation_stale", CommandErrorCode::RoomGenerationStale },
        { u"connection_generation_stale",
          CommandErrorCode::ConnectionGenerationStale },
        { u"permission_denied", CommandErrorCode::PermissionDenied },
        { u"target_not_found", CommandErrorCode::TargetNotFound },
        { u"cannot_kick_self", CommandErrorCode::CannotKickSelf },
        { u"chat_empty", CommandErrorCode::ChatEmpty },
        { u"chat_too_long", CommandErrorCode::ChatTooLong },
        { u"rate_limited", CommandErrorCode::RateLimited },
        { u"rounds_capability_required",
          CommandErrorCode::RoundsCapabilityRequired },
        { u"competition_capability_required",
          CommandErrorCode::CompetitionCapabilityRequired },
        { u"inventory_busy", CommandErrorCode::InventoryBusy },
        { u"inventory_invalid", CommandErrorCode::InventoryInvalid },
        { u"inventory_stale", CommandErrorCode::InventoryStale },
        { u"inventory_capacity_exceeded",
          CommandErrorCode::InventoryCapacityExceeded },
        { u"availability_stale", CommandErrorCode::AvailabilityStale },
        { u"selection_not_common", CommandErrorCode::SelectionNotCommon },
        { u"selection_stale", CommandErrorCode::SelectionStale },
        { u"ready_not_allowed", CommandErrorCode::ReadyNotAllowed },
        { u"round_stale", CommandErrorCode::RoundStale },
        { u"launch_stage_stale", CommandErrorCode::LaunchStageStale },
        { u"result_invalid", CommandErrorCode::ResultInvalid },
        { u"round_already_terminal", CommandErrorCode::RoundAlreadyTerminal },
        { u"server_capacity", CommandErrorCode::ServerCapacity },
    };
    for (const auto& [spelling, code] : entries) {
        if (value == spelling) {
            return code;
        }
    }
    fail();
}

auto
parseFatalErrorCode(QStringView value) -> FatalErrorCode
{
    static const std::pair<QStringView, FatalErrorCode> entries[]{
        { u"malformed_message", FatalErrorCode::MalformedMessage },
        { u"frame_too_large", FatalErrorCode::FrameTooLarge },
        { u"unexpected_binary", FatalErrorCode::UnexpectedBinary },
        { u"hello_required", FatalErrorCode::HelloRequired },
        { u"hello_repeated", FatalErrorCode::HelloRepeated },
        { u"protocol_incompatible", FatalErrorCode::ProtocolIncompatible },
        { u"capability_required", FatalErrorCode::CapabilityRequired },
        { u"invalid_ticket", FatalErrorCode::InvalidTicket },
        { u"ticket_replayed", FatalErrorCode::TicketReplayed },
        { u"server_shutting_down", FatalErrorCode::ServerShuttingDown },
        { u"malformed_inventory", FatalErrorCode::MalformedInventory },
    };
    for (const auto& [spelling, code] : entries) {
        if (value == spelling) {
            return code;
        }
    }
    fail();
}

auto
isDisplayMessageKey(QStringView value) -> bool
{
    if (value.size() <= 6 || value.size() > MaxDisplayMessageKeyCharacters ||
        !value.startsWith(QStringLiteral("arena."))) {
        return false;
    }
    return std::all_of(value.begin() + 6, value.end(), [](QChar ch) {
        const auto c = ch.unicode();
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '.';
    });
}

auto
encodeHello(const ClientHello& hello) -> QJsonObject
{
    if (hello.protocolMajor != ProtocolMajor ||
        hello.protocolMinor < LegacyProtocolMinor ||
        hello.protocolMinor > ProtocolMinor) {
        fail(ProtocolFailureCode::ProtocolIncompatible);
    }
    if (!validCodePointString(hello.clientVersion,
                              MaxClientVersionCodePoints)) {
        fail();
    }
    QJsonArray capabilities;
    QSet<QString> seen;
    for (const auto& capability : hello.capabilities) {
        if (!isCapability(capability) || seen.contains(capability)) {
            fail();
        }
        seen.insert(capability);
        capabilities.append(capability);
    }
    if (hello.capabilities.isEmpty() ||
        hello.capabilities.size() > MaxCapabilities) {
        fail();
    }
    if (!hello.capabilities.contains(QString::fromLatin1(RoomsCapability))) {
        fail(ProtocolFailureCode::CapabilityRequired);
    }
    const auto hasRounds =
      hello.capabilities.contains(QString::fromLatin1(RoundsCapability));
    const auto hasCompetition =
      hello.capabilities.contains(QString::fromLatin1(CompetitionCapability));
    if ((hasCompetition && !hasRounds) ||
        (hello.protocolMinor == LegacyProtocolMinor &&
         (hasRounds || hasCompetition)) ||
        (hello.protocolMinor == RoundsProtocolMinor && hasCompetition)) {
        fail();
    }
    if (hello.ticket && (hello.ticket->isEmpty() ||
                         hello.ticket->size() > MaxTicketCharacters)) {
        fail();
    }
    if (hello.ticket) {
        requireValidUnicode(*hello.ticket);
    }
    if (hello.resume && !hello.ticket) {
        fail();
    }
    QJsonObject data{
        { QStringLiteral("protocolMajor"), hello.protocolMajor },
        { QStringLiteral("protocolMinor"), hello.protocolMinor },
        { QStringLiteral("clientVersion"), hello.clientVersion },
        { QStringLiteral("capabilities"), capabilities },
    };
    if (hello.ticket) {
        data.insert(QStringLiteral("ticket"), *hello.ticket);
    }
    if (hello.resume) {
        if (!isSafeIdentifier(hello.resume->roomId, MaxOpaqueIdCharacters) ||
            !isSafeIdentifier(hello.resume->seatToken, MaxOpaqueIdCharacters)) {
            fail();
        }
        data.insert(
          QStringLiteral("resume"),
          QJsonObject{
            { QStringLiteral("roomId"), hello.resume->roomId },
            { QStringLiteral("seatToken"), hello.resume->seatToken } });
    }
    return { { QStringLiteral("type"), QStringLiteral("client_hello") },
             { QStringLiteral("data"), data } };
}

void
validateRequestId(QStringView requestId)
{
    if (!isSafeIdentifier(requestId, MaxRequestIdCharacters)) {
        fail();
    }
}

void
validateOpaqueId(QStringView id)
{
    if (!isSafeIdentifier(id, MaxOpaqueIdCharacters)) {
        fail();
    }
}

void
validatePassword(const std::optional<QString>& password)
{
    if (!password) {
        return;
    }
    requireValidUnicode(*password);
    if (password->isEmpty() || password->toUtf8().size() > MaxPasswordBytes) {
        fail();
    }
}

void
validatePositiveGeneration(qint64 value)
{
    if (value < 1 || value > MaxJsonSafeInteger) {
        fail();
    }
}

auto
requestEnvelope(QString type, QString requestId, QJsonObject data)
  -> QJsonObject
{
    validateRequestId(requestId);
    return { { QStringLiteral("type"), std::move(type) },
             { QStringLiteral("requestId"), std::move(requestId) },
             { QStringLiteral("data"), std::move(data) } };
}

auto
encodeDirectorySubscribe(const DirectorySubscribe&) -> QJsonObject
{
    return { { QStringLiteral("type"), QStringLiteral("directory_subscribe") },
             { QStringLiteral("data"), QJsonObject{} } };
}

auto
encodeRoomCreate(const RoomCreate& command) -> QJsonObject
{
    auto name = ecmaTrim(command.name);
    if (!validCodePointString(name, MaxRoomNameCodePoints)) {
        fail();
    }
    validatePassword(command.password);
    QJsonObject data{ { QStringLiteral("name"), std::move(name) } };
    if (command.password) {
        data.insert(QStringLiteral("password"), *command.password);
    }
    return requestEnvelope(
      QStringLiteral("room_create"), command.requestId, std::move(data));
}

auto
encodeRoomJoin(const RoomJoin& command) -> QJsonObject
{
    validateOpaqueId(command.roomId);
    validatePassword(command.password);
    QJsonObject data{ { QStringLiteral("roomId"), command.roomId } };
    if (command.password) {
        data.insert(QStringLiteral("password"), *command.password);
    }
    return requestEnvelope(
      QStringLiteral("room_join"), command.requestId, std::move(data));
}

auto
generationData(QString roomId,
               qint64 roomGeneration,
               qint64 connectionGeneration) -> QJsonObject
{
    validateOpaqueId(roomId);
    validatePositiveGeneration(roomGeneration);
    validatePositiveGeneration(connectionGeneration);
    return { { QStringLiteral("roomId"), std::move(roomId) },
             { QStringLiteral("roomGeneration"), roomGeneration },
             { QStringLiteral("connectionGeneration"), connectionGeneration } };
}

auto
encodeRoomLeave(const RoomLeave& command) -> QJsonObject
{
    return requestEnvelope(QStringLiteral("room_leave"),
                           command.requestId,
                           generationData(command.roomId,
                                          command.roomGeneration,
                                          command.connectionGeneration));
}

auto
encodeRoomKick(const RoomKick& command) -> QJsonObject
{
    auto data = generationData(
      command.roomId, command.roomGeneration, command.connectionGeneration);
    validateOpaqueId(command.targetMemberId);
    data.insert(QStringLiteral("targetMemberId"), command.targetMemberId);
    return requestEnvelope(
      QStringLiteral("room_kick"), command.requestId, std::move(data));
}

auto
encodeChatSend(const ChatSend& command) -> QJsonObject
{
    auto data = generationData(
      command.roomId, command.roomGeneration, command.connectionGeneration);
    auto text = ecmaTrim(command.text);
    if (!validCodePointString(text, MaxChatCodePoints)) {
        fail();
    }
    data.insert(QStringLiteral("text"), std::move(text));
    return requestEnvelope(
      QStringLiteral("chat_send"), command.requestId, std::move(data));
}

auto
encodeHeartbeatReply(const HeartbeatReply& command) -> QJsonObject
{
    validateOpaqueId(command.nonce);
    return { { QStringLiteral("type"), QStringLiteral("heartbeat_reply") },
             { QStringLiteral("data"),
               QJsonObject{ { QStringLiteral("nonce"), command.nonce } } } };
}

void
validateNonNegativeSafe(qint64 value)
{
    if (value < 0 || value > MaxJsonSafeInteger) {
        fail();
    }
}

void
validateTransferId(QStringView value)
{
    if (!isTransferId(value)) {
        fail();
    }
}

void
insertInventoryDeclaration(QJsonObject& data,
                           qint64 libraryGeneration,
                           qint64 hashCount,
                           qint64 byteCount,
                           qint64 chunkCount,
                           QStringView vectorDigest)
{
    validatePositiveGeneration(libraryGeneration);
    if (hashCount < 0 || hashCount > MaxInventoryHashes || byteCount < 0 ||
        byteCount > MaxInventoryBytes || chunkCount < 0 ||
        chunkCount > MaxInventoryChunks || byteCount != hashCount * 32 ||
        chunkCount !=
          (hashCount + InventoryHashesPerChunk - 1) / InventoryHashesPerChunk ||
        !isLowerHex(vectorDigest, Sha256Characters)) {
        fail();
    }
    data.insert(QStringLiteral("libraryGeneration"), libraryGeneration);
    data.insert(QStringLiteral("hashCount"), hashCount);
    data.insert(QStringLiteral("byteCount"), byteCount);
    data.insert(QStringLiteral("chunkCount"), chunkCount);
    data.insert(QStringLiteral("vectorDigest"), vectorDigest.toString());
}

auto
encodeInventoryUploadBegin(const InventoryUploadBegin& command) -> QJsonObject
{
    auto data = generationData(
      command.roomId, command.roomGeneration, command.connectionGeneration);
    insertInventoryDeclaration(data,
                               command.libraryGeneration,
                               command.hashCount,
                               command.byteCount,
                               command.chunkCount,
                               command.vectorDigest);
    return requestEnvelope(QStringLiteral("inventory_upload_begin"),
                           command.requestId,
                           std::move(data));
}

auto
encodeInventoryUploadCommit(const InventoryUploadCommit& command) -> QJsonObject
{
    auto data = generationData(
      command.roomId, command.roomGeneration, command.connectionGeneration);
    validateTransferId(command.uploadId);
    data.insert(QStringLiteral("uploadId"), command.uploadId);
    insertInventoryDeclaration(data,
                               command.libraryGeneration,
                               command.hashCount,
                               command.byteCount,
                               command.chunkCount,
                               command.vectorDigest);
    return requestEnvelope(QStringLiteral("inventory_upload_commit"),
                           command.requestId,
                           std::move(data));
}

auto
encodeInventoryUploadAbort(const InventoryUploadAbort& command) -> QJsonObject
{
    auto data = generationData(
      command.roomId, command.roomGeneration, command.connectionGeneration);
    validateTransferId(command.uploadId);
    validatePositiveGeneration(command.libraryGeneration);
    data.insert(QStringLiteral("uploadId"), command.uploadId);
    data.insert(QStringLiteral("libraryGeneration"), command.libraryGeneration);
    return requestEnvelope(QStringLiteral("inventory_upload_abort"),
                           command.requestId,
                           std::move(data));
}

auto
encodeAvailabilityApplied(const AvailabilityApplied& command) -> QJsonObject
{
    auto data = generationData(
      command.roomId, command.roomGeneration, command.connectionGeneration);
    validatePositiveGeneration(command.availabilityRevision);
    data.insert(QStringLiteral("availabilityRevision"),
                command.availabilityRevision);
    return requestEnvelope(QStringLiteral("availability_applied"),
                           command.requestId,
                           std::move(data));
}

auto
encodeAvailabilityResync(const AvailabilityResync& command) -> QJsonObject
{
    auto data = generationData(
      command.roomId, command.roomGeneration, command.connectionGeneration);
    validateNonNegativeSafe(command.currentRevision);
    data.insert(QStringLiteral("currentRevision"), command.currentRevision);
    return requestEnvelope(QStringLiteral("availability_resync"),
                           command.requestId,
                           std::move(data));
}

auto
encodeSelectionSet(const SelectionSet& command) -> QJsonObject
{
    auto data = generationData(
      command.roomId, command.roomGeneration, command.connectionGeneration);
    validatePositiveGeneration(command.availabilityRevision);
    validatePositiveGeneration(command.inventoryRevision);
    data.insert(QStringLiteral("availabilityRevision"),
                command.availabilityRevision);
    data.insert(QStringLiteral("inventoryRevision"), command.inventoryRevision);
    data.insert(QStringLiteral("selection"),
                encodeSelection(command.selection));
    return requestEnvelope(
      QStringLiteral("selection_set"), command.requestId, std::move(data));
}

auto
encodeReadySet(const ReadySet& command) -> QJsonObject
{
    auto data = generationData(
      command.roomId, command.roomGeneration, command.connectionGeneration);
    validatePositiveGeneration(command.selectionRevision);
    validatePositiveGeneration(command.availabilityRevision);
    validatePositiveGeneration(command.inventoryRevision);
    data.insert(QStringLiteral("ready"), command.ready);
    data.insert(QStringLiteral("selectionRevision"), command.selectionRevision);
    data.insert(QStringLiteral("availabilityRevision"),
                command.availabilityRevision);
    data.insert(QStringLiteral("inventoryRevision"), command.inventoryRevision);
    return requestEnvelope(
      QStringLiteral("ready_set"), command.requestId, std::move(data));
}

auto
probeFailureString(RoundProbeFailureReason value) -> QString
{
    switch (value) {
        case RoundProbeFailureReason::MissingFile:
            return QStringLiteral("missing_file");
        case RoundProbeFailureReason::HashMismatch:
            return QStringLiteral("hash_mismatch");
        case RoundProbeFailureReason::ReadFailed:
            return QStringLiteral("read_failed");
        case RoundProbeFailureReason::Cancelled:
            return QStringLiteral("cancelled");
    }
    fail();
}

auto
loadFailureString(RoundLoadFailureReason value) -> QString
{
    switch (value) {
        case RoundLoadFailureReason::MissingFile:
            return QStringLiteral("missing_file");
        case RoundLoadFailureReason::HashMismatch:
            return QStringLiteral("hash_mismatch");
        case RoundLoadFailureReason::ParseFailed:
            return QStringLiteral("parse_failed");
        case RoundLoadFailureReason::UnsupportedConfig:
            return QStringLiteral("unsupported_config");
        case RoundLoadFailureReason::ResourceFailed:
            return QStringLiteral("resource_failed");
        case RoundLoadFailureReason::Cancelled:
            return QStringLiteral("cancelled");
    }
    fail();
}

auto
roundResultData(QString roomId,
                qint64 roomGeneration,
                qint64 connectionGeneration,
                QString roundId,
                QString launchAttemptId,
                qint64 selectionRevision,
                qint64 availabilityRevision,
                qint64 inventoryRevision) -> QJsonObject
{
    auto data =
      generationData(std::move(roomId), roomGeneration, connectionGeneration);
    validateOpaqueId(roundId);
    validateOpaqueId(launchAttemptId);
    validatePositiveGeneration(selectionRevision);
    validatePositiveGeneration(availabilityRevision);
    validatePositiveGeneration(inventoryRevision);
    data.insert(QStringLiteral("roundId"), std::move(roundId));
    data.insert(QStringLiteral("launchAttemptId"), std::move(launchAttemptId));
    data.insert(QStringLiteral("selectionRevision"), selectionRevision);
    data.insert(QStringLiteral("availabilityRevision"), availabilityRevision);
    data.insert(QStringLiteral("inventoryRevision"), inventoryRevision);
    return data;
}

auto
encodeRoundProbeResult(const RoundProbeResult& command) -> QJsonObject
{
    auto data = roundResultData(command.roomId,
                                command.roomGeneration,
                                command.connectionGeneration,
                                command.roundId,
                                command.launchAttemptId,
                                command.selectionRevision,
                                command.availabilityRevision,
                                command.inventoryRevision);
    validateOpaqueId(command.nonce);
    data.insert(QStringLiteral("nonce"), command.nonce);
    data.insert(QStringLiteral("ok"), command.ok);
    if (command.ok) {
        if (!command.sha256 || command.failureReason ||
            !isLowerHex(*command.sha256, Sha256Characters)) {
            fail();
        }
        data.insert(QStringLiteral("sha256"), *command.sha256);
    } else {
        if (command.sha256 || !command.failureReason) {
            fail();
        }
        data.insert(QStringLiteral("reason"),
                    probeFailureString(*command.failureReason));
    }
    return requestEnvelope(
      QStringLiteral("round_probe_result"), command.requestId, std::move(data));
}

auto
encodeRoundLoadResult(const RoundLoadResult& command) -> QJsonObject
{
    auto data = roundResultData(command.roomId,
                                command.roomGeneration,
                                command.connectionGeneration,
                                command.roundId,
                                command.launchAttemptId,
                                command.selectionRevision,
                                command.availabilityRevision,
                                command.inventoryRevision);
    data.insert(QStringLiteral("ok"), command.ok);
    if (command.ok) {
        if (command.failureReason) {
            fail();
        }
        if (command.chartLengthMs) {
            if (*command.chartLengthMs < 0 ||
                *command.chartLengthMs > MaxChartLengthMs) {
                fail();
            }
            data.insert(QStringLiteral("chartLengthMs"),
                        *command.chartLengthMs);
        }
    } else {
        if (!command.failureReason || command.chartLengthMs) {
            fail();
        }
        data.insert(QStringLiteral("reason"),
                    loadFailureString(*command.failureReason));
    }
    return requestEnvelope(
      QStringLiteral("round_load_result"), command.requestId, std::move(data));
}

auto
competitionData(QString roomId,
                qint64 roomGeneration,
                qint64 connectionGeneration,
                QString roundId,
                QString launchAttemptId) -> QJsonObject
{
    auto data =
      generationData(std::move(roomId), roomGeneration, connectionGeneration);
    validateOpaqueId(roundId);
    validateOpaqueId(launchAttemptId);
    data.insert(QStringLiteral("roundId"), std::move(roundId));
    data.insert(QStringLiteral("launchAttemptId"), std::move(launchAttemptId));
    return data;
}

auto
encodeRoundTelemetry(const RoundTelemetry& command) -> QJsonObject
{
    auto data = competitionData(command.roomId,
                                command.roomGeneration,
                                command.connectionGeneration,
                                command.roundId,
                                command.launchAttemptId);
    data.insert(QStringLiteral("telemetry"),
                encodeTelemetry(command.telemetry));
    return { { QStringLiteral("type"), QStringLiteral("round_telemetry") },
             { QStringLiteral("data"), std::move(data) } };
}

auto
encodeRoundResultSubmit(const RoundResultSubmit& command) -> QJsonObject
{
    auto data = competitionData(command.roomId,
                                command.roomGeneration,
                                command.connectionGeneration,
                                command.roundId,
                                command.launchAttemptId);
    data.insert(QStringLiteral("result"), encodeFinalResult(command.result));
    return requestEnvelope(QStringLiteral("round_result_submit"),
                           command.requestId,
                           std::move(data));
}

auto
encodeRoundAbandon(const RoundAbandon& command) -> QJsonObject
{
    if (command.reason != DnfReason::Aborted &&
        command.reason != DnfReason::ResultUnavailable) {
        fail();
    }
    auto data = competitionData(command.roomId,
                                command.roomGeneration,
                                command.connectionGeneration,
                                command.roundId,
                                command.launchAttemptId);
    data.insert(QStringLiteral("reason"), dnfReasonString(command.reason));
    return requestEnvelope(
      QStringLiteral("round_abandon"), command.requestId, std::move(data));
}

auto
parseServerHello(const QJsonObject& data) -> ServerHello
{
    requireExactKeys(
      data,
      { "protocolMajor", "protocolMinor", "capabilities", "resume" },
      { "identity" });
    const auto major = requiredSafeInteger(data, "protocolMajor");
    const auto minor = requiredSafeInteger(data, "protocolMinor");
    if (major != ProtocolMajor || minor < LegacyProtocolMinor ||
        minor > ProtocolMinor) {
        fail(ProtocolFailureCode::ProtocolIncompatible);
    }
    ServerHello result;
    result.protocolMajor = static_cast<int>(major);
    result.protocolMinor = static_cast<int>(minor);
    result.capabilities = parseCapabilities(
      requiredArray(data, "capabilities"), true, result.protocolMinor);
    if (data.contains(QStringLiteral("identity"))) {
        result.identity = parsePublicIdentity(requiredObject(data, "identity"));
    }
    const auto resume = requiredObject(data, "resume");
    const auto status = requiredString(resume, "status");
    if (status == QStringLiteral("not_requested")) {
        requireExactKeys(resume, { "status" });
        result.resume = ResumeNotRequested{};
    } else if (status == QStringLiteral("succeeded")) {
        requireExactKeys(resume, { "status", "room" });
        result.resume =
          ResumeSucceeded{ parseRoomSnapshot(requiredObject(resume, "room")) };
    } else if (status == QStringLiteral("failed")) {
        requireExactKeys(resume, { "status", "code", "displayMessageKey" });
        const auto code = requiredString(resume, "code");
        const auto displayMessageKey =
          requiredString(resume, "displayMessageKey");
        if (code == QStringLiteral("room_resume_failed") &&
            displayMessageKey == QStringLiteral("arena.error.resumeFailed")) {
            result.resume = ResumeFailed{
                .code = ResumeFailureCode::RoomResumeFailed,
            };
        } else if (code == QStringLiteral("competition_capability_required") &&
                   displayMessageKey ==
                     QStringLiteral(
                       "arena.error.competitionCapabilityRequired")) {
            result.resume = ResumeFailed{
                .code = ResumeFailureCode::CompetitionCapabilityRequired,
            };
        } else {
            fail();
        }
    } else {
        fail();
    }
    return result;
}

auto
parseDirectorySnapshot(const QJsonObject& data) -> DirectorySnapshot
{
    requireExactKeys(data, { "revision", "rooms" });
    DirectorySnapshot result;
    result.revision = requiredSafeInteger(data, "revision");
    QSet<QString> ids;
    for (const auto& value : requiredArray(data, "rooms")) {
        if (!value.isObject()) {
            fail();
        }
        auto room = parseRoomSummary(value.toObject());
        if (ids.contains(room.roomId)) {
            fail();
        }
        ids.insert(room.roomId);
        result.rooms.push_back(std::move(room));
    }
    return result;
}

auto
parseRoomDirectoryUpdated(const QJsonObject& data) -> RoomDirectoryUpdated
{
    requireExactKeys(data, { "revision", "upserts", "removedRoomIds" });
    RoomDirectoryUpdated result;
    result.revision = requiredSafeInteger(data, "revision");
    QSet<QString> upsertIds;
    for (const auto& value : requiredArray(data, "upserts")) {
        if (!value.isObject()) {
            fail();
        }
        auto room = parseRoomSummary(value.toObject());
        if (upsertIds.contains(room.roomId)) {
            fail();
        }
        upsertIds.insert(room.roomId);
        result.upserts.push_back(std::move(room));
    }
    QSet<QString> removedIds;
    for (const auto& value : requiredArray(data, "removedRoomIds")) {
        if (!value.isString()) {
            fail();
        }
        auto roomId = value.toString();
        validateOpaqueId(roomId);
        if (removedIds.contains(roomId) || upsertIds.contains(roomId)) {
            fail();
        }
        removedIds.insert(roomId);
        result.removedRoomIds.push_back(std::move(roomId));
    }
    return result;
}

auto
parseFatalError(const QJsonObject& data) -> FatalError
{
    requireExactKeys(data, { "code", "displayMessageKey" });
    const auto displayKey = requiredString(data, "displayMessageKey");
    if (!isDisplayMessageKey(displayKey)) {
        fail();
    }
    return FatalError{ .code =
                         parseFatalErrorCode(requiredString(data, "code")),
                       .displayMessageKey = displayKey };
}

struct RoomEventHeader
{
    QString roomId;
    qint64 roomGeneration{};
};

auto
parseRoomEventHeader(const QJsonObject& data) -> RoomEventHeader
{
    auto roomId = requiredString(data, "roomId");
    validateOpaqueId(roomId);
    return { .roomId = std::move(roomId),
             .roomGeneration =
               requiredSafeInteger(data, "roomGeneration", true) };
}

auto
parseRoomMemberJoined(const QJsonObject& data) -> RoomMemberJoined
{
    requireExactKeys(data, { "roomId", "roomGeneration", "member" });
    auto header = parseRoomEventHeader(data);
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .member = parseMember(requiredObject(data, "member")) };
}

auto
parseRoomMemberUpdated(const QJsonObject& data) -> RoomMemberUpdated
{
    requireExactKeys(data, { "roomId", "roomGeneration", "member" });
    auto header = parseRoomEventHeader(data);
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .member = parseMember(requiredObject(data, "member")) };
}

auto
parseRoomMemberLeft(const QJsonObject& data) -> RoomMemberLeft
{
    requireExactKeys(data,
                     { "roomId", "roomGeneration", "memberId", "reason" });
    auto header = parseRoomEventHeader(data);
    auto memberId = requiredString(data, "memberId");
    validateOpaqueId(memberId);
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .memberId = std::move(memberId),
             .reason = parseMemberLeftReason(requiredString(data, "reason")) };
}

auto
parseRoomOwnerChanged(const QJsonObject& data) -> RoomOwnerChanged
{
    requireExactKeys(data, { "roomId", "roomGeneration", "ownerMemberId" });
    auto header = parseRoomEventHeader(data);
    auto owner = nullableString(data, "ownerMemberId");
    if (owner) {
        validateOpaqueId(*owner);
    }
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .ownerMemberId = std::move(owner) };
}

auto
parseChatMessageEvent(const QJsonObject& data) -> ChatMessageEvent
{
    requireExactKeys(data, { "roomId", "roomGeneration", "message" });
    auto header = parseRoomEventHeader(data);
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .message = parseChatMessage(requiredObject(data, "message")) };
}

auto
parseServerHeartbeat(const QJsonObject& data) -> ServerHeartbeat
{
    requireExactKeys(data, { "nonce", "sentAtMs" });
    auto nonce = requiredString(data, "nonce");
    validateOpaqueId(nonce);
    return { .nonce = std::move(nonce),
             .sentAtMs = requiredSafeInteger(data, "sentAtMs") };
}

auto
parseServerGoingAway(const QJsonObject& data) -> ServerGoingAway
{
    requireExactKeys(data, { "displayMessageKey" }, { "retryAfterMs" });
    if (requiredString(data, "displayMessageKey") !=
        QStringLiteral("arena.serverGoingAway")) {
        fail();
    }
    ServerGoingAway result;
    if (data.contains(QStringLiteral("retryAfterMs"))) {
        result.retryAfterMs = requiredSafeInteger(data, "retryAfterMs");
    }
    return result;
}

auto
parseCommandError(QString requestId, const QJsonObject& data) -> CommandError
{
    validateRequestId(requestId);
    requireExactKeys(data, { "code", "displayMessageKey" });
    const auto displayKey = requiredString(data, "displayMessageKey");
    if (!isDisplayMessageKey(displayKey)) {
        fail();
    }
    return { .requestId = std::move(requestId),
             .code = parseCommandErrorCode(requiredString(data, "code")),
             .displayMessageKey = displayKey };
}

struct ParsedRoomBinding
{
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
};

auto
parseRoomBinding(const QJsonObject& data) -> ParsedRoomBinding
{
    auto roomId = requiredString(data, "roomId");
    validateOpaqueId(roomId);
    return { .roomId = std::move(roomId),
             .roomGeneration =
               requiredSafeInteger(data, "roomGeneration", true),
             .connectionGeneration =
               requiredSafeInteger(data, "connectionGeneration", true) };
}

struct ParsedInventoryDeclaration
{
    qint64 libraryGeneration{};
    qint64 hashCount{};
    qint64 byteCount{};
    qint64 chunkCount{};
    QString vectorDigest;
};

auto
parseInventoryDeclaration(const QJsonObject& data) -> ParsedInventoryDeclaration
{
    ParsedInventoryDeclaration result{
        .libraryGeneration =
          requiredSafeInteger(data, "libraryGeneration", true),
        .hashCount = requiredSafeInteger(data, "hashCount"),
        .byteCount = requiredSafeInteger(data, "byteCount"),
        .chunkCount = requiredSafeInteger(data, "chunkCount"),
        .vectorDigest = requiredString(data, "vectorDigest"),
    };
    if (result.hashCount > MaxInventoryHashes ||
        result.byteCount > MaxInventoryBytes ||
        result.chunkCount > MaxInventoryChunks ||
        result.byteCount != result.hashCount * 32 ||
        result.chunkCount != (result.hashCount + InventoryHashesPerChunk - 1) /
                               InventoryHashesPerChunk ||
        !isLowerHex(result.vectorDigest, Sha256Characters)) {
        fail();
    }
    return result;
}

auto
parseInventoryUploadReady(QString requestId, const QJsonObject& data)
  -> InventoryUploadReady
{
    validateRequestId(requestId);
    requireExactKeys(data,
                     { "roomId",
                       "roomGeneration",
                       "connectionGeneration",
                       "uploadId",
                       "libraryGeneration",
                       "hashCount",
                       "byteCount",
                       "chunkCount",
                       "vectorDigest",
                       "deadlineMs" });
    auto binding = parseRoomBinding(data);
    auto uploadId = requiredString(data, "uploadId");
    validateTransferId(uploadId);
    auto declaration = parseInventoryDeclaration(data);
    return {
        .requestId = std::move(requestId),
        .roomId = std::move(binding.roomId),
        .roomGeneration = binding.roomGeneration,
        .connectionGeneration = binding.connectionGeneration,
        .uploadId = std::move(uploadId),
        .libraryGeneration = declaration.libraryGeneration,
        .hashCount = declaration.hashCount,
        .byteCount = declaration.byteCount,
        .chunkCount = declaration.chunkCount,
        .vectorDigest = std::move(declaration.vectorDigest),
        .deadlineMs = requiredSafeInteger(data, "deadlineMs"),
    };
}

auto
parseInventoryCommitted(QString requestId, const QJsonObject& data)
  -> InventoryCommitted
{
    validateRequestId(requestId);
    requireExactKeys(data,
                     { "roomId",
                       "roomGeneration",
                       "connectionGeneration",
                       "libraryGeneration",
                       "inventoryRevision",
                       "inventoryState" });
    auto binding = parseRoomBinding(data);
    if (requiredString(data, "inventoryState") != QStringLiteral("ready")) {
        fail();
    }
    return {
        .requestId = std::move(requestId),
        .roomId = std::move(binding.roomId),
        .roomGeneration = binding.roomGeneration,
        .connectionGeneration = binding.connectionGeneration,
        .libraryGeneration =
          requiredSafeInteger(data, "libraryGeneration", true),
        .inventoryRevision =
          requiredSafeInteger(data, "inventoryRevision", true),
        .inventoryState = InventoryState::Ready,
    };
}

void
validateAvailabilityDeclaration(qint64 count,
                                qint64 chunkCount,
                                QStringView digest)
{
    if (count < 0 || count > MaxInventoryHashes || chunkCount < 0 ||
        chunkCount > MaxInventoryChunks ||
        chunkCount !=
          (count + InventoryHashesPerChunk - 1) / InventoryHashesPerChunk ||
        !isLowerHex(digest, Sha256Characters)) {
        fail();
    }
}

auto
parseAvailabilityTransferBegin(const QJsonObject& data)
  -> AvailabilityTransferBegin
{
    const auto mode = requiredString(data, "mode");
    const auto reset = mode == QStringLiteral("reset");
    const auto delta = mode == QStringLiteral("delta");
    if (!reset && !delta) {
        fail();
    }
    if (reset) {
        requireExactKeys(data,
                         { "roomId",
                           "roomGeneration",
                           "transferId",
                           "mode",
                           "targetRevision",
                           "basis",
                           "resetCount",
                           "resetChunkCount",
                           "resetDigest" });
    } else {
        requireExactKeys(data,
                         { "roomId",
                           "roomGeneration",
                           "transferId",
                           "mode",
                           "baseRevision",
                           "targetRevision",
                           "basis",
                           "addedCount",
                           "addedChunkCount",
                           "addedDigest",
                           "removedCount",
                           "removedChunkCount",
                           "removedDigest" });
    }
    auto header = parseRoomEventHeader(data);
    auto transferId = requiredString(data, "transferId");
    validateTransferId(transferId);
    AvailabilityTransferBegin result{
        .roomId = std::move(header.roomId),
        .roomGeneration = header.roomGeneration,
        .transferId = std::move(transferId),
        .mode = reset ? AvailabilityTransferMode::Reset
                      : AvailabilityTransferMode::Delta,
        .targetRevision = requiredSafeInteger(data, "targetRevision", true),
        .basis = parseFrozenParticipants(requiredArray(data, "basis")),
    };
    if (reset) {
        result.resetCount = requiredSafeInteger(data, "resetCount");
        result.resetChunkCount = requiredSafeInteger(data, "resetChunkCount");
        result.resetDigest = requiredString(data, "resetDigest");
        validateAvailabilityDeclaration(
          result.resetCount, result.resetChunkCount, result.resetDigest);
    } else {
        result.baseRevision = requiredSafeInteger(data, "baseRevision", true);
        result.addedCount = requiredSafeInteger(data, "addedCount");
        result.addedChunkCount = requiredSafeInteger(data, "addedChunkCount");
        result.addedDigest = requiredString(data, "addedDigest");
        result.removedCount = requiredSafeInteger(data, "removedCount");
        result.removedChunkCount =
          requiredSafeInteger(data, "removedChunkCount");
        result.removedDigest = requiredString(data, "removedDigest");
        validateAvailabilityDeclaration(
          result.addedCount, result.addedChunkCount, result.addedDigest);
        validateAvailabilityDeclaration(
          result.removedCount, result.removedChunkCount, result.removedDigest);
        if (result.targetRevision <= result.baseRevision ||
            result.addedCount + result.removedCount > MaxInventoryHashes) {
            fail();
        }
    }
    return result;
}

auto
parseAvailabilityTransferCommit(const QJsonObject& data)
  -> AvailabilityTransferCommit
{
    requireExactKeys(
      data, { "roomId", "roomGeneration", "transferId", "targetRevision" });
    auto header = parseRoomEventHeader(data);
    auto transferId = requiredString(data, "transferId");
    validateTransferId(transferId);
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .transferId = std::move(transferId),
             .targetRevision =
               requiredSafeInteger(data, "targetRevision", true) };
}

auto
nullableSelection(const QJsonObject& data, const char* key)
  -> std::optional<SelectionSnapshot>
{
    const auto value = data.value(QString::fromLatin1(key));
    if (value.isNull()) {
        return std::nullopt;
    }
    if (!value.isObject()) {
        fail();
    }
    return parseSelection(value.toObject());
}

auto
parseSelectionChanged(const QJsonObject& data) -> SelectionChanged
{
    requireExactKeys(data,
                     { "roomId",
                       "roomGeneration",
                       "selectionRevision",
                       "availabilityRevision",
                       "selection",
                       "selectedByMemberId" });
    auto header = parseRoomEventHeader(data);
    auto selectedBy = nullableString(data, "selectedByMemberId");
    if (selectedBy) {
        validateOpaqueId(*selectedBy);
    }
    return {
        .roomId = std::move(header.roomId),
        .roomGeneration = header.roomGeneration,
        .selectionRevision =
          requiredSafeInteger(data, "selectionRevision", true),
        .availabilityRevision =
          requiredSafeInteger(data, "availabilityRevision", true),
        .selection = nullableSelection(data, "selection"),
        .selectedByMemberId = std::move(selectedBy),
    };
}

auto
parseSelectionRejected(QString requestId, const QJsonObject& data)
  -> SelectionRejected
{
    validateRequestId(requestId);
    const auto reason = requiredString(data, "reason");
    SelectionRejected result{ .requestId = std::move(requestId) };
    if (reason == QStringLiteral("not_common")) {
        requireExactKeys(data, { "reason", "missingMemberIds" });
        result.reason = SelectionRejectionReason::NotCommon;
        const auto ids = requiredArray(data, "missingMemberIds");
        if (ids.size() > RoomCapacity) {
            fail();
        }
        QSet<QString> seen;
        for (const auto& value : ids) {
            if (!value.isString()) {
                fail();
            }
            auto id = value.toString();
            validateOpaqueId(id);
            if (seen.contains(id)) {
                fail();
            }
            seen.insert(id);
            result.missingMemberIds.push_back(std::move(id));
        }
    } else if (reason == QStringLiteral("stale")) {
        requireExactKeys(data, { "reason" });
        result.reason = SelectionRejectionReason::Stale;
    } else if (reason == QStringLiteral("not_allowed")) {
        requireExactKeys(data, { "reason" });
        result.reason = SelectionRejectionReason::NotAllowed;
    } else {
        fail();
    }
    return result;
}

auto
parseRoundLoadingStarted(const QJsonObject& data) -> RoundLoadingStarted
{
    requireExactKeys(data, { "roomId", "roomGeneration", "round" });
    auto header = parseRoomEventHeader(data);
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .round = parseFrozenRound(requiredObject(data, "round")) };
}

auto
parseRoundProbeRequested(const QJsonObject& data) -> RoundProbeRequested
{
    requireExactKeys(data,
                     { "roomId",
                       "roomGeneration",
                       "connectionGeneration",
                       "roundId",
                       "launchAttemptId",
                       "selectionRevision",
                       "availabilityRevision",
                       "inventoryRevision",
                       "nonce",
                       "sha256",
                       "deadlineMs" });
    auto binding = parseRoomBinding(data);
    auto roundId = requiredString(data, "roundId");
    auto launchAttemptId = requiredString(data, "launchAttemptId");
    auto nonce = requiredString(data, "nonce");
    auto sha256 = requiredString(data, "sha256");
    validateOpaqueId(roundId);
    validateOpaqueId(launchAttemptId);
    validateOpaqueId(nonce);
    if (!isLowerHex(sha256, Sha256Characters)) {
        fail();
    }
    return {
        .roomId = std::move(binding.roomId),
        .roomGeneration = binding.roomGeneration,
        .connectionGeneration = binding.connectionGeneration,
        .roundId = std::move(roundId),
        .launchAttemptId = std::move(launchAttemptId),
        .selectionRevision =
          requiredSafeInteger(data, "selectionRevision", true),
        .availabilityRevision =
          requiredSafeInteger(data, "availabilityRevision", true),
        .inventoryRevision =
          requiredSafeInteger(data, "inventoryRevision", true),
        .nonce = std::move(nonce),
        .sha256 = std::move(sha256),
        .deadlineMs = requiredSafeInteger(data, "deadlineMs"),
    };
}

auto
parseRoundLoadRequested(const QJsonObject& data) -> RoundLoadRequested
{
    requireExactKeys(
      data, { "roomId", "roomGeneration", "connectionGeneration", "round" });
    auto binding = parseRoomBinding(data);
    return { .roomId = std::move(binding.roomId),
             .roomGeneration = binding.roomGeneration,
             .connectionGeneration = binding.connectionGeneration,
             .round = parseFrozenRound(requiredObject(data, "round")) };
}

auto
parseRoundStartScheduled(const QJsonObject& data) -> RoundStartScheduled
{
    const auto legacy = hasExactKeys(data,
                                     { "roomId",
                                       "roomGeneration",
                                       "connectionGeneration",
                                       "roundId",
                                       "launchAttemptId",
                                       "startAtServerMs",
                                       "startAfterMs" });
    const auto competition = hasExactKeys(data,
                                          { "roomId",
                                            "roomGeneration",
                                            "connectionGeneration",
                                            "roundId",
                                            "launchAttemptId",
                                            "startAtServerMs",
                                            "startAfterMs",
                                            "playDeadlineAtServerMs" });
    if (!legacy && !competition) {
        fail();
    }
    auto binding = parseRoomBinding(data);
    auto roundId = requiredString(data, "roundId");
    auto launchAttemptId = requiredString(data, "launchAttemptId");
    validateOpaqueId(roundId);
    validateOpaqueId(launchAttemptId);
    const auto startAfterMs = requiredSafeInteger(data, "startAfterMs", true);
    if (startAfterMs < MinRoundStartAfterMs ||
        startAfterMs > MaxRoundStartAfterMs) {
        fail();
    }
    RoundStartScheduled result{
        .roomId = std::move(binding.roomId),
        .roomGeneration = binding.roomGeneration,
        .connectionGeneration = binding.connectionGeneration,
        .roundId = std::move(roundId),
        .launchAttemptId = std::move(launchAttemptId),
        .startAtServerMs = requiredSafeInteger(data, "startAtServerMs"),
        .startAfterMs = startAfterMs,
    };
    if (competition) {
        const auto deadline =
          requiredSafeInteger(data, "playDeadlineAtServerMs");
        if (deadline < result.startAtServerMs) {
            fail();
        }
        result.playDeadlineAtServerMs = deadline;
    }
    return result;
}

auto
parseRoundStarted(const QJsonObject& data) -> RoundStarted
{
    const auto legacy = hasExactKeys(
      data, { "roomId", "roomGeneration", "roundId", "launchAttemptId" });
    const auto competition = hasExactKeys(data,
                                          { "roomId",
                                            "roomGeneration",
                                            "roundId",
                                            "launchAttemptId",
                                            "playDeadlineAtServerMs" });
    if (!legacy && !competition) {
        fail();
    }
    auto header = parseRoomEventHeader(data);
    auto roundId = requiredString(data, "roundId");
    auto launchAttemptId = requiredString(data, "launchAttemptId");
    validateOpaqueId(roundId);
    validateOpaqueId(launchAttemptId);
    RoundStarted result{ .roomId = std::move(header.roomId),
                         .roomGeneration = header.roomGeneration,
                         .roundId = std::move(roundId),
                         .launchAttemptId = std::move(launchAttemptId) };
    if (competition) {
        result.playDeadlineAtServerMs =
          requiredSafeInteger(data, "playDeadlineAtServerMs");
    }
    return result;
}

auto
parseRoundTerminalAccepted(QString requestId, const QJsonObject& data)
  -> RoundTerminalAccepted
{
    validateRequestId(requestId);
    requireExactKeys(
      data,
      { "roomId", "roomGeneration", "roundId", "launchAttemptId", "terminal" });
    auto header = parseRoomEventHeader(data);
    auto roundId = requiredString(data, "roundId");
    auto launchAttemptId = requiredString(data, "launchAttemptId");
    validateOpaqueId(roundId);
    validateOpaqueId(launchAttemptId);
    const auto terminal = requiredString(data, "terminal");
    TerminalKind terminalKind;
    if (terminal == QStringLiteral("finished")) {
        terminalKind = TerminalKind::Finished;
    } else if (terminal == QStringLiteral("dnf")) {
        terminalKind = TerminalKind::Dnf;
    } else {
        fail();
    }
    return { .requestId = std::move(requestId),
             .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .roundId = std::move(roundId),
             .launchAttemptId = std::move(launchAttemptId),
             .terminal = terminalKind };
}

auto
parseRoundFinalized(const QJsonObject& data) -> RoundFinalized
{
    requireExactKeys(data,
                     { "roomId",
                       "roomGeneration",
                       "roundId",
                       "launchAttemptId",
                       "result",
                       "members" });
    auto header = parseRoomEventHeader(data);
    RoundFinalized finalized{
        .roomId = std::move(header.roomId),
        .roomGeneration = header.roomGeneration,
        .roundId = requiredString(data, "roundId"),
        .launchAttemptId = requiredString(data, "launchAttemptId"),
        .result = parseRoundResultSnapshot(requiredObject(data, "result")),
    };
    validateOpaqueId(finalized.roundId);
    validateOpaqueId(finalized.launchAttemptId);
    if (finalized.result.roundId != finalized.roundId) {
        fail();
    }
    const auto members = requiredArray(data, "members");
    if (members.size() > RoomCapacity) {
        fail();
    }
    QSet<QString> ids;
    for (const auto& value : members) {
        if (!value.isObject()) {
            fail();
        }
        const auto object = value.toObject();
        if (!object.contains(QStringLiteral("ready"))) {
            fail();
        }
        auto member = parseMember(object);
        if (ids.contains(member.memberId)) {
            fail();
        }
        ids.insert(member.memberId);
        finalized.members.push_back(std::move(member));
    }
    return finalized;
}

auto
parseRoundLaunchCancellationReason(QStringView value)
  -> RoundLaunchCancellationReason
{
    static const std::pair<QStringView, RoundLaunchCancellationReason>
      entries[]{
          { u"missing_file", RoundLaunchCancellationReason::MissingFile },
          { u"hash_mismatch", RoundLaunchCancellationReason::HashMismatch },
          { u"read_failed", RoundLaunchCancellationReason::ReadFailed },
          { u"parse_failed", RoundLaunchCancellationReason::ParseFailed },
          { u"unsupported_config",
            RoundLaunchCancellationReason::UnsupportedConfig },
          { u"resource_failed", RoundLaunchCancellationReason::ResourceFailed },
          { u"probe_timeout", RoundLaunchCancellationReason::ProbeTimeout },
          { u"load_timeout", RoundLaunchCancellationReason::LoadTimeout },
          { u"participant_left",
            RoundLaunchCancellationReason::ParticipantLeft },
          { u"participant_kicked",
            RoundLaunchCancellationReason::ParticipantKicked },
          { u"chart_length_mismatch",
            RoundLaunchCancellationReason::ChartLengthMismatch },
          { u"server_shutdown", RoundLaunchCancellationReason::ServerShutdown },
          { u"cancelled", RoundLaunchCancellationReason::Cancelled },
      };
    for (const auto& [spelling, reason] : entries) {
        if (value == spelling) {
            return reason;
        }
    }
    fail();
}

auto
parseRoundLaunchCancelled(const QJsonObject& data) -> RoundLaunchCancelled
{
    requireExactKeys(data,
                     { "roomId",
                       "roomGeneration",
                       "roundId",
                       "launchAttemptId",
                       "reason",
                       "selection",
                       "selectionRevision",
                       "availabilityRevision" });
    auto header = parseRoomEventHeader(data);
    auto roundId = requiredString(data, "roundId");
    auto launchAttemptId = requiredString(data, "launchAttemptId");
    validateOpaqueId(roundId);
    validateOpaqueId(launchAttemptId);
    return {
        .roomId = std::move(header.roomId),
        .roomGeneration = header.roomGeneration,
        .roundId = std::move(roundId),
        .launchAttemptId = std::move(launchAttemptId),
        .reason =
          parseRoundLaunchCancellationReason(requiredString(data, "reason")),
        .selection = nullableSelection(data, "selection"),
        .selectionRevision = requiredSafeInteger(data, "selectionRevision"),
        .availabilityRevision =
          requiredSafeInteger(data, "availabilityRevision"),
    };
}

} // namespace

auto
encodeClientMessage(const ClientMessage& message) -> EncodeClientResult
{
    try {
        const auto object = std::visit(
          [](const auto& value) -> QJsonObject {
              using Value = std::decay_t<decltype(value)>;
              if constexpr (std::is_same_v<Value, ClientHello>) {
                  return encodeHello(value);
              } else if constexpr (std::is_same_v<Value, DirectorySubscribe>) {
                  return encodeDirectorySubscribe(value);
              } else if constexpr (std::is_same_v<Value, RoomCreate>) {
                  return encodeRoomCreate(value);
              } else if constexpr (std::is_same_v<Value, RoomJoin>) {
                  return encodeRoomJoin(value);
              } else if constexpr (std::is_same_v<Value, RoomLeave>) {
                  return encodeRoomLeave(value);
              } else if constexpr (std::is_same_v<Value, RoomKick>) {
                  return encodeRoomKick(value);
              } else if constexpr (std::is_same_v<Value, ChatSend>) {
                  return encodeChatSend(value);
              } else if constexpr (std::is_same_v<Value, HeartbeatReply>) {
                  return encodeHeartbeatReply(value);
              } else if constexpr (std::is_same_v<Value,
                                                  InventoryUploadBegin>) {
                  return encodeInventoryUploadBegin(value);
              } else if constexpr (std::is_same_v<Value,
                                                  InventoryUploadCommit>) {
                  return encodeInventoryUploadCommit(value);
              } else if constexpr (std::is_same_v<Value,
                                                  InventoryUploadAbort>) {
                  return encodeInventoryUploadAbort(value);
              } else if constexpr (std::is_same_v<Value, AvailabilityApplied>) {
                  return encodeAvailabilityApplied(value);
              } else if constexpr (std::is_same_v<Value, AvailabilityResync>) {
                  return encodeAvailabilityResync(value);
              } else if constexpr (std::is_same_v<Value, SelectionSet>) {
                  return encodeSelectionSet(value);
              } else if constexpr (std::is_same_v<Value, ReadySet>) {
                  return encodeReadySet(value);
              } else if constexpr (std::is_same_v<Value, RoundProbeResult>) {
                  return encodeRoundProbeResult(value);
              } else if constexpr (std::is_same_v<Value, RoundLoadResult>) {
                  return encodeRoundLoadResult(value);
              } else if constexpr (std::is_same_v<Value, RoundTelemetry>) {
                  return encodeRoundTelemetry(value);
              } else if constexpr (std::is_same_v<Value, RoundResultSubmit>) {
                  return encodeRoundResultSubmit(value);
              } else if constexpr (std::is_same_v<Value, RoundAbandon>) {
                  return encodeRoundAbandon(value);
              } else {
                  fail();
              }
          },
          message);
        const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
        if (bytes.size() > MaxClientMessageBytes) {
            fail(ProtocolFailureCode::FrameTooLarge);
        }
        return QString::fromUtf8(bytes);
    } catch (const DecodeFailure& failure) {
        return ProtocolFailure{ failure.code };
    }
}

auto
decodeServerMessage(QStringView text) -> DecodeServerResult
{
    // UTF-8 cannot contain fewer bytes than the number of UTF-16 code units.
    // Reject obviously oversized frames without allocating a second copy.
    if (text.size() > MaxServerMessageBytes) {
        return ProtocolFailure{ ProtocolFailureCode::FrameTooLarge };
    }
    const auto bytes = text.toString().toUtf8();
    if (bytes.size() > MaxServerMessageBytes) {
        return ProtocolFailure{ ProtocolFailureCode::FrameTooLarge };
    }
    try {
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(bytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || document.isNull() ||
            !document.isObject()) {
            fail();
        }
        const auto envelope = document.object();
        const auto typeValue = envelope.value(QStringLiteral("type"));
        const auto dataValue = envelope.value(QStringLiteral("data"));
        if (!typeValue.isString() || !dataValue.isObject()) {
            fail();
        }
        const auto type = typeValue.toString();
        if ((type == QStringLiteral("round_standings") &&
             bytes.size() > MaxStandingsMessageBytes) ||
            (type == QStringLiteral("round_finalized") &&
             bytes.size() > MaxFinalizationMessageBytes)) {
            fail(ProtocolFailureCode::FrameTooLarge);
        }
        if (type == QStringLiteral("server_hello")) {
            // Version/capability negotiation failures take precedence over
            // unrelated schema errors in a recognizable hello.
            const auto data = dataValue.toObject();
            const auto major = data.value(QStringLiteral("protocolMajor"));
            const auto minor = data.value(QStringLiteral("protocolMinor"));
            if ((major.isDouble() && major.toDouble() != ProtocolMajor) ||
                (minor.isDouble() && (minor.toDouble() < LegacyProtocolMinor ||
                                      minor.toDouble() > ProtocolMinor))) {
                fail(ProtocolFailureCode::ProtocolIncompatible);
            }
            const auto capabilities =
              data.value(QStringLiteral("capabilities"));
            if (capabilities.isArray()) {
                bool allStrings = true;
                bool foundRequired = false;
                for (const auto& value : capabilities.toArray()) {
                    allStrings = allStrings && value.isString();
                    foundRequired =
                      foundRequired ||
                      value.toString() == QString::fromLatin1(RoomsCapability);
                }
                if (allStrings && !foundRequired) {
                    fail(ProtocolFailureCode::CapabilityRequired);
                }
            }
            requireExactKeys(envelope, { "type", "data" });
            return ServerMessage{ parseServerHello(data) };
        }
        const auto data = dataValue.toObject();
        if (type == QStringLiteral("room_snapshot")) {
            requireExactKeys(envelope, { "type", "requestId", "data" });
            auto requestId = requiredString(envelope, "requestId");
            validateRequestId(requestId);
            return ServerMessage{ RoomSnapshotEvent{
              .requestId = std::move(requestId),
              .room = parseRoomSnapshot(data),
            } };
        }
        if (type == QStringLiteral("command_error")) {
            requireExactKeys(envelope, { "type", "requestId", "data" });
            return ServerMessage{ parseCommandError(
              requiredString(envelope, "requestId"), data) };
        }
        if (type == QStringLiteral("inventory_upload_ready")) {
            requireExactKeys(envelope, { "type", "requestId", "data" });
            return ServerMessage{ parseInventoryUploadReady(
              requiredString(envelope, "requestId"), data) };
        }
        if (type == QStringLiteral("inventory_committed")) {
            requireExactKeys(envelope, { "type", "requestId", "data" });
            return ServerMessage{ parseInventoryCommitted(
              requiredString(envelope, "requestId"), data) };
        }
        if (type == QStringLiteral("selection_rejected")) {
            requireExactKeys(envelope, { "type", "requestId", "data" });
            return ServerMessage{ parseSelectionRejected(
              requiredString(envelope, "requestId"), data) };
        }
        if (type == QStringLiteral("round_terminal_accepted")) {
            requireExactKeys(envelope, { "type", "requestId", "data" });
            return ServerMessage{ parseRoundTerminalAccepted(
              requiredString(envelope, "requestId"), data) };
        }

        requireExactKeys(envelope, { "type", "data" });
        if (type == QStringLiteral("fatal_error")) {
            return ServerMessage{ parseFatalError(data) };
        }
        if (type == QStringLiteral("directory_snapshot")) {
            return ServerMessage{ parseDirectorySnapshot(data) };
        }
        if (type == QStringLiteral("room_directory_updated")) {
            return ServerMessage{ parseRoomDirectoryUpdated(data) };
        }
        if (type == QStringLiteral("room_member_joined")) {
            return ServerMessage{ parseRoomMemberJoined(data) };
        }
        if (type == QStringLiteral("room_member_updated")) {
            return ServerMessage{ parseRoomMemberUpdated(data) };
        }
        if (type == QStringLiteral("room_member_left")) {
            return ServerMessage{ parseRoomMemberLeft(data) };
        }
        if (type == QStringLiteral("room_owner_changed")) {
            return ServerMessage{ parseRoomOwnerChanged(data) };
        }
        if (type == QStringLiteral("chat_message")) {
            return ServerMessage{ parseChatMessageEvent(data) };
        }
        if (type == QStringLiteral("server_heartbeat")) {
            return ServerMessage{ parseServerHeartbeat(data) };
        }
        if (type == QStringLiteral("server_going_away")) {
            return ServerMessage{ parseServerGoingAway(data) };
        }
        if (type == QStringLiteral("availability_transfer_begin")) {
            return ServerMessage{ parseAvailabilityTransferBegin(data) };
        }
        if (type == QStringLiteral("availability_transfer_commit")) {
            return ServerMessage{ parseAvailabilityTransferCommit(data) };
        }
        if (type == QStringLiteral("selection_changed")) {
            return ServerMessage{ parseSelectionChanged(data) };
        }
        if (type == QStringLiteral("round_loading_started")) {
            return ServerMessage{ parseRoundLoadingStarted(data) };
        }
        if (type == QStringLiteral("round_probe_requested")) {
            return ServerMessage{ parseRoundProbeRequested(data) };
        }
        if (type == QStringLiteral("round_load_requested")) {
            return ServerMessage{ parseRoundLoadRequested(data) };
        }
        if (type == QStringLiteral("round_start_scheduled")) {
            return ServerMessage{ parseRoundStartScheduled(data) };
        }
        if (type == QStringLiteral("round_started")) {
            return ServerMessage{ parseRoundStarted(data) };
        }
        if (type == QStringLiteral("round_standings")) {
            return ServerMessage{ parseLiveStandings(data) };
        }
        if (type == QStringLiteral("round_finalized")) {
            return ServerMessage{ parseRoundFinalized(data) };
        }
        if (type == QStringLiteral("round_launch_cancelled")) {
            return ServerMessage{ parseRoundLaunchCancelled(data) };
        }
        fail();
    } catch (const DecodeFailure& failure) {
        return ProtocolFailure{ failure.code };
    }
}

auto
displayMessageKey(ProtocolFailureCode code) -> QString
{
    switch (code) {
        case ProtocolFailureCode::MalformedMessage:
            return QStringLiteral("arena.error.malformedMessage");
        case ProtocolFailureCode::FrameTooLarge:
            return QStringLiteral("arena.error.frameTooLarge");
        case ProtocolFailureCode::ProtocolIncompatible:
            return QStringLiteral("arena.error.protocolIncompatible");
        case ProtocolFailureCode::CapabilityRequired:
            return QStringLiteral("arena.error.capabilityRequired");
    }
    return {};
}

} // namespace arena
