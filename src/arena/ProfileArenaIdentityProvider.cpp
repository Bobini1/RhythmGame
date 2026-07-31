#include "ProfileArenaIdentityProvider.h"

#include "qml_components/ProfileList.h"
#include "resource_managers/Profile.h"

#include <utility>

namespace arena {
namespace {

auto
mapTicketFailure(resource_managers::ArenaTicketOperation::Error error)
  -> ArenaIdentityProvider::TicketFailure
{
    using Source = resource_managers::ArenaTicketOperation::Error;
    switch (error) {
        case Source::NotLoggedIn:
            return ArenaIdentityProvider::TicketFailure::NotLoggedIn;
        case Source::Network:
            return ArenaIdentityProvider::TicketFailure::Network;
        case Source::Rejected:
            return ArenaIdentityProvider::TicketFailure::Rejected;
        case Source::MalformedResponse:
            return ArenaIdentityProvider::TicketFailure::MalformedResponse;
    }
    return ArenaIdentityProvider::TicketFailure::MalformedResponse;
}

} // namespace

ProfileArenaIdentityProvider::ProfileArenaIdentityProvider(
  qml_components::ProfileList* profileList,
  QObject* parent)
  : ArenaIdentityProvider(parent)
  , m_profileList(profileList)
{
    Q_ASSERT(profileList != nullptr);
    m_ticketOperations.reserve(1);
    connect(profileList,
            &qml_components::ProfileList::mainProfileChanged,
            this,
            [this] {
                for (const auto& connection :
                     std::as_const(m_profileConnections)) {
                    disconnect(connection);
                }
                m_profileConnections.clear();
                cancelTicketOperations();
                m_profile.clear();
                bindActiveProfile();
                emit activeProfileChanged();
            });
    bindActiveProfile();
}

ProfileArenaIdentityProvider::~ProfileArenaIdentityProvider()
{
    if (!m_ticketOperations.isEmpty()) {
        cancelTicketOperations();
    }
    for (const auto& connection : std::as_const(m_profileConnections)) {
        disconnect(connection);
    }
}

void
ProfileArenaIdentityProvider::bindActiveProfile()
{
    if (!m_profileList) {
        return;
    }
    m_profile = m_profileList->getMainProfile();
    if (!m_profile) {
        return;
    }
    m_profileConnections.push_back(connect(
      m_profile, &resource_managers::Profile::loginStateChanged, this, [this] {
          emit loginStateChanged();
      }));
    m_profileConnections.push_back(
      connect(m_profile,
              &resource_managers::Profile::onlineUserDataChanged,
              this,
              [this] { emit identityChanged(); }));
    const QPointer<resource_managers::Profile> boundProfile(m_profile);
    m_profileConnections.push_back(
      connect(m_profile, &QObject::destroyed, this, [this, boundProfile] {
          if (m_profile == boundProfile) {
              cancelTicketOperations();
              m_profile.clear();
              emit loginStateChanged();
              emit identityChanged();
          }
      }));
}

auto
ProfileArenaIdentityProvider::loggedIn() const -> bool
{
    return m_profile && m_profile->getLoginState() ==
                          resource_managers::Profile::LoginState::LoggedIn;
}

auto
ProfileArenaIdentityProvider::identity() const -> std::optional<PublicIdentity>
{
    if (!m_profile) {
        return std::nullopt;
    }
    const auto data = m_profile->getOnlineUserDataValue();
    if (!data) {
        return std::nullopt;
    }
    return PublicIdentity{
        .userId = QString::number(data->userId),
        .displayName = data->username,
        .avatarUrl = data->image.isEmpty()
                       ? std::nullopt
                       : std::optional<QString>{ data->image },
    };
}

void
ProfileArenaIdentityProvider::requestTicket(quint64 requestId)
{
    if (!m_profile || !loggedIn()) {
        QMetaObject::invokeMethod(
          this,
          [this, requestId] {
              emit ticketFailed(requestId, TicketFailure::NotLoggedIn);
          },
          Qt::QueuedConnection);
        return;
    }
    if (!m_ticketOperations.isEmpty()) {
        cancelTicketOperations();
    }
    const QPointer<resource_managers::Profile> profileAtRequest(m_profile);
    auto* operation = m_profile->requestArenaTicket();
    m_ticketOperations.push_back(operation);
    connect(
      operation,
      &resource_managers::ArenaTicketOperation::succeeded,
      this,
      [this, operation, profileAtRequest, requestId](const QString& ticket) {
          removeTicketOperation(operation);
          if (!m_profile || m_profile != profileAtRequest) {
              return;
          }
          emit ticketReady(requestId, ticket);
      });
    connect(operation,
            &resource_managers::ArenaTicketOperation::failed,
            this,
            [this, operation, profileAtRequest, requestId](
              resource_managers::ArenaTicketOperation::Error error) {
                removeTicketOperation(operation);
                if (!m_profile || m_profile != profileAtRequest) {
                    return;
                }
                emit ticketFailed(requestId, mapTicketFailure(error));
            });
}

void
ProfileArenaIdentityProvider::removeTicketOperation(
  resource_managers::ArenaTicketOperation* operation)
{
    for (auto it = m_ticketOperations.begin();
         it != m_ticketOperations.end();) {
        if (!*it || *it == operation) {
            it = m_ticketOperations.erase(it);
        } else {
            ++it;
        }
    }
}

void
ProfileArenaIdentityProvider::cancelTicketOperations()
{
    const auto operations = std::exchange(
      m_ticketOperations,
      QVector<QPointer<resource_managers::ArenaTicketOperation>>{});
    for (const auto& operation : operations) {
        if (operation) {
            operation->cancel();
        }
    }
}

} // namespace arena
