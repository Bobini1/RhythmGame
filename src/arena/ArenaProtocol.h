#pragma once

#include "ArenaTypes.h"

#include <QString>
#include <QStringView>

#include <variant>

namespace arena {

enum class ProtocolFailureCode
{
    MalformedMessage,
    FrameTooLarge,
    ProtocolIncompatible,
    CapabilityRequired,
};

struct ProtocolFailure
{
    ProtocolFailureCode code;
    bool operator==(const ProtocolFailure&) const = default;
};

using EncodeClientResult = std::variant<QString, ProtocolFailure>;
using DecodeServerResult = std::variant<ServerMessage, ProtocolFailure>;

[[nodiscard]] auto
encodeClientMessage(const ClientMessage& message) -> EncodeClientResult;
[[nodiscard]] auto
decodeServerMessage(QStringView text) -> DecodeServerResult;
[[nodiscard]] auto
displayMessageKey(ProtocolFailureCode code) -> QString;

} // namespace arena
