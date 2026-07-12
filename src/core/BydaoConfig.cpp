// BydaoConfig.cpp
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <functional>

#include "BydaoScript/BydaoConfig.h"

namespace BydaoScript {

BydaoConfig::BydaoConfig() {
}

BydaoConfig::~BydaoConfig() {
}

bool BydaoConfig::load(const QString& fileName, FileType type /* = TYPE_AUTO */) {
    m_loaded = false;
    m_lastError.clear();
    m_params.clear();

    QString filePath = findFile(fileName);
    if (filePath.isEmpty()) {
        m_lastError = QString("Config file not found: %1").arg(fileName);
        return false;
    }

    m_fileName = filePath;

    bool ok = false;
    if ( type == TYPE_AUTO ) {
        m_type = detectType(filePath);

        if (m_type == TYPE_INI) {
            ok = parseIni(filePath);
        }
        else if (m_type == TYPE_JSON) {
            ok = parseJson(filePath);
        }
        else {
            m_lastError = QString("Unknown type of config file: %1").arg(filePath);
            return false;
        }
    }
    else if ( type == TYPE_INI ) {
        m_type = TYPE_INI;
        ok = parseIni(filePath);
    }
    else if ( type == TYPE_JSON ) {
        m_type = TYPE_JSON;
        ok = parseJson(filePath);
    }
    else {
        m_lastError = QString("Unknown type of config file: %1").arg( filePath );
        return false;
    }

    m_loaded = ok;
    return ok;
}

BydaoConfig::FileType BydaoConfig::detectType(const QString& fileName) const {
    QString ext = QFileInfo(fileName).suffix().toLower();
    if (ext == "ini" || ext == "conf" || ext == "cfg") {
        return TYPE_INI;
    }
    else if (ext == "json") {
        return TYPE_JSON;
    }
    return TYPE_AUTO;
}

QString BydaoConfig::findFile(const QString& fileName) const {
    if (QFile::exists(fileName)) {
        return QFileInfo(fileName).absoluteFilePath();
    }

    QString confPath = QDir("conf").absoluteFilePath(fileName);
    if (QFile::exists(confPath)) {
        return QFileInfo(confPath).absoluteFilePath();
    }

    QString configPath = QDir("config").absoluteFilePath(fileName);
    if (QFile::exists(configPath)) {
        return QFileInfo(configPath).absoluteFilePath();
    }

    return QString();
}

bool BydaoConfig::parseIni(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QString("Cannot open INI file: %1").arg(filePath);
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    QString currentSection = "main";

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        // Пропускаем пустые строки и комментарии
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#')) {
            continue;
        }

        // Секция [section]
        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.length() - 2).trimmed();
            continue;
        }

        // Параметр key = value
        int eqPos = line.indexOf('=');
        if (eqPos == -1) continue;

        QString key = line.left(eqPos).trimmed();
        QString value = line.mid(eqPos + 1).trimmed();

        if (key.isEmpty()) continue;

        // Удаляем inline-комментарий
        static QRegularExpression inlineComment("[;\\#].*$");
        value.remove(inlineComment);
        value = value.trimmed();

        // Снимаем кавычки
        if (value.length() >= 2) {
            if ((value.startsWith('"') && value.endsWith('"')) ||
                (value.startsWith('\'') && value.endsWith('\''))) {
                value = value.mid(1, value.length() - 2);
            }
        }

        m_params[currentSection + "/" + key] = value;
    }

    file.close();
    return true;
}

