#pragma once

#include "ArenaScheduler.h"

#include <QElapsedTimer>
#include <QHash>
#include <QPointer>

class QTimer;

namespace arena {

class QtArenaScheduler final : public ArenaScheduler
{
  public:
    explicit QtArenaScheduler(QObject* parent = nullptr);
    ~QtArenaScheduler() override;

    [[nodiscard]] auto monotonicNowMs() const -> qint64 override;
    [[nodiscard]] auto scheduleOnce(qint64 delayMs,
                                    QObject* context,
                                    std::function<void()> callback)
      -> TaskId override;
    void cancel(TaskId taskId) override;

  private:
    QElapsedTimer m_clock;
    QHash<TaskId, QPointer<QTimer>> m_timers;
    TaskId m_nextTaskId{ 1 };
};

} // namespace arena
