// Copyright 2026 Oleh Horshkov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <QLibrary>
#include <QSet>
#include <QHash>

#include "BydaoNative.h"
#include "BydaoMetaData.h"

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

    virtual MetaData* metaData() { return nullptr; };

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
    MetaData* loadMetaData(const QString& name, QString* error = nullptr);

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

    QMap< QString, MetaData*>   m_metaData;
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
