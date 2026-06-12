#include "Lr2ResolvedTextRegistry.h"

#include "Lr2ResolvedText.h"

#include <algorithm>
#include <utility>

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

void Lr2ResolvedTextRegistry::retainTextId(int sourceTextId, Lr2ResolvedText* listener) {
    if (sourceTextId < 0) {
        return;
    }

    addTextListener(sourceTextId, listener);
    const int previousCount = m_refCounts.value(sourceTextId, 0);
    m_refCounts.insert(sourceTextId, previousCount + 1);
    if (previousCount == 0) {
        appendActiveTextId(sourceTextId);
    }
}

void Lr2ResolvedTextRegistry::releaseTextId(int sourceTextId, Lr2ResolvedText* listener) {
    if (sourceTextId < 0) {
        return;
    }

    removeTextListener(sourceTextId, listener);
    const int previousCount = m_refCounts.value(sourceTextId, 0);
    if (previousCount <= 1) {
        m_refCounts.remove(sourceTextId);
        m_texts.remove(sourceTextId);
        removeActiveTextId(sourceTextId);
        return;
    }
    m_refCounts.insert(sourceTextId, previousCount - 1);
}

void Lr2ResolvedTextRegistry::setText(int sourceTextId, const QString& text) {
    if (sourceTextId < 0) {
        return;
    }

    auto it = m_texts.constFind(sourceTextId);
    if (it != m_texts.constEnd() && it.value() == text) {
        return;
    }

    m_texts.insert(sourceTextId, text);
    notifyTextListeners(sourceTextId, text);
}

int Lr2ResolvedTextRegistry::activeTextIdAt(int index) const {
    return index >= 0 && index < m_activeTextIds.size()
        ? m_activeTextIds.at(index)
        : -1;
}

bool Lr2ResolvedTextRegistry::isTextIdActive(int sourceTextId) const {
    return sourceTextId >= 0 && m_refCounts.value(sourceTextId, 0) > 0;
}

void Lr2ResolvedTextRegistry::queueAllTextRefresh() {
    m_queuedTextRefreshAll = true;
    m_queuedTextRefreshIds.clear();
}

void Lr2ResolvedTextRegistry::queueTextRefreshIds(const QList<int>& sourceTextIds) {
    if (m_queuedTextRefreshAll) {
        return;
    }

    for (int id : sourceTextIds) {
        if (id >= 0) {
            m_queuedTextRefreshIds.insert(id);
        }
    }
}

bool Lr2ResolvedTextRegistry::refreshQueuedTexts(const QJSValue& resolveText) {
    if (!resolveText.isCallable()) {
        return false;
    }

    const QList<int> ids = takeQueuedTextRefreshIds();
    if (ids.isEmpty()) {
        return false;
    }

    QJSValue callback = resolveText;
    bool refreshed = false;
    for (int id : ids) {
        QJSValue text = callback.call(QJSValueList{ QJSValue(id) });
        if (text.isError()) {
            continue;
        }
        setText(id, text.toString());
        refreshed = true;
    }
    return refreshed;
}

void Lr2ResolvedTextRegistry::appendActiveTextId(int sourceTextId) {
    const auto insertIt =
        std::lower_bound(m_activeTextIds.begin(), m_activeTextIds.end(), sourceTextId);
    m_activeTextIds.insert(insertIt, sourceTextId);
    ++m_activeTextIdRevision;
    emit activeTextIdsChanged();
}

void Lr2ResolvedTextRegistry::removeActiveTextId(int sourceTextId) {
    if (!m_activeTextIds.removeOne(sourceTextId)) {
        return;
    }
    m_textListeners.remove(sourceTextId);
    ++m_activeTextIdRevision;
    emit activeTextIdsChanged();
}

QList<int> Lr2ResolvedTextRegistry::takeQueuedTextRefreshIds() {
    if (m_queuedTextRefreshAll) {
        m_queuedTextRefreshAll = false;
        m_queuedTextRefreshIds.clear();
        return m_activeTextIds;
    }

    QList<int> ids;
    ids.reserve(m_queuedTextRefreshIds.size());
    for (int id : std::as_const(m_queuedTextRefreshIds)) {
        if (isTextIdActive(id)) {
            ids.append(id);
        }
    }
    m_queuedTextRefreshIds.clear();
    std::sort(ids.begin(), ids.end());
    return ids;
}

void Lr2ResolvedTextRegistry::addTextListener(int sourceTextId, Lr2ResolvedText* listener) {
    if (!listener) {
        return;
    }

    QList<QPointer<Lr2ResolvedText>>& listeners = m_textListeners[sourceTextId];
    for (const QPointer<Lr2ResolvedText>& existing : listeners) {
        if (existing == listener) {
            return;
        }
    }
    listeners.append(listener);
}

void Lr2ResolvedTextRegistry::removeTextListener(int sourceTextId, Lr2ResolvedText* listener) {
    if (!listener) {
        return;
    }

    auto it = m_textListeners.find(sourceTextId);
    if (it == m_textListeners.end()) {
        return;
    }
    auto& listeners = it.value();
    listeners.erase(std::remove_if(listeners.begin(),
                                   listeners.end(),
                                   [listener](const QPointer<Lr2ResolvedText>& existing) {
                                       return existing.isNull() || existing == listener;
                                   }),
                    listeners.end());
    if (it->isEmpty()) {
        m_textListeners.erase(it);
    }
}

void Lr2ResolvedTextRegistry::notifyTextListeners(int sourceTextId, const QString& text) {
    const QList<QPointer<Lr2ResolvedText>> listeners = m_textListeners.value(sourceTextId);
    for (const QPointer<Lr2ResolvedText>& listener : listeners) {
        if (listener) {
            listener->setRegistryFallbackText(text);
        }
    }
}
