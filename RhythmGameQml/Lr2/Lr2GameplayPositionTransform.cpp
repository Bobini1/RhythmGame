#include "Lr2GameplayPositionTransform.h"

#include "Lr2GameplayFrameState.h"
#include "gameplay_logic/ChartRunner.h"

#include <QMatrix4x4>
#include <QQuickWindow>

#include <cmath>

using gameplay_logic::Player;

Lr2GameplayPositionTransform::Lr2GameplayPositionTransform(QObject* parent)
  : QQuickTransform(parent)
{
}

Lr2GameplayFrameState*
Lr2GameplayPositionTransform::frameState() const
{
    return m_frameState;
}

void
Lr2GameplayPositionTransform::setFrameState(Lr2GameplayFrameState* state)
{
    if (m_frameState == state) {
        return;
    }
    m_frameState = state;
    reconnectPositionSignal();
    emit frameStateChanged();
    update();
}

QObject*
Lr2GameplayPositionTransform::player() const
{
    return m_player;
}

void
Lr2GameplayPositionTransform::setPlayer(QObject* player)
{
    if (m_player == player) {
        return;
    }
    m_player = player;
    reconnectPositionSignal();
    emit playerChanged();
    update();
}

QQuickItem*
Lr2GameplayPositionTransform::targetItem() const
{
    return m_targetItem;
}

void
Lr2GameplayPositionTransform::setTargetItem(QQuickItem* item)
{
    if (m_targetItem == item) {
        return;
    }

    if (m_targetWindowConnection) {
        disconnect(m_targetWindowConnection);
        m_targetWindowConnection = {};
    }
    if (m_targetDestroyedConnection) {
        disconnect(m_targetDestroyedConnection);
        m_targetDestroyedConnection = {};
    }

    m_targetItem = item;
    if (m_targetItem) {
        m_targetWindowConnection =
          connect(m_targetItem,
                  &QQuickItem::windowChanged,
                  this,
                  &Lr2GameplayPositionTransform::reconnectRenderFrameSignal);
        m_targetDestroyedConnection =
          connect(m_targetItem, &QObject::destroyed, this, [this]() {
              m_targetItem = nullptr;
              reconnectRenderFrameSignal(nullptr);
              emit targetItemChanged();
          });
    }

    reconnectRenderFrameSignal(m_targetItem ? m_targetItem->window() : nullptr);
    emit targetItemChanged();
    update();
}

int
Lr2GameplayPositionTransform::side() const
{
    return m_side;
}

void
Lr2GameplayPositionTransform::setSide(int side)
{
    side = side == 2 ? 2 : 1;
    if (m_side == side) {
        return;
    }
    m_side = side;
    reconnectPositionSignal();
    emit sideChanged();
    update();
}

qreal
Lr2GameplayPositionTransform::multiplier() const
{
    return m_multiplier;
}

void
Lr2GameplayPositionTransform::setMultiplier(qreal multiplier)
{
    if (!std::isfinite(multiplier)) {
        multiplier = 0.0;
    }
    if (std::abs(m_multiplier - multiplier) <= 0.000001) {
        return;
    }
    m_multiplier = multiplier;
    emit multiplierChanged();
    update();
}

void
Lr2GameplayPositionTransform::applyTo(QMatrix4x4* matrix) const
{
    if (!matrix || m_multiplier == 0.0) {
        return;
    }

    qreal position = 0.0;
    if (m_frameState) {
        position = m_frameState->positionAtRenderTime(m_side);
    } else if (const auto* player = qobject_cast<Player*>(m_player.data())) {
        position = player->getPosition();
    }
    if (std::isfinite(position)) {
        matrix->translate(0.0F, static_cast<float>(position * m_multiplier));
    }
}

void
Lr2GameplayPositionTransform::reconnectPositionSignal()
{
    if (m_positionConnection) {
        disconnect(m_positionConnection);
        m_positionConnection = {};
    }

    if (auto* player = qobject_cast<Player*>(m_player.data())) {
        m_positionConnection = connect(player,
                                       &Player::positionChanged,
                                       this,
                                       &Lr2GameplayPositionTransform::update);
        return;
    }

    if (m_frameState) {
        m_positionConnection =
          m_side == 2 ? connect(m_frameState,
                                &Lr2GameplayFrameState::position2Changed,
                                this,
                                &Lr2GameplayPositionTransform::update)
                      : connect(m_frameState,
                                &Lr2GameplayFrameState::position1Changed,
                                this,
                                &Lr2GameplayPositionTransform::update);
    }
}

void
Lr2GameplayPositionTransform::reconnectRenderFrameSignal(QQuickWindow* window)
{
    if (m_renderFrameConnection) {
        disconnect(m_renderFrameConnection);
        m_renderFrameConnection = {};
    }

    if (window) {
        m_renderFrameConnection =
          connect(window,
                  &QQuickWindow::beforeSynchronizing,
                  this,
                  &Lr2GameplayPositionTransform::updateForRenderFrame,
                  Qt::DirectConnection);
    }
}

void
Lr2GameplayPositionTransform::updateForRenderFrame()
{
    update();
}
