// BydaoLogger.cpp
#include "BydaoScript/BydaoLogger.h"
#include <QDir>
#include <QDateTime>
#include <QDebug>

namespace BydaoScript {

BydaoLogger& BydaoLogger::instance() {
    static BydaoLogger logger;
    return logger;
}

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
    QString timeStr = QTime::currentTime().toString("HH:mm:ss.zzz");
    QString levelStr = levelToString(level);

    QString result;
    result += timeStr;
    result += " " + levelStr;

    if (!source.isEmpty()) {
        result += " [" + source + "]";
    }

    result += " " + message;
    return result;
}

QString BydaoLogger::levelToString(Level level) const {
    switch (level) {
    case DEBUG: return "DEBUG";
    case INFO:  return "INFO";
    case ERROR: return "ERROR";
    default:    return "UNKNOWN";
    }
}

void BydaoLogger::debug(const QString& message, const QString& source) {
    if (!m_debugEnabled) return;
    log(DEBUG, message, source);
}

void BydaoLogger::info(const QString& message, const QString& source) {
    log(INFO, message, source);
}

void BydaoLogger::error(const QString& message, const QString& source) {
    log(ERROR, message, source);
}

void BydaoLogger::log(Level level, const QString& message, const QString& source) {
    if (level == DEBUG && !m_debugEnabled) return;
    write(level, message, source);
}

} // namespace BydaoScript
