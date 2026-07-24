#include "ProbeState.h"

#include "ExceptionBoundary.h"

#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QWebSocket>
#include <QtConcurrent>

#include <stdexcept>
#include <string_view>

ProbeState::ProbeState(QObject* parent)
    : QObject{parent}
    , m_network{new QNetworkAccessManager{this}}
    , m_webSocket{new QWebSocket{QString{}, QWebSocketProtocol::VersionLatest,
                                this}}
{
    try {
        static_cast<void>(crossStaticLibraryBoundary());
    } catch (const std::runtime_error& error) {
        m_exceptionPassed =
            std::string_view{error.what()} == "wasm-native-exception";
        emit exceptionPassedChanged();
    }

    auto* watcher = new QFutureWatcher<int>{this};
    connect(watcher, &QFutureWatcher<int>::finished, this, [this, watcher] {
        m_threadPassed = watcher->result() == 42;
        emit threadPassedChanged();
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([] { return 42; }));
}

bool ProbeState::exceptionPassed() const
{
    return m_exceptionPassed;
}

bool ProbeState::threadPassed() const
{
    return m_threadPassed;
}
