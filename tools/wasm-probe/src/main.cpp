#include "ProbeState.h"

#include <QGuiApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>

extern "C" const char* rhythmGameWasmProbeInputDigest();
extern "C" const char* rhythmGameWasmProbeDependencyArchiveMarker();

int main(int argc, char* argv[])
{
    qInfo().noquote() << rhythmGameWasmProbeInputDigest();
    qInfo().noquote() << rhythmGameWasmProbeDependencyArchiveMarker();
    auto* app = new QGuiApplication{argc, argv};
    auto* state = new ProbeState{app};
    auto* engine = new QQmlApplicationEngine{app};
    engine->rootContext()->setContextProperty("probeState", state);
    engine->loadFromModule("RhythmGame.WasmProbe", "Main");
    if (engine->rootObjects().isEmpty()) {
        return 1;
    }

#ifdef __EMSCRIPTEN__
    return 0;
#else
    return app->exec();
#endif
}
