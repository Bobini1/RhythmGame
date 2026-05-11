#include "Lr2SkinElementActiveOptionsState.h"

Lr2SkinElementActiveOptionsState::Lr2SkinElementActiveOptionsState(QObject* parent)
    : QObject(parent) {}

QVariantList Lr2SkinElementActiveOptionsState::activeOptions() const {
    return m_activeOptions;
}

bool Lr2SkinElementActiveOptionsState::setActiveOptions(const QVariantList& value) {
    if (m_activeOptions == value) {
        return false;
    }

    m_activeOptions = value;
    emit activeOptionsChanged();
    return true;
}

