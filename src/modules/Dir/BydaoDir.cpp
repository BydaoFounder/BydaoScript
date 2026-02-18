#include "../include/BydaoScript/BydaoBool.h"
#include "../include/BydaoScript/BydaoArray.h"
#include "../include/BydaoScript/BydaoString.h"
#include "BydaoDir.h"
#include "BydaoDir_global.h"

namespace BydaoScript {
namespace Modules {

// ========== Статический info ==========
BydaoModuleInfoImpl* BydaoDirModule::s_info = nullptr;

BydaoModuleInfoImpl* BydaoDirModule::createInfo() {
    auto* info = new BydaoModuleInfoImpl();
    info->m_name = "Dir";
    info->m_version = "1.0.0";
    info->m_properties = {"path", "name"};
    info->m_methods = {
        {"open",    {"path"}},
        {"list",    {"filter"}},
        {"cd",      {"path"}},
        {"current", {}},
        {"mkdir",   {"path"}},
        {"rmdir",   {"path"}}
    };
    return info;
}

BydaoModuleInfo* BydaoDirModule::info() const {
    if (!s_info) s_info = createInfo();
    return s_info;
}

// ========== BydaoDirModule ==========

BydaoDirModule::BydaoDirModule(QObject* parent)
    : BydaoModule(parent)
{
    registerMethod("open",   [this](auto& args, auto& result) { return this->method_open(args, result); });
    registerMethod("list",   [this](auto& args, auto& result) { return this->method_list(args, result); });
    registerMethod("cd",     [this](auto& args, auto& result) { return this->method_cd(args, result); });
    registerMethod("current",[this](auto& args, auto& result) { return this->method_current(args, result); });
    registerMethod("mkdir",  [this](auto& args, auto& result) { return this->method_mkdir(args, result); });
    registerMethod("rmdir",  [this](auto& args, auto& result) { return this->method_rmdir(args, result); });
}

BydaoDirModule::~BydaoDirModule() {
    shutdown();
}

bool BydaoDirModule::initialize() { return true; }
bool BydaoDirModule::shutdown() { return true; }

// ========== Методы модуля ==========

bool BydaoDirModule::method_open(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    result = BydaoValue(new BydaoDirObject(args[0].toString()));
    return true;
}

bool BydaoDirModule::method_list(const QVector<BydaoValue>& args, BydaoValue& result) {
    QString path = args.size() > 0 ? args[0].toString() : ".";
    QDir dir(path);
    auto entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    auto* array = new BydaoArray();
    for (const QString& entry : entries) {
        array->append(BydaoValue(new BydaoString(entry)));
    }
    result = BydaoValue(array);
    return true;
}

bool BydaoDirModule::method_cd(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    bool ok = QDir::setCurrent(args[0].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoDirModule::method_current(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(QDir::currentPath()));
    return true;
}

bool BydaoDirModule::method_mkdir(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    bool ok = QDir().mkpath(args[0].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoDirModule::method_rmdir(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    bool ok = QDir().rmdir(args[0].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

// ========== BydaoDirObject ==========

BydaoDirObject::BydaoDirObject(const QString& path, QObject* parent)
    : BydaoModule(parent)
{
    if (path.isEmpty()) {
        m_dir = QDir::current();
    } else {
        m_dir = QDir(path);
    }

    registerMethod("list",   [this](auto& args, auto& result) { return this->method_list(args, result); });
    registerMethod("cd",     [this](auto& args, auto& result) { return this->method_cd(args, result); });
    registerMethod("mkdir",  [this](auto& args, auto& result) { return this->method_mkdir(args, result); });
    registerMethod("rmdir",  [this](auto& args, auto& result) { return this->method_rmdir(args, result); });
    registerMethod("exists", [this](auto& args, auto& result) { return this->method_exists(args, result); });
    registerMethod("current",[this](auto& args, auto& result) { return this->method_current(args, result); });

    // Свойства "path" и "name" релизованы в методе getProperty()
    // registerProperty("path");
    // registerProperty("name");
}

bool BydaoDirObject::getProperty(const QString& name, BydaoValue& result) {
    if (name == "path") {
        result = BydaoValue(new BydaoString(m_dir.path()));
        return true;
    }
    if (name == "name") {
        result = BydaoValue(new BydaoString(m_dir.dirName()));
        return true;
    }
    return BydaoModule::getProperty(name, result);
}

// ========== Методы объекта ==========

bool BydaoDirObject::method_list(const QVector<BydaoValue>& args, BydaoValue& result) {
    QString filter = args.size() > 0 ? args[0].toString() : "*";
    QStringList nameFilters = filter.isEmpty() ? QStringList() : QStringList(filter);
    auto entries = m_dir.entryList(nameFilters, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    auto* array = new BydaoArray();
    for (const QString& entry : entries) {
        array->append(BydaoValue(new BydaoString(entry)));
    }
    result = BydaoValue(array);
    return true;
}

bool BydaoDirObject::method_cd(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    bool ok = m_dir.cd(args[0].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoDirObject::method_mkdir(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    bool ok = m_dir.mkdir(args[0].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoDirObject::method_rmdir(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    bool ok = m_dir.rmdir(args[0].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoDirObject::method_exists(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(m_dir.exists()));
    return true;
}

bool BydaoDirObject::method_current(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(m_dir.absolutePath()));
    return true;
}

// ========== Экспорт модуля ==========

extern "C" {
MODULE_EXPORT BydaoScript::BydaoModule* createModule() {
    return new BydaoDirModule();
}

MODULE_EXPORT void destroyModule(BydaoScript::BydaoModule* module) {
    delete module;
}
}

} // namespace Modules
} // namespace BydaoScript
