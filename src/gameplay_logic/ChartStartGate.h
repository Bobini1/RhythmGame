#ifndef RHYTHMGAME_CHARTSTARTGATE_H
#define RHYTHMGAME_CHARTSTARTGATE_H

namespace gameplay_logic {

/**
 * Coordinates an optional pre-game hold with ChartRunner's existing
 * start-before-ready latch. It contains no Arena-specific state.
 */
class ChartStartGate
{
    bool held{};
    bool pendingStart{};

  public:
    void hold() { held = true; }

    [[nodiscard]] auto requestStart(bool ready) -> bool
    {
        pendingStart = true;
        if (held || !ready) {
            return false;
        }
        pendingStart = false;
        return true;
    }

    [[nodiscard]] auto onReady() -> bool
    {
        if (held || !pendingStart) {
            return false;
        }
        pendingStart = false;
        return true;
    }

    [[nodiscard]] auto release(bool ready) -> bool
    {
        held = false;
        if (!ready || !pendingStart) {
            return false;
        }
        pendingStart = false;
        return true;
    }

    void reset()
    {
        held = false;
        pendingStart = false;
    }

    [[nodiscard]] auto isHeld() const -> bool { return held; }
    [[nodiscard]] auto hasPendingStart() const -> bool { return pendingStart; }
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHARTSTARTGATE_H
