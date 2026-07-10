#include "QtArenaScheduler.h"

#include <QTimer>

#include <limits>
#include <utility>

namespace arena {

QtArenaScheduler::QtArenaScheduler(QObject* parent)
  : ArenaScheduler(parent)
{
    m_clock.start();
}

QtArenaScheduler::~QtArenaScheduler()
{
    const auto taskIds = m_timers.keys();
    for (const auto taskId : taskIds) {
        cancel(taskId);
    }
}

auto
QtArenaScheduler::monotonicNowMs() const -> qint64
{
    return m_clock.elapsed();
}

auto
QtArenaScheduler::scheduleOnce(qint64 delayMs,
                               QObject* context,
                               std::function<void()> callback) -> TaskId
{
    if (delayMs < 0 || context == nullptr || !callback ||
        delayMs > (std::numeric_limits<int>::max)()) {
        return InvalidTaskId;
    }
    auto taskId = m_nextTaskId++;
    if (taskId == InvalidTaskId) {
        taskId = m_nextTaskId++;
    }
    auto* timer = new QTimer(this);
    timer->setSingleShot(true);
    m_timers.insert(taskId, timer);
    const QPointer<QObject> guardedContext(context);
    connect(
      timer,
      &QTimer::timeout,
      this,
      [this, taskId, guardedContext, callback = std::move(callback)]() mutable {
          const auto timer = m_timers.take(taskId);
          if (timer) {
              timer->deleteLater();
          }
          if (guardedContext) {
              callback();
          }
      });
    timer->start(static_cast<int>(delayMs));
    return taskId;
}

void
QtArenaScheduler::cancel(TaskId taskId)
{
    if (taskId == InvalidTaskId) {
        return;
    }
    const auto timer = m_timers.take(taskId);
    if (!timer) {
        return;
    }
    timer->stop();
    timer->deleteLater();
}

} // namespace arena
