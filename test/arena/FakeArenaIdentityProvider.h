#pragma once

#include "arena/ArenaIdentityProvider.h"

#include <utility>

namespace arena::test {

class FakeArenaIdentityProvider final : public ArenaIdentityProvider
{
  public:
    using ArenaIdentityProvider::ArenaIdentityProvider;

    bool loggedInValue{};
    std::optional<PublicIdentity> identityValue{ std::nullopt };
    QVector<quint64> ticketRequests{};

    [[nodiscard]] auto loggedIn() const -> bool override
    {
        return loggedInValue;
    }
    [[nodiscard]] auto identity() const
      -> std::optional<PublicIdentity> override
    {
        return identityValue;
    }
    void requestTicket(quint64 requestId) override
    {
        ticketRequests.push_back(requestId);
    }

    void setLoggedIn(bool value)
    {
        loggedInValue = value;
        emit loginStateChanged();
    }
    void setIdentity(std::optional<PublicIdentity> value)
    {
        identityValue = std::move(value);
        emit identityChanged();
    }
    void replaceActiveProfile(bool loggedIn,
                              std::optional<PublicIdentity> identity)
    {
        loggedInValue = loggedIn;
        identityValue = std::move(identity);
        emit activeProfileChanged();
    }
    void succeedTicket(quint64 requestId, QString ticket)
    {
        emit ticketReady(requestId, std::move(ticket));
    }
    void failTicket(quint64 requestId, TicketFailure failure)
    {
        emit ticketFailed(requestId, failure);
    }
};

} // namespace arena::test
