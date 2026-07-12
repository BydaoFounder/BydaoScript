// BydaoConfig.h
#ifndef BYDAOCONFIG_H
#define BYDAOCONFIG_H

#include <QHash>
#include <QString>
#include <QStringList>

namespace BydaoScript {

class BydaoConfig {
public:
    BydaoConfig();
    ~BydaoConfig();

    // Типы файлов
    enum FileType { TYPE_AUTO, TYPE_INI, TYPE_JSON };

    // Загрузка файла (ищет в ".", "./conf")
    bool load(const QString& fileName, FileType type = TYPE_AUTO);

    // Получение значений
    QString get(const QString& path, const QString& defaultValue = "") const;
    int getInt(const QString& path, int defaultValue = 0) const;
    double getReal(const QString& path, double defaultValue = 0.0) const;
    bool getBool(const QString& path, bool defaultValue = false) const;
    QStringList getArray(const QString& path, const QStringList& defaultValue = {}) const;
    QHash<QString, QString> getDict(const QString& path, const QHash<QString, QString>& defaultValue = {}) const;

    // Проверка существования параметра
    bool contains(const QString& path) const;

    // Ошибки
    QString lastError() const { return m_lastError; }
    bool isValid() const { return m_loaded; }

private:
    FileType detectType(const QString& fileName) const;
    QString findFile(const QString& fileName) const;
    bool parseIni(const QString& filePath);
    bool parseJson(const QString& filePath);

    QString resolvePath(const QString& path, const QString& defaultValue = "") const;

    QHash<QString, QString> m_params;
    bool m_loaded = false;
    QString m_lastError;
    QString m_fileName;
    FileType m_type = TYPE_AUTO;
};

} // namespace BydaoScript

#endif // BYDAOCONFIG_H
