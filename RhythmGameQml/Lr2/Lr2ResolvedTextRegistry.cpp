#include "Lr2ResolvedTextRegistry.h"

#include <algorithm>

Lr2ResolvedTextRegistry::Lr2ResolvedTextRegistry(QObject* parent) : QObject(parent) {}

QList<int> Lr2ResolvedTextRegistry::activeTextIds() const {
    return m_activeTextIds;
}

int Lr2ResolvedTextRegistry::activeTextIdCount() const {
    return m_activeTextIds.size();
}

int Lr2ResolvedTextRegistry::activeTextIdRevision() const {
    return m_activeTextIdRevision;
}

QString Lr2ResolvedTextRegistry::textFor(int sourceTextId) const {
    return m_texts.value(sourceTextId);
}

void Lr2ResolvedTextRegistry::retainTextId(int sourceTextId) {
    if (sourceTextId < 0) {
        return;
    }

    const int previousCount = m_refCounts.value(sourceTextId, 0);
    m_refCounts.insert(sourceTextId, previousCount + 1);
    if (previousCount == 0) {
        appendActiveTextId(sourceTextId);
    }
}

void Lr2ResolvedTextRegistry::releaseTextId(int sourceTextId) {
    if (sourceTextId < 0) {
        return;
    }

    const int previousCount = m_refCounts.value(sourceTextId, 0);
    if (previousCount <= 1) {
        m_refCounts.remove(sourceTextId);
        removeActiveTextId(sourceTextId);
        return;
    }
    m_refCounts.insert(sourceTextId, previousCount - 1);
}

void Lr2ResolvedTextRegistry::setText(int sourceTextId, const QString& text) {
    if (sourceTextId < 0 || m_texts.value(sourceTextId) == text) {
        return;
    }

    m_texts.insert(sourceTextId, text);
    emit textChanged(sourceTextId, text);
}

int Lr2ResolvedTextRegistry::activeTextIdAt(int index) const {
    return index >= 0 && index < m_activeTextIds.size()
        ? m_activeTextIds.at(index)
        : -1;
}

bool Lr2ResolvedTextRegistry::isTextIdActive(int sourceTextId) const {
    return sourceTextId >= 0 && m_refCounts.value(sourceTextId, 0) > 0;
}

void Lr2ResolvedTextRegistry::appendActiveTextId(int sourceTextId) {
    m_activeTextIds.append(sourceTextId);
    std::sort(m_activeTextIds.begin(), m_activeTextIds.end());
    ++m_activeTextIdRevision;
    emit activeTextIdsChanged();
}

void Lr2ResolvedTextRegistry::removeActiveTextId(int sourceTextId) {
    if (!m_activeTextIds.removeOne(sourceTextId)) {
        return;
    }
    ++m_activeTextIdRevision;
    emit activeTextIdsChanged();
}
