#pragma once

#include "BydaoNative.h"
#include <QLibrary>
#include <QSet>        // +++ ДОБАВИТЬ +++
#include <QHash>       // +++ ДОБАВИТЬ (если ещё нет) +++

namespace BydaoScript {

// Информация о модуле (для парсера)
class BydaoModuleInfo {
public:
    virtual ~BydaoModuleInfo() = default;

    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual bool hasMember(const QString& name) const = 0;
    virtual bool isMethod(const QString& name) const = 0;
    virtual int methodArgCount(const QString& name) const = 0;
    virtual QStringList methodArgs(const QString& name) const = 0;
    virtual QStringList members() const = 0;
};

// Реализация info по умолчанию
class BydaoModuleInfoImpl : public BydaoModuleInfo {
public:
    QString m_name;
    QString m_version;
    QSet<QString> m_properties;                    // теперь компилируется
    QHash<QString, QStringList> m_methods;

    QString name() const override { return m_name; }
    QString version() const override { return m_version; }

    bool hasMember(const QString& name) const override {
        return m_properties.contains(name) || m_methods.contains(name);
    }

    bool isMethod(const QString& name) const override {
        return m_methods.contains(name);
    }

    int methodArgCount(const QString& name) const override {
        return m_methods.value(name).size();
    }

    QStringList methodArgs(const QString& name) const override {
        return m_methods.value(name);
    }

    QStringList members() const override {
        QStringList result;
        result.append(m_properties.values());
        result.append(m_methods.keys());
        return result;
    }
};

// Базовый класс модуля
class BydaoModule : public BydaoNative {
    Q_OBJECT

public:
    explicit BydaoModule(QObject* parent = nullptr);
    virtual ~BydaoModule();

    virtual QString name() const = 0;
    virtual QString version() const = 0;

    // Мета-информация для парсера
    virtual BydaoModuleInfo* info() const = 0;

    // Жизненный цикл
    virtual bool initialize();
    virtual bool shutdown();

protected:

    virtual void release() {
    }

};

// Менеджер модулей
class BydaoModuleManager {
public:
    static BydaoModuleManager& instance();

    BydaoModule* loadModule(const QString& name, QString* error = nullptr);
    BydaoModuleInfo* loadModuleInfo(const QString& name, QString* error = nullptr);

    void addModulePath(const QString& path);
    void unloadAllModules();

private:
    BydaoModuleManager();
    ~BydaoModuleManager();

    QString findModuleFile(const QString& name);

    QHash<QString, BydaoModule*> m_modules;
    QHash<QString, QLibrary*> m_libraries;
    QHash<QString, BydaoModuleInfo*> m_infoCache;
    QStringList m_modulePaths;
};

// Макросы для экспорта
#ifdef Q_OS_WIN
#define MODULE_EXPORT __declspec(dllexport)
#else
#define MODULE_EXPORT __attribute__((visibility("default")))
#endif

#define BYDAO_MODULE(ModuleClass) \
extern "C" { \
    MODULE_EXPORT BydaoScript::BydaoModule* createModule() { \
        return new ModuleClass(); \
} \
    MODULE_EXPORT void destroyModule(BydaoScript::BydaoModule* module) { \
        delete module; \
} \
}

} // namespace BydaoScript
