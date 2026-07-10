#pragma once

#include "arena/ArenaScheduler.h"

#include <QPointer>
#include <QVector>

#include <algorithm>
#include <limits>
#include <utility>

namespace arena::test {

class FakeArenaScheduler final : public ArenaScheduler
{
  public:
    using ArenaScheduler::ArenaScheduler;

    [[nodiscard]] auto monotonicNowMs() const -> qint64 override
    {
        return m_nowMs;
    }

    [[nodiscard]] auto scheduleOnce(qint64 delayMs,
                                    QObject* context,
                                    std::function<void()> callback)
      -> TaskId override
    {
        if (delayMs < 0 || context == nullptr || !callback ||
            delayMs > (std::numeric_limits<qint64>::max)() - m_nowMs) {
            return InvalidTaskId;
        }
        const auto id = m_nextId++;
        m_tasks.push_back(
          { id, m_nowMs + delayMs, context, std::move(callback) });
        return id;
    }

    void cancel(TaskId taskId) override
    {
        if (taskId == InvalidTaskId) {
            return;
        }
        const auto firstRemoved = std::remove_if(
          m_tasks.begin(), m_tasks.end(), [taskId](const Task& task) {
              return task.id == taskId;
          });
        m_tasks.erase(firstRemoved, m_tasks.end());
    }

    void advanceBy(qint64 milliseconds)
    {
        if (milliseconds >= 0 &&
            milliseconds <= (std::numeric_limits<qint64>::max)() - m_nowMs) {
            advanceTo(m_nowMs + milliseconds);
        }
    }

    void advanceTo(qint64 targetMs)
    {
        if (targetMs < m_nowMs) {
            return;
        }
        while (true) {
            auto next =
              std::min_element(m_tasks.begin(),
                               m_tasks.end(),
                               [](const Task& lhs, const Task& rhs) {
                                   return std::pair{ lhs.dueMs, lhs.id } <
                                          std::pair{ rhs.dueMs, rhs.id };
                               });
            if (next == m_tasks.end() || next->dueMs > targetMs) {
                break;
            }
            auto task = std::move(*next);
            m_tasks.erase(next);
            m_nowMs = task.dueMs;
            if (task.context) {
                task.callback();
            }
        }
        m_nowMs = targetMs;
    }

    [[nodiscard]] auto pendingCount() const -> qsizetype
    {
        return m_tasks.size();
    }

  private:
    struct Task
    {
        TaskId id;
        qint64 dueMs;
        QPointer<QObject> context;
        std::function<void()> callback;
    };

    qint64 m_nowMs{};
    TaskId m_nextId{ 1 };
    QVector<Task> m_tasks{};
};

} // namespace arena::test
