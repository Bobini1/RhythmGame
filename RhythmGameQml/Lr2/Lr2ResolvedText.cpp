#include "Lr2ResolvedText.h"

Lr2ResolvedText::Lr2ResolvedText(QObject* parent) : QObject(parent) {}

Lr2ResolvedText::~Lr2ResolvedText() {
    unregisterTextId();
}

Lr2ResolvedTextRegistry* Lr2ResolvedText::registry() const {
    return m_registry;
}

void Lr2ResolvedText::setRegistry(Lr2ResolvedTextRegistry* value) {
    if (m_registry == value) {
        return;
    }

    unregisterTextId();
    if (m_registry) {
        QObject::disconnect(m_registryTextConnection);
        QObject::disconnect(m_registryDestroyedConnection);
    }
    m_registry = value;
    reconnectRegistry();
    registerTextId();
    syncFallbackText();
    emit registryChanged();
}

int Lr2ResolvedText::sourceTextId() const {
    return m_sourceTextId;
}

void Lr2ResolvedText::setSourceTextId(int value) {
    if (m_sourceTextId == value) {
        return;
    }

    unregisterTextId();
    m_sourceTextId = value;
    registerTextId();
    syncFallbackText();
    emit sourceTextIdChanged();
}

bool Lr2ResolvedText::isSearchText() const {
    return m_searchText;
}

void Lr2ResolvedText::setSearchText(bool value) {
    if (m_searchText == value) {
        return;
    }

    m_searchText = value;
    updateText();
    emit searchTextChanged();
}

QString Lr2ResolvedText::editingText() const {
    return m_editingText;
}

void Lr2ResolvedText::setEditingText(const QString& value) {
    if (m_editingText == value) {
        return;
    }

    m_editingText = value;
    updateText();
    emit editingTextChanged();
}

QString Lr2ResolvedText::text() const {
    return m_text;
}

void Lr2ResolvedText::registerTextId() {
    if (!m_registry || m_sourceTextId < 0 || m_registeredTextId == m_sourceTextId) {
        return;
    }

    m_registry->retainTextId(m_sourceTextId);
    m_registeredTextId = m_sourceTextId;
}

void Lr2ResolvedText::unregisterTextId() {
    if (!m_registry || m_registeredTextId < 0) {
        m_registeredTextId = -1;
        return;
    }

    m_registry->releaseTextId(m_registeredTextId);
    m_registeredTextId = -1;
}

void Lr2ResolvedText::reconnectRegistry() {
    if (!m_registry) {
        m_fallbackText.clear();
        updateText();
        return;
    }

    m_registryTextConnection = QObject::connect(
        m_registry,
        &Lr2ResolvedTextRegistry::textChanged,
        this,
        [this](int sourceTextId, const QString& text) {
            if (sourceTextId != m_sourceTextId || m_fallbackText == text) {
                return;
            }
            m_fallbackText = text;
            updateText();
        });
    m_registryDestroyedConnection = QObject::connect(
        m_registry,
        &QObject::destroyed,
        this,
        [this]() {
            m_registry = nullptr;
            m_registeredTextId = -1;
            m_fallbackText.clear();
            updateText();
            emit registryChanged();
        });
}

void Lr2ResolvedText::syncFallbackText() {
    const QString nextText = m_registry && m_sourceTextId >= 0
        ? m_registry->textFor(m_sourceTextId)
        : QString();
    if (m_fallbackText != nextText) {
        m_fallbackText = nextText;
    }
    updateText();
}

void Lr2ResolvedText::updateText() {
    const QString nextText = m_searchText && !m_editingText.isEmpty()
        ? m_editingText
        : m_fallbackText;
    if (m_text == nextText) {
        return;
    }

    m_text = nextText;
    emit textChanged();
}
