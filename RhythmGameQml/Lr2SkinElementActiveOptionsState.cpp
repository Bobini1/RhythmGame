#include "Lr2SkinElementActiveOptionsState.h"

Lr2SkinElementActiveOptionsState::Lr2SkinElementActiveOptionsState(QObject* parent)
    : QObject(parent) {}

QVariantList Lr2SkinElementActiveOptionsState::activeOptions() const {
    return m_activeOptions;
}

QSet<int> Lr2SkinElementActiveOptionsState::activeOptionSet() const {
    return m_activeOptionSet;
}

bool Lr2SkinElementActiveOptionsState::setActiveOptions(const QVariantList& value) {
    if (m_activeOptions == value) {
        return false;
    }

    m_activeOptions = value;
    m_activeOptionSet.clear();
    for (const QVariant& option : m_activeOptions) {
        bool ok = false;
        const int optionValue = option.toInt(&ok);
        if (ok) {
            m_activeOptionSet.insert(optionValue);
        }
    }
    emit activeOptionsChanged();
    return true;
}