bool BydaoConfig::parseJson(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QString("Cannot open JSON file: %1").arg(filePath);
        return false;
    }

    QByteArray rawData = file.readAll();
    file.close();

    // Очищаем JSON от комментариев и trailing commas
    QString jsonStr = QString::fromUtf8(rawData);

    // Удаляем однострочные комментарии //
    jsonStr.replace(QRegularExpression("//[^\n]*"), "");

    // Удаляем многострочные комментарии /* */
    jsonStr.replace(QRegularExpression("/\\*.*?\\*/", QRegularExpression::DotMatchesEverythingOption), "");

    // Удаляем trailing commas перед ] и }
    jsonStr.replace(QRegularExpression(",(\\s*[}\\]])"), "\\1");

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = QString("JSON parse error: %1").arg(parseError.errorString());
        return false;
    }

    // Рекурсивно обходим JSON
    std::function<void(const QJsonObject&, const QString&)> flattenJson;
    flattenJson = [this, &flattenJson](const QJsonObject& obj, const QString& prefix) {
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            QString key = it.key();
            QJsonValue value = it.value();
            QString fullPath = prefix.isEmpty() ? key : prefix + "/" + key;

            if (value.isObject()) {
                flattenJson(value.toObject(), fullPath);
            } else if (value.isArray()) {
                QJsonDocument arrDoc(value.toArray());
                m_params[fullPath] = QString::fromUtf8(arrDoc.toJson(QJsonDocument::Compact));
            } else {
                m_params[fullPath] = value.toVariant().toString();
            }
        }
    };

    if (doc.isObject()) {
        flattenJson(doc.object(), "main");
    } else {
        m_lastError = "JSON root is not an object";
        return false;
    }

    return true;
}

QString BydaoConfig::resolvePath(const QString& path, const QString& defaultValue) const {
    // Прямой поиск
    if (m_params.contains(path)) {
        return m_params[path];
    }

    // Если путь без секции, пробуем "main/путь"
    if (!path.contains('/')) {
        QString mainPath = "main/" + path;
        if (m_params.contains(mainPath)) {
            return m_params[mainPath];
        }
    }

    return defaultValue;
}

QString BydaoConfig::get(const QString& path, const QString& defaultValue) const {
    if (!m_loaded) return defaultValue;
    return resolvePath(path, defaultValue);
}

int BydaoConfig::getInt(const QString& path, int defaultValue) const {
    QString val = get(path);
    if (val.isEmpty()) return defaultValue;

    bool ok;
    int result = val.toInt(&ok);
    return ok ? result : defaultValue;
}

double BydaoConfig::getReal(const QString& path, double defaultValue) const {
    QString val = get(path);
    if (val.isEmpty()) return defaultValue;

    bool ok;
    double result = val.toDouble(&ok);
    return ok ? result : defaultValue;
}

bool BydaoConfig::getBool(const QString& path, bool defaultValue) const {
    QString val = get(path).toLower();
    if (val.isEmpty()) return defaultValue;

    if (val == "true" || val == "1" || val == "yes" || val == "on") return true;
    if (val == "false" || val == "0" || val == "no" || val == "off") return false;

    return defaultValue;
}

QStringList BydaoConfig::getArray(const QString& path, const QStringList& defaultValue) const {
    QString val = get(path);
    if (val.isEmpty()) return defaultValue;

    // Пробуем распарсить как JSON массив
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError && doc.isArray()) {
        QStringList result;
        for (const QJsonValue& v : doc.array()) {
            result << v.toString();
        }
        return result;
    }

    // Иначе разбиваем по запятой (для INI)
    return val.split(',', Qt::SkipEmptyParts);
}

QHash<QString, QString> BydaoConfig::getDict(const QString& path, const QHash<QString, QString>& defaultValue) const {
    QHash<QString, QString> result;

    QString prefix = path + "/";
    for (auto it = m_params.begin(); it != m_params.end(); ++it) {
        if (it.key().startsWith(prefix)) {
            QString key = it.key().mid(prefix.length());
            if (!key.contains('/')) {
                result[key] = it.value();
            }
        }
    }

    return result.isEmpty() ? defaultValue : result;
}

bool BydaoConfig::contains(const QString& path) const {
    if (!m_loaded) return false;

    if (m_params.contains(path)) return true;
    if (!path.contains('/') && m_params.contains("main/" + path)) return true;

    return false;
}

} // namespace BydaoScript
