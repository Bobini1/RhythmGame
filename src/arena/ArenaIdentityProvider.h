#pragma once

#include "ArenaTypes.h"

#include <QObject>

#include <optional>

namespace arena {

class ArenaIdentityProvider : public QObject
{
    Q_OBJECT
  public:
    enum class TicketFailure
    {
        NotLoggedIn,
        Network,
        Rejected,
        MalformedResponse
    };
    Q_ENUM(TicketFailure)

    using QObject::QObject;
    ~ArenaIdentityProvider() override = default;

    [[nodiscard]] virtual auto loggedIn() const -> bool = 0;
    [[nodiscard]] virtual auto identity() const
      -> std::optional<PublicIdentity> = 0;
    virtual void requestTicket(quint64 requestId) = 0;

  signals:
    void activeProfileChanged();
    void loginStateChanged();
    void identityChanged();
    void ticketReady(quint64 requestId, const QString& ticket);
    void ticketFailed(quint64 requestId,
                      arena::ArenaIdentityProvider::TicketFailure failure);
};

} // namespace arena
