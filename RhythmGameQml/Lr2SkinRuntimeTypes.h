#pragma once

#include <QSet>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVector>

namespace lr2skin::runtime {

enum ClockMode {
    ManualClock = 0,
    RenderClock = 1,
    SelectSourceClock = 2,
    BarClock = 3,
    GlobalClock = 4,
    SelectLiveClock = 5,
    SelectInfoClock = 6,
};

enum SpriteStateOverrideKind {
    NoSpriteStateOverride = 0,
    SelectScrollSpriteStateOverride = 1,
    GameplayProgressSpriteStateOverride = 2,
    GameplayLaneCoverSpriteStateOverride = 3,
    NumberRefSpriteStateOverride = 4,
    GenericSliderSpriteStateOverride = 5,
};

struct Dst {
    bool valid = false;
    int time = 0;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int acc = 0;
    int a = 255;
    int r = 255;
    int g = 255;
    int b = 255;
    int blend = 0;
    int filter = 0;
    int angle = 0;
    int center = 0;
    int sortId = 0;
    int loop = 0;
    int timer = 0;
    int op1 = 0;
    int op2 = 0;
    int op3 = 0;
    int op4 = 0;
    QVector<int> offsets;
};

struct Source {
    bool valid = false;
    int divX = 1;
    int divY = 1;
    int cycle = 0;
    int timer = 0;
    int resultChartType = 0;
    bool button = false;
    int buttonId = 0;
    bool onMouse = false;
    bool mouseCursor = false;
    bool slider = false;
    int sliderDirection = 0;
    int sliderRange = 0;
    int sliderType = 0;
    int sliderDisabled = 0;
    bool sliderRefNumber = false;
    int side = 0;
    bool hasKind = false;
    int kind = 0;
};

struct State {
    bool valid = false;
    qreal x = 0;
    qreal y = 0;
    qreal w = 0;
    qreal h = 0;
    qreal a = 255;
    qreal r = 255;
    qreal g = 255;
    qreal b = 255;
    qreal angle = 0;
    int center = 0;
    qreal sortId = 0;
    int blend = 0;
    int filter = 0;
    int op1 = 0;
    int op2 = 0;
    int op3 = 0;
    int op4 = 0;
};

struct DstAnalysis {
    bool canUseStaticState = false;
    bool usesActiveOptions = false;
    bool usesDynamicTimer = false;
    bool loopsContinuously = false;
    int scratchRotationSide = 0;
    int firstTimer = 0;
    int firstSortId = 0;
};

QVariantList readVariantList(const QVariant& value);
QVector<int> readOffsets(const QVariant& value);
bool readDst(const QVariant& value, Dst& dst);
QVector<Dst> readDsts(const QVariantList& dsts);
QVector<Dst> readDsts(const QVariant& dsts);
bool readSource(const QVariant& value, Source& source);

DstAnalysis analyzeDsts(const QVector<Dst>& dsts);
int animationLimitFor(const QVector<Dst>& dsts);
QSet<int> activeOptionSet(const QVariant& activeOptions);
QVariantList activeOptionsForDsts(const Dst& firstDst, const QVariant& activeOptions);
bool allOpsMatch(const Dst& dst, const QSet<int>& activeOptions);
qreal timerValue(const QVariant& timers, int timerIdx);

State currentState(const QVector<Dst>& dsts,
                   int globalTime,
                   qreal timerFire,
                   const QSet<int>& activeOptions);
State copyDstAsState(const Dst& dst, const Dst& controlDst);
QVariant stateToVariant(const State& state);
bool sameState(const State& lhs, const State& rhs);
qreal applyAccel(qreal progress, int accType);

bool sourceCyclesContinuously(const Source& source);
bool sourceUsesDynamicTimer(const Source& source);
bool isSelectBarElement(int type, const Source& source);
qreal selectBarElementLayer(int type, const Source& source);
int staticNoteElementSortId(const QVariantList& noteDsts);
int firstSortId(const QVariantList& dsts);
int firstSortId(const QVariant& dsts);
SpriteStateOverrideKind spriteStateOverrideKind(const QString& screenKey,
                                                bool gameplayScreen,
                                                const Source& source);

State translatedSliderState(State state, qreal position, int range, int direction);
QVariant sliderTrackState(const Source& source, const State& baseState);
qreal sliderPositionFromPointer(const Source& source,
                                const QVariant& track,
                                qreal pointerX,
                                qreal pointerY);
bool rectContains(const QVariant& state, qreal skinX, qreal skinY);

} // namespace lr2skin::runtime
