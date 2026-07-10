#pragma once

#include "ArenaIdentityProvider.h"

#include <QMetaObject>
#include <QPointer>
#include <QVector>

namespace qml_components {
class ProfileList;
}
namespace resource_managers {
class ArenaTicketOperation;
class Profile;
}

namespace arena {

class ProfileArenaIdentityProvider final : public ArenaIdentityProvider
{
    Q_OBJECT
  public:
    explicit ProfileArenaIdentityProvider(
      qml_components::ProfileList* profileList,
      QObject* parent = nullptr);
    ~ProfileArenaIdentityProvider() override;

    [[nodiscard]] auto loggedIn() const -> bool override;
    [[nodiscard]] auto identity() const
      -> std::optional<PublicIdentity> override;
    void requestTicket(quint64 requestId) override;

  private:
    QPointer<qml_components::ProfileList> m_profileList;
    QPointer<resource_managers::Profile> m_profile;
    QVector<QMetaObject::Connection> m_profileConnections{};
    QVector<QPointer<resource_managers::ArenaTicketOperation>>
      m_ticketOperations{};

    void bindActiveProfile();
    void cancelTicketOperations();
    void removeTicketOperation(
      resource_managers::ArenaTicketOperation* operation);
};

} // namespace arena
