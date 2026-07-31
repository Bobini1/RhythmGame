#pragma once

#include <QObject>
#include <QtTypes>

#include <functional>

namespace arena {

class ArenaScheduler : public QObject
{
  public:
    using TaskId = quint64;
    static constexpr TaskId InvalidTaskId = 0;

    using QObject::QObject;
    ~ArenaScheduler() override = default;

    [[nodiscard]] virtual auto monotonicNowMs() const -> qint64 = 0;
    [[nodiscard]] virtual auto scheduleOnce(qint64 delayMs,
                                            QObject* context,
                                            std::function<void()> callback)
      -> TaskId = 0;
    virtual void cancel(TaskId taskId) = 0;
};

} // namespace arena
