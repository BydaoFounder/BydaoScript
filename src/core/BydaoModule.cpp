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
#include "BydaoScript/BydaoModule.h"
#include <QCoreApplication>
#include <QDir>

namespace BydaoScript {

// ========== BydaoModule ==========

BydaoModule::BydaoModule(QObject* parent)
    : BydaoNative(parent)
{}

BydaoModule::~BydaoModule() {
}

bool BydaoModule::initialize() { return true; }
bool BydaoModule::shutdown() { return true; }

// ========== BydaoModuleManager ==========

BydaoModuleManager& BydaoModuleManager::instance() {
    static BydaoModuleManager manager;
    return manager;
}

BydaoModuleManager::BydaoModuleManager() {
    addModulePath(".");
    addModulePath("./modules");
    addModulePath(QCoreApplication::applicationDirPath());
    addModulePath(QCoreApplication::applicationDirPath() + "/modules");
}

BydaoModuleManager::~BydaoModuleManager() {
//    qDebug() << "~BydaoModuleManager()";
    unloadAllModules();
}

void BydaoModuleManager::addModulePath(const QString& path) {
    QString normalized = QDir::cleanPath(path);
    if (!m_modulePaths.contains(normalized)) {
        m_modulePaths.prepend(normalized);
    }
}

QString BydaoModuleManager::findModuleFile(const QString& name) {
    // Поиск файла модуля
    QStringList extensions;
#ifdef Q_OS_WIN
    extensions << ".dll";
#elif Q_OS_MAC
    extensions << ".dylib" << ".so";
#else
    extensions << ".so";
#endif

    for (const QString& path : m_modulePaths) {
        for (const QString& ext : extensions) {
            QString fullPath = path + "/" + name + ext;
            if (QFile::exists(fullPath)) {
                return fullPath;
            }

            fullPath = path + "/lib" + name + ext;
            if (QFile::exists(fullPath)) {
                return fullPath;
            }
        }
    }
    return QString();
}

BydaoModule* BydaoModuleManager::loadModule(const QString& name, QString* error) {
    // Уже загружен?
    if (m_modules.contains(name)) {
        return m_modules[name];
    }

    QString path = findModuleFile(name);
    if (path.isEmpty()) {
        if (error) *error = "Module not found: " + name;
        return nullptr;
    }

    QLibrary* lib = new QLibrary(path);
    if (!lib->load()) {
        if (error) *error = lib->errorString();
        delete lib;
        return nullptr;
    }

    auto create = (BydaoModule* (*)())lib->resolve("createModule");
    if (!create) {
        if (error) *error = "No createModule()";
        lib->unload();
        delete lib;
        return nullptr;
    }

    // СОЗДАЁМ РАБОЧИЙ модуль
    BydaoModule* module = create();
    if (!module) {
        if (error) *error = "Failed to create module";
        lib->unload();
        delete lib;
        return nullptr;
    }

    module->initialize();

    // Сохраняем
    m_modules[name] = module;
    m_libraries[name] = lib;

    return module;
}

BydaoModuleInfo* BydaoModuleManager::loadModuleInfo(const QString& name, QString* error) {
    // Уже есть в кэше?
    if (m_infoCache.contains(name)) {
        return m_infoCache[name];
    }

    QString path = findModuleFile(name);
    if (path.isEmpty()) {
        if (error) *error = "Module not found: " + name;
        return nullptr;
    }

    QLibrary lib(path);
    if (!lib.load()) {
        if (error) *error = lib.errorString();
        return nullptr;
    }

    auto create = (BydaoModule* (*)())lib.resolve("createModule");
    if (!create) {
        if (error) *error = "No createModule()";
        lib.unload();
        return nullptr;
    }

    // ВРЕМЕННЫЙ модуль
    BydaoModule* tempModule = create();
    if (!tempModule) {
        if (error) *error = "Failed to create module";
        lib.unload();
        return nullptr;
    }

    // Копируем информацию
    BydaoModuleInfo* originalInfo = tempModule->info();
    auto* infoCopy = new BydaoModuleInfoImpl();

    if (auto* impl = dynamic_cast<BydaoModuleInfoImpl*>(originalInfo)) {
        infoCopy->m_name = impl->m_name;
        infoCopy->m_version = impl->m_version;
        infoCopy->m_properties = impl->m_properties;
        infoCopy->m_methods = impl->m_methods;
    }

    m_infoCache[name] = infoCopy;

    // Удаляем ВРЕМЕННЫЙ модуль (он больше не нужен)
    auto destroy = (void (*)(BydaoModule*))lib.resolve("destroyModule");
    if (destroy) destroy(tempModule);
    else delete tempModule;

    lib.unload();

    return infoCopy;
}

void BydaoModuleManager::unloadAllModules() {
    // qDebug() << "Unloading all modules, count:" << m_modules.size();

    // 1. Сначала вызываем shutdown для всех
    for (auto* module : m_modules) {
        module->shutdown();  // вызываем shutdown
    }

    // 2. Потом удаляем
    for (auto it = m_modules.begin(); it != m_modules.end(); ++it) {
        // qDebug() << "  Deleting module:" << it.key();
        delete it.value();  // деструктор уже НЕ вызывает shutdown
    }
    m_modules.clear();

    // 3. Выгружаем библиотеки
    for (auto* lib : m_libraries) {
        lib->unload();
        delete lib;
    }
    m_libraries.clear();

    // qDebug() << "All modules unloaded";
}

} // namespace BydaoScript
