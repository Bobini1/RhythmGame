#include "ProbeState.h"

#include <QByteArray>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStringList>
#include <QTimer>

#include <vector>

extern "C" const char *rhythmGameWasmProbeInputDigest();
extern "C" const char *rhythmGameWasmProbeDependencyArchiveMarker();

namespace
{
struct ProcessLifetimeArguments
{
    ProcessLifetimeArguments(int count, char *const values[])
        : argc(count)
    {
        storage.reserve(static_cast<std::size_t>(argc));
        pointers.reserve(static_cast<std::size_t>(argc) + 1);
        for (int index = 0; index < argc; ++index) {
            storage.emplace_back(values[index] != nullptr ? values[index] : "");
        }
        for (QByteArray &value : storage) {
            pointers.push_back(value.data());
        }
        pointers.push_back(nullptr);
    }

    int argc;
    std::vector<QByteArray> storage;
    std::vector<char *> pointers;

    [[nodiscard]] QStringList currentArguments() const
    {
        QStringList result;
        result.reserve(argc);
        for (int index = 0; index < argc; ++index) {
            result.append(QString::fromLocal8Bit(pointers[index]));
        }
        return result;
    }
};
}

int main(int argc, char *argv[])
{
    qInfo().noquote() << rhythmGameWasmProbeInputDigest();
    qInfo().noquote() << rhythmGameWasmProbeDependencyArchiveMarker();

    static ProcessLifetimeArguments processArguments{argc, argv};
    auto *app = new QGuiApplication{
        processArguments.argc,
        processArguments.pointers.data()};
    auto *state = new ProbeState{
        processArguments.currentArguments(),
        app};
    auto *engine = new QQmlApplicationEngine{app};
    engine->rootContext()->setContextProperty(
        QStringLiteral("probeState"),
        state);
    engine->loadFromModule("RhythmGame.WasmProbe", "Main");

    if (engine->rootObjects().size() != 1) {
        state->failStartup(
            u"qml-root-count",
            u"the QML module did not create exactly one root object");
        return 1;
    }
    auto *window =
        qobject_cast<QQuickWindow *>(engine->rootObjects().constFirst());
    if (window == nullptr) {
        state->failStartup(
            u"qml-root-type",
            u"the QML root is not a QQuickWindow");
        return 1;
    }
    state->attachWindow(window);

#ifdef __EMSCRIPTEN__
    QTimer::singleShot(0, state, [state] {
        // The callback owns the post-main-tick event and all async starts.
        state->postMainTick();
    });
    state->recordMainReturning(); // main-returning
    return 0;
#else
    return app->exec();
#endif
}
