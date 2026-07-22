// BydaoLogger.cpp
#include "BydaoScript/BydaoLogger.h"
#include <QDir>
#include <QDateTime>
#include <QDebug>

namespace BydaoScript {

BydaoLogger::~BydaoLogger() {
    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
}

bool BydaoLogger::init(const QString& logDir) {
    QMutexLocker locker(&m_mutex);

    if (m_initialized) return true;

    m_logDir = logDir;

    QDir dir;
    if (!dir.mkpath(m_logDir)) {
        qWarning() << "BydaoLogger: Cannot create log directory:" << m_logDir;
        return false;
    }

    m_currentDate = QDate::currentDate();
    openFile();

    m_initialized = true;
    return true;
}

void BydaoLogger::openFile() {
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }

    QString dateStr = m_currentDate.toString("yyyy-MM-dd");
    m_filePath = QString("%1/site_%2.log").arg(m_logDir, dateStr);

    m_file.setFileName(m_filePath);
    if (!m_file.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "BydaoLogger: Cannot open log file:" << m_filePath;
        return;
    }

    m_stream.setDevice(&m_file);
    m_stream.setEncoding(QStringConverter::Utf8);
}

void BydaoLogger::write(Level level, const QString& message, const QString& source) {
    QMutexLocker locker(&m_mutex);

    // Ротация при смене даты
    QDate today = QDate::currentDate();
    if (today != m_currentDate) {
        m_currentDate = today;
        openFile();
    }

    if (!m_file.isOpen()) return;

    QString line = formatMessage(level, message, source);
    m_stream << line << "\n";
    m_stream.flush();
}

QString BydaoLogger::formatMessage(Level level, const QString& message, const QString& source) const {

//    QString result = QTime::currentTime().toString("HH:mm:ss.zzzzzz");
    QString result = currentTimeMicro();

    if ( m_runtime ) {
        QString traceId = m_runtime->getTraceId();
        if ( ! traceId.isEmpty() ) {
            result += " " + traceId;
        }
    }

    result += " " + levelToString(level);

    if ( ! source.isEmpty() ) {
        result += " [" + source + "]";
    }

    result += " " + message;
    return result;
}

QString BydaoLogger::currentTimeMicro() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(
                    now.time_since_epoch()) % 1000000;

    return QDateTime::fromSecsSinceEpoch(time_t)
               .toString("HH:mm:ss") +
           QString(".%1").arg(usec.count(), 6, 10, QChar('0'));
}

QString BydaoLogger::levelToString(Level level) const {
    switch (level) {
    case DEBUG: return "DEBUG";
    case INFO:  return "INFO ";
    case ERROR: return "ERROR";
    default:    return "-----";
    }
}

void BydaoLogger::debug(const QString& message, const QString& source) {
    log(DEBUG, message, source);
}

void BydaoLogger::info(const QString& message, const QString& source) {
    log(INFO, message, source);
}

void BydaoLogger::error(const QString& message, const QString& source) {
    log(ERROR, message, source);
}

void BydaoLogger::log(Level level, const QString& message, const QString& source) {
    write(level, message, source);
}

} // namespace BydaoScript
