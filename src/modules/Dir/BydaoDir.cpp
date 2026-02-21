#include "../include/BydaoScript/BydaoBool.h"
#include "../include/BydaoScript/BydaoArray.h"
#include "../include/BydaoScript/BydaoString.h"
#include "BydaoDir.h"
//#include "BydaoDir_global.h"

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
    registerMethod("open",    &BydaoDirModule::method_open);
    registerMethod("list",    &BydaoDirModule::method_list);
    registerMethod("cd",      &BydaoDirModule::method_cd);
    registerMethod("current", &BydaoDirModule::method_current);
    registerMethod("mkdir",   &BydaoDirModule::method_mkdir);
    registerMethod("rmdir",   &BydaoDirModule::method_rmdir);
}

BydaoDirModule::~BydaoDirModule() {
//    shutdown();
}

bool BydaoDirModule::initialize() { return true; }
bool BydaoDirModule::shutdown() { return true; }

void BydaoDirModule::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoDirModule::callMethod(const QString& name,
                             const QVector<BydaoValue>& args,
                             BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

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
    result = BydaoValue::fromBool( ok );
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
    result = BydaoValue::fromBool( ok );
    return true;
}

bool BydaoDirModule::method_rmdir(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    bool ok = QDir().rmdir(args[0].toString());
    result = BydaoValue::fromBool( ok );
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

    registerMethod("list",   &BydaoDirObject::method_list);
    registerMethod("cd",     &BydaoDirObject::method_cd);
    registerMethod("mkdir",  &BydaoDirObject::method_mkdir);
    registerMethod("rmdir",  &BydaoDirObject::method_rmdir);
    registerMethod("exists", &BydaoDirObject::method_exists);
    registerMethod("current",&BydaoDirObject::method_current);

    // Свойства (ReadOnly)
    registerProperty("path",
                     [this]() { return BydaoValue::fromString(m_dir.path()); },
                     nullptr,
                     BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));

    registerProperty("name",
                     [this]() { return BydaoValue::fromString(m_dir.dirName()); },
                     nullptr,
                     BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));
}

void BydaoDirObject::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoDirObject::callMethod(const QString& name,
                                const QVector<BydaoValue>& args,
                                BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
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
    result = BydaoValue::fromBool( ok );
    return true;
}

bool BydaoDirObject::method_mkdir(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    bool ok = m_dir.mkdir(args[0].toString());
    result = BydaoValue::fromBool( ok );
    return true;
}

bool BydaoDirObject::method_rmdir(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    bool ok = m_dir.rmdir(args[0].toString());
    result = BydaoValue::fromBool( ok );
    return true;
}

bool BydaoDirObject::method_exists(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue::fromBool( m_dir.exists() );
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
