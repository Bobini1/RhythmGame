//
// Created by PC on 14/08/2025.
//

#ifndef RHYTHMGAME_QTSINK_H
#define RHYTHMGAME_QTSINK_H

#include <QObject>
#include <mutex>
#include <spdlog/logger.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/details/synchronous_factory.h>

namespace support {

template <typename Mutex>
class QtSink : public spdlog::sinks::base_sink<Mutex> {
public:
    QtSink(QObject *qt_object, std::string meta_method)
        : qt_object_(qt_object),
          meta_method_(std::move(meta_method)) {
        if (!qt_object_) {
            spdlog::throw_spdlog_ex("qt_sink: qt_object is null");
        }
    }

    ~QtSink() override { QtSink::flush_(); }

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        const auto str = spdlog::string_view_t(formatted.data(), formatted.size());
        QMetaObject::invokeMethod(
            qt_object_, meta_method_.c_str(), Qt::AutoConnection,
            Q_ARG(QString, QString::fromUtf8(str.data(), static_cast<int>(str.size())).trimmed()));
    }

    void flush_() override {}

private:
    QObject *qt_object_ = nullptr;
    std::string meta_method_;
};

using QtSinkMt = QtSink<std::mutex>;

template <typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<spdlog::logger> qtLoggerMt(const std::string &logger_name,
                                            QObject *qt_object,
                                            const std::string &meta_method) {
    return Factory::template create<QtSinkMt>(logger_name, qt_object, meta_method);
}

} // namespace support

#endif // RHYTHMGAME_QTSINK_H
