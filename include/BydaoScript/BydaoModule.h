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

#include "BydaoObject.h"
#include "BydaoMetaData.h"

namespace BydaoScript {

// Базовый класс модуля
class BydaoModule : public BydaoObject {

public:
    explicit BydaoModule();
    virtual ~BydaoModule();

    virtual QString name() const = 0;
    virtual QString version() const = 0;

    virtual MetaData* metaData() { return nullptr; };

    // Получить список используемых мета-данных
    virtual UsedMetaDataList usedMetaData() { return UsedMetaDataList(); };

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
    void         unloadModule(const QString& name);

    void addModulePath(const QString& path);
    void unloadAllModules();

private:
    BydaoModuleManager();
    ~BydaoModuleManager();

    QString findModuleFile(const QString& name);

    QHash<QString, BydaoModule*> m_modules;
    QHash<QString, QLibrary*> m_libraries;
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
