#include "../include/BydaoScript/BydaoArray.h"
#include "../include/BydaoScript/BydaoString.h"
#include "../include/BydaoScript/BydaoNull.h"
#include "BydaoDir.h"
//#include "BydaoDir_global.h"

namespace BydaoScript {
namespace Modules {

// ========== BydaoDirModule ==========

BydaoDirModule::BydaoDirModule()
    : BydaoModule()
{
    registerMethod("new",     &BydaoDirModule::method_new);
    registerMethod("isDir",   &BydaoDirModule::method_isdir);
    registerMethod("isFile",  &BydaoDirModule::method_isfile);
    registerMethod("list",    &BydaoDirModule::method_list);
    registerMethod("cd",      &BydaoDirModule::method_cd);
    registerMethod("current", &BydaoDirModule::method_current);
    registerMethod("mkdir",   &BydaoDirModule::method_mkdir);
    registerMethod("rmdir",   &BydaoDirModule::method_rmdir);
}

BydaoDirModule::~BydaoDirModule() {
//    shutdown();
}

MetaData*   BydaoDirModule::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData->external = true;
        metaData
            ->appendType( "new",    FuncMetaData("Dir",         FMD_IMMUTABLE) << FuncArgMetaData("path","String",ARG_IN) )
            .appendType( "isDir",   FuncMetaData("Bool",        FMD_IMMUTABLE) << FuncArgMetaData("path","String",ARG_IN,"\".\"") )
            .appendType( "isFile",  FuncMetaData("Bool",        FMD_IMMUTABLE) << FuncArgMetaData("path","String",ARG_IN,"\".\"") )
            .appendType( "list",    FuncMetaData("StringArray", FMD_IMMUTABLE) << FuncArgMetaData("path","String",ARG_IN,"\".\"") )
            .appendType( "cd",      FuncMetaData("Bool",        FMD_IMMUTABLE) << FuncArgMetaData("path","String",ARG_IN,"") )
            .appendType( "current", FuncMetaData("String",      FMD_IMMUTABLE) )
            .appendType( "mkdir",   FuncMetaData("Bool",        FMD_IMMUTABLE) << FuncArgMetaData("path","String",ARG_IN,"") )
            .appendType( "rmdir",   FuncMetaData("Bool",        FMD_IMMUTABLE) << FuncArgMetaData("path","String",ARG_IN,"") )
            ;
    }
    return metaData;
}

/**
 * Вернуть список используемых типов.
 */
UsedMetaDataList    BydaoDirModule::usedMetaData() {
    static UsedMetaDataList list;
    return list;
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

bool BydaoDirModule::method_new(const QVector<BydaoValue>& args, BydaoValue& result) {
    QString name = (args.size() == 1) ? args[0].toString() : "./";
    result = BydaoValue(new BydaoDirObject( name ), BydaoTypeId::TYPE_OBJECT);
    return true;
}

bool BydaoDirModule::method_isdir(const QVector<BydaoValue>& args, BydaoValue& result) {
    if ( args.size() != 1 ) {
        result = BydaoValue::fromBool( false );
    }
    QFileInfo info( args[0].toString() );
    result = BydaoValue::fromBool( info.isDir() );
    return true;
}

bool BydaoDirModule::method_isfile(const QVector<BydaoValue>& args, BydaoValue& result) {
    if ( args.size() != 1 ) {
        result = BydaoValue::fromBool( false );
    }
    QFileInfo info( args[0].toString() );
    result = BydaoValue::fromBool( info.isFile() );
    return true;
}

/**
 * Сформировать список файлов.
 *
 *
 *
 * @param args
 * @param result
 *
 * @return
 */
bool BydaoDirModule::method_list(const QVector<BydaoValue>& args, BydaoValue& result) {

    QString path, mask;
    if ( args.size() > 0 ) {
        QString str = QDir::cleanPath( args[0].toString() );
        if ( str.contains(QChar('*')) || str.contains(QChar('?')) ) {
            str.replace( QChar('\\'), QChar('/') );
            int pos = str.lastIndexOf( QChar('/') );
            if ( pos >= 0 ) {
                path = str.left( pos );
                mask = str.mid( pos + 1 );
            }
            else {
                path = "";
                mask = str;
            }
        }
        else {
            path = str;
            mask = "*";
        }
    }
    if ( path.isEmpty() ) path = "./";
    if ( ! path.endsWith( QDir::separator() ) ) path += QDir::separator();
    if ( mask.isEmpty() ) mask = "*";

    QDir::Filters filters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files;
    QDir::SortFlags sort = QDir::Name | QDir::IgnoreCase;

    QDir dir(path);
    QFileInfoList list = dir.entryInfoList( QStringList() << mask, filters, sort );
    auto* array = new BydaoArray();
    foreach ( const QFileInfo& info, list ) {
        array->append(BydaoValue( BydaoString::create(info.fileName()), BydaoTypeId::TYPE_STRING ));
    }
    result = BydaoValue(array, BydaoTypeId::TYPE_ARRAY);
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
    result = BydaoValue( BydaoString::create(QDir::currentPath()), BydaoTypeId::TYPE_STRING );
    return true;
}

bool BydaoDirModule::method_mkdir(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    result = BydaoValue::fromBool( QDir().mkpath(args[0].toString()) );
    return true;
}

bool BydaoDirModule::method_rmdir(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    result = BydaoValue::fromBool( QDir().rmdir(args[0].toString()) );
    return true;
}

// ========== BydaoDirObject ==========

BydaoDirObject::BydaoDirObject(const QString& path)
    : BydaoModule()
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
    // registerProperty("path",
    //                  [this]() { return BydaoValue::fromString(m_dir.path()); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));

    // registerProperty("name",
    //                  [this]() { return BydaoValue::fromString(m_dir.dirName()); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));
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
        array->append( BydaoValue( BydaoString::create(entry), BydaoTypeId::TYPE_STRING ) );
    }
    result = BydaoValue(array, BydaoTypeId::TYPE_ARRAY);
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
    result = BydaoValue( BydaoString::create(m_dir.absolutePath()), BydaoTypeId::TYPE_STRING );
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
