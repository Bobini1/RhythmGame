#include "Lr2SkinElementNumberState.h"

#include <limits>

Lr2SkinElementNumberState::Lr2SkinElementNumberState(QObject* parent)
  : QObject(parent)
{
}

int
Lr2SkinElementNumberState::dependencyMask() const
{
    return m_dependencyMask;
}

int
Lr2SkinElementNumberState::revision() const
{
    return m_revision;
}

void
Lr2SkinElementNumberState::setDependencyMask(int dependencyMask)
{
    if (m_dependencyMask == dependencyMask) {
        return;
    }
    m_dependencyMask = dependencyMask;
    emit dependencyMaskChanged();
}

void
Lr2SkinElementNumberState::refresh()
{
    m_revision =
      m_revision == std::numeric_limits<int>::max() ? 0 : m_revision + 1;
    emit revisionChanged();
}
