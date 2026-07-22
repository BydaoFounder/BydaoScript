// BydaoLogger.h
#ifndef BYDAOLOGGER_H
#define BYDAOLOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDate>
#include <QTime>

#include "BydaoRuntime.h"

namespace BydaoScript {

class BydaoLogger {
public:
    BydaoLogger() = default;
    ~BydaoLogger();

    bool init(const QString& logDir);

    void debug(const QString& message, const QString& source = "");
    void info(const QString& message, const QString& source = "");
    void error(const QString& message, const QString& source = "");

    // Универсальный метод
    enum Level { DEBUG, INFO, ERROR };
    void log(Level level, const QString& message, const QString& source = "");

    void    setRuntime( BydaoRuntime* runtime ) {
        m_runtime = runtime;
    }

    void setRequestId(uint16_t id) { m_requestId = id; }

private:
    BydaoLogger(const BydaoLogger&) = delete;
    BydaoLogger& operator=(const BydaoLogger&) = delete;

    void write(Level level, const QString& message, const QString& source);
    void openFile();
    QString formatMessage(Level level, const QString& message, const QString& source) const;
    QString levelToString(Level level) const;

    QString currentTimeMicro() const;

    QMutex m_mutex;
    QString m_logDir;
    QString m_filePath;
    QFile m_file;
    QTextStream m_stream;
    QDate m_currentDate;

    uint16_t m_requestId = 0;
    bool m_initialized = false;

    BydaoRuntime*   m_runtime = nullptr;
};

} // namespace BydaoScript

#endif // BYDAOLOGGER_H