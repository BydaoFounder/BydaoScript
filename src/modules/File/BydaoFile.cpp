#include "../include/BydaoScript/BydaoBool.h"
#include "../include/BydaoScript/BydaoInt.h"
#include "../include/BydaoScript/BydaoString.h"
#include "../include/BydaoScript/BydaoNull.h"
#include "../include/BydaoScript/BydaoArray.h"
#include "BydaoFile.h"
#include <QFileInfo>

namespace BydaoScript {
namespace Modules {

// ========== BydaoFileModule ==========

BydaoFileModule::BydaoFileModule()
    : BydaoModule()
{
    registerMethod("new",      &BydaoFileModule::method_new);
    registerMethod("exists",   &BydaoFileModule::method_exists);
    registerMethod("copy",     &BydaoFileModule::method_copy);
    registerMethod("move",     &BydaoFileModule::method_move);
    registerMethod("rename",   &BydaoFileModule::method_rename);
    registerMethod("remove",   &BydaoFileModule::method_remove);
    registerMethod("size",     &BydaoFileModule::method_size);
    registerMethod("readAll",  &BydaoFileModule::method_readAll);
    registerMethod("writeAll", &BydaoFileModule::method_writeAll);
}

BydaoFileModule::~BydaoFileModule() {
}

MetaData*   BydaoFileModule::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData->external = true;
        metaData->name = "File";

        // Методы модуля File

        metaData
            ->appendType( "new",     FuncMetaData("File", FMD_IMMUTABLE)
                                    << FuncArgMetaData("fileName","String",ARG_IN)
                                    )
            .appendType( "exists",   FuncMetaData("Bool", FMD_IMMUTABLE)
                                    << FuncArgMetaData("fileName","String",ARG_IN,"\"\"")
                                    )
            .appendType( "copy",     FuncMetaData("Bool", FMD_IMMUTABLE)
                                    << FuncArgMetaData("fromName","String",ARG_IN)
                                    << FuncArgMetaData("toName","String",ARG_IN)
                                    )
            .appendType( "move",     FuncMetaData("Bool", FMD_IMMUTABLE)
                                    << FuncArgMetaData("fromName","String",ARG_IN)
                                    << FuncArgMetaData("toName","String",ARG_IN)
                                    )
            .appendType( "rename",   FuncMetaData("Bool", FMD_IMMUTABLE)
                                    << FuncArgMetaData("fromName","String",ARG_IN)
                                    << FuncArgMetaData("toName","String",ARG_IN)
                                    )
            .appendType( "remove",   FuncMetaData("Bool", FMD_IMMUTABLE)
                                    << FuncArgMetaData("fileName","String",ARG_IN)
                                    )
            .appendType( "size",     FuncMetaData("Int", FMD_IMMUTABLE)
                                    << FuncArgMetaData("fileName","String",ARG_IN)
                                    )
            .appendType( "readAll",  FuncMetaData("String", FMD_IMMUTABLE)
                                    << FuncArgMetaData("fileName","String",ARG_IN)
                                    )
            .appendType( "writeAll", FuncMetaData("Bool", FMD_IMMUTABLE)
                                    << FuncArgMetaData("fileName","String",ARG_IN)
                                    << FuncArgMetaData("data","String",ARG_IN)
                                    )
            ;
    }
    return metaData;
}

UsedMetaDataList    BydaoFileModule::usedMetaData() {
    static UsedMetaDataList list;

    if ( list.isEmpty() ) {
        list << UsedMetaData( "File", BydaoFileObject::metaData(), true );
    }

    return list;
}


bool BydaoFileModule::initialize() {
    return true;
}

bool BydaoFileModule::shutdown() {
    return true;
}

void BydaoFileModule::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoFileModule::callMethod(const QString& name,
                                 const QVector<BydaoValue>& args,
                                 BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

// ========== Методы модуля ==========

bool BydaoFileModule::method_new(const QVector<BydaoValue>& args, BydaoValue& result) {
    QString name = (args.size() == 1) ? args[0].toString() : "";
    result = BydaoValue(new BydaoFileObject( name ), BydaoTypeId::TYPE_OBJECT);
    return true;
}

bool BydaoFileModule::method_exists(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    result = BydaoValue::fromBool( QFileInfo::exists(args[0].toString()) );
    return true;
}

bool BydaoFileModule::method_copy(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;
    bool ok = QFile::copy(args[0].toString(), args[1].toString());
    result = BydaoValue::fromBool(ok);
    return true;
}

bool BydaoFileModule::method_move(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;
    bool ok = QFile::rename(args[0].toString(), args[1].toString());
    result = BydaoValue::fromBool(ok);
    return true;
}

bool BydaoFileModule::method_rename(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;
    bool ok = QFile::rename(args[0].toString(), args[1].toString());
    result = BydaoValue::fromBool(ok);
    return true;
}

bool BydaoFileModule::method_remove(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    bool ok = QFile::remove(args[0].toString());
    result = BydaoValue::fromBool(ok);
    return true;
}

bool BydaoFileModule::method_size(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    qint64 size = QFileInfo(args[0].toString()).size();
    result = BydaoValue::fromInt( size );
    return true;
}

bool BydaoFileModule::method_readAll(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QFile file(args[0].toString());
    if ( ! file.exists() ) {
        setError( QString( "file '%1' not found" ).arg( file.fileName() ) );
        return false;
    }
    if ( file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();
        result = BydaoValue( BydaoString::create(content), BydaoTypeId::TYPE_STRING );
        return true;
    }
    setError( QString( "cannot open file '%1'" ).arg( file.fileName() ) );
    return false;
}

bool BydaoFileModule::method_writeAll(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;

    QFile file(args[0].toString());
    if ( ! file.exists() ) {
        setError( QString( "file '%1' not found" ).arg( file.fileName() ) );
        return false;
    }
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << args[1].toString();
        file.close();
        result = BydaoValue::fromBool( true );
        return true;
    }
    setError( QString( "cannot open file '%1'" ).arg( file.fileName() ) );
    return false;
}

// ========== BydaoFileObject ==========

BydaoFileObject::BydaoFileObject(const QString& path)
    : BydaoObject()
    , m_file(path)
    , m_stream(nullptr)
{
    registerMethod("open",      &BydaoFileObject::method_open);
    registerMethod("close",     &BydaoFileObject::method_close);
    registerMethod("read",      &BydaoFileObject::method_read);
    registerMethod("readln",    &BydaoFileObject::method_readLine);
    registerMethod("readlnAll", &BydaoFileObject::method_readLines);
    registerMethod("write",     &BydaoFileObject::method_write);
    registerMethod("writeln",   &BydaoFileObject::method_writeLine);
    registerMethod("append",    &BydaoFileObject::method_append);
    registerMethod("pos",       &BydaoFileObject::method_pos);
    registerMethod("seek",      &BydaoFileObject::method_seek);
    registerMethod("atEnd",     &BydaoFileObject::method_atEnd);
    registerMethod("isOpen",    &BydaoFileObject::method_isOpen);
    registerMethod("exists",    &BydaoFileObject::method_exists);
    registerMethod("remove",    &BydaoFileObject::method_remove);
}

BydaoFileObject::~BydaoFileObject() {
    if (m_stream) {
        delete m_stream;
        m_stream = nullptr;
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
}


MetaData*   BydaoFileObject::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData->external = true;
        metaData->name = "File";

        // Методы объекта типа File

        metaData
            ->appendObj( "open",       FuncMetaData("Bool", FMD_ALTERABLE )
                                 << FuncArgMetaData("mode","String",ARG_IN)
                     )
            .appendObj( "close",       FuncMetaData("Void", FMD_ALTERABLE)
                    )
            .appendObj( "read",        FuncMetaData("String", FMD_ALTERABLE)
                                << FuncArgMetaData("count","Int",ARG_IN,"-1")
                    )
            .appendObj( "readln",      FuncMetaData("String", FMD_ALTERABLE)
                    )
            .appendObj( "readlnAll",   FuncMetaData("Array", FMD_ALTERABLE)
                    )
            .appendObj( "write",       FuncMetaData("Bool", FMD_ALTERABLE)
                                 << FuncArgMetaData("data","String",ARG_IN)
                    )
            .appendObj( "writeln",     FuncMetaData("Bool", FMD_ALTERABLE)
                                   << FuncArgMetaData("data","String",ARG_IN)
                    )
            .appendObj( "append",      FuncMetaData("Bool", FMD_ALTERABLE)
                                  << FuncArgMetaData("data","String",ARG_IN)
                    )
            .appendObj( "isOpen",      FuncMetaData("Bool", FMD_IMMUTABLE)
                    )
            .appendObj( "atEnd",       FuncMetaData("Bool", FMD_IMMUTABLE)
                    )
            .appendObj( "pos",         FuncMetaData("Int", FMD_IMMUTABLE)
                    )
            .appendObj( "seek",        FuncMetaData("Bool", FMD_ALTERABLE)
                                << FuncArgMetaData("pos","Int",ARG_IN)
                    )
            .appendObj( "exists",      FuncMetaData("Bool", FMD_IMMUTABLE)
                    )
            .appendObj( "remove",      FuncMetaData("Bool", FMD_IMMUTABLE)
                    )
            ;

        metaData
            // переменные объекта
            ->appendObj( "name",     VarMetaData("String", VMD_VARIABLE) )
            .appendObj(  "path",     VarMetaData("String", VMD_CONST) )
            ;
    }
    return metaData;
}

void BydaoFileObject::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoFileObject::callMethod(const QString& name,
                                const QVector<BydaoValue>& args,
                                BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

QIODevice::OpenMode BydaoFileObject::parseMode(const QString& mode) {
    QIODevice::OpenMode flags = QIODevice::NotOpen;

    for (QChar ch : mode) {
        switch (ch.toLatin1()) {
        case 'r': flags |= QIODevice::ReadOnly; break;
        case 'w': flags |= QIODevice::WriteOnly | QIODevice::Truncate; break;
        case 'a': flags |= QIODevice::WriteOnly | QIODevice::Append; break;
        case '+': flags |= QIODevice::ReadWrite; break;
        case 't': flags |= QIODevice::Text; break;
        case 'b': break;
        default: break;
        }
    }
    return flags;
}

// ========== Методы объекта ==========

bool BydaoFileObject::method_open(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    if (m_file.isOpen()) {
        m_file.close();
    }

    QIODevice::OpenMode flags = parseMode(args[0].toString());
    bool ok = m_file.open(flags);

    if (ok && (flags & QIODevice::Text)) {
        if (m_stream) delete m_stream;
        m_stream = new QTextStream(&m_file);
    }

    result = BydaoValue::fromBool( ok );
    return true;
}

bool BydaoFileObject::method_close(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);

    if (m_stream) {
        delete m_stream;
        m_stream = nullptr;
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
    result = BydaoValue::fromNull();
    return true;
}

bool BydaoFileObject::method_read(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (!m_stream) {
        result = BydaoValue::fromNull();
        return false;
    }
    qint64 maxlen = args.size() > 0 ? args[0].toInt() : -1;
    result = BydaoValue( BydaoString::create( maxlen < 0 ? m_stream->readAll() : m_stream->read(maxlen) ), BydaoTypeId::TYPE_STRING );
    return true;
}

bool BydaoFileObject::method_readLine(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);

    if ( ! m_stream || m_stream->atEnd() ) {
        result = BydaoValue::fromNull();
        return false;
    }

    result = BydaoValue( BydaoString::create(m_stream->readLine()), BydaoTypeId::TYPE_STRING );
    return true;
}

bool BydaoFileObject::method_readLines(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);

    if (!m_stream) {
        result = BydaoValue::fromNull();
        return false;
    }

    auto* array = new BydaoArray();
    while (!m_stream->atEnd()) {
        array->append( BydaoValue( BydaoString::create( m_stream->readLine() ), BydaoTypeId::TYPE_STRING ) );
    }
    result = BydaoValue(array, BydaoTypeId::TYPE_ARRAY);
    return true;
}

bool BydaoFileObject::method_write(const QVector<BydaoValue>& args, BydaoValue& result) {

    if (!m_stream) {
        result = BydaoValue::fromNull();
        return false;
    }

    if ( args.size() != 1 ) {
        result = BydaoValue::fromBool( false );
        return false;
    }

    *m_stream << args[0].toString();
    result = BydaoValue::fromBool( m_stream->status() == QTextStream::Ok );
    return true;
}

bool BydaoFileObject::method_writeLine(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (!m_stream || args.size() != 1) {
        result = BydaoValue::fromBool( false );
        return false;
    }

    *m_stream << args[0].toString() << Qt::endl;
    result = BydaoValue::fromBool( m_stream->status() == QTextStream::Ok );
    return true;
}

bool BydaoFileObject::method_append(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) {
        result = BydaoValue::fromBool( false );
        return false;
    }

    QIODevice::OpenMode oldMode;
    bool wasOpen = m_file.isOpen();
    if ( wasOpen ) {
        oldMode = m_file.openMode();
        m_file.close();
    }

    bool ok = m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    if (ok) {
        QTextStream appender(&m_file);
        appender << args[0].toString();
        m_file.close();
    }

    if (wasOpen) {
        m_file.open(oldMode);
    }

    result = BydaoValue::fromBool( ok );
    return true;
}

bool BydaoFileObject::method_pos(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);

    if (!m_stream) {
        result = BydaoValue::fromNull();
        return false;
    }
    result = BydaoValue::fromInt( m_file.pos() );
    return true;
}

bool BydaoFileObject::method_seek(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (!m_stream) {
        result = BydaoValue::fromNull();
        return false;
    }

    if (args.size() != 1) {
        result = BydaoValue::fromBool( false );
        return false;
    }

    result = BydaoValue::fromBool( m_file.seek( args[0].toInt() ) );
    return true;
}

bool BydaoFileObject::method_atEnd(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    if (!m_stream) {
        result = BydaoValue::fromNull();
        return false;
    }
    result = BydaoValue::fromBool( m_file.atEnd() );
    return true;
}

bool BydaoFileObject::method_isOpen(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    if (!m_stream) {
        result = BydaoValue::fromNull();
        return false;
    }
    result = BydaoValue::fromBool( m_file.isOpen() );
    return true;
}

bool BydaoFileObject::method_exists(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue::fromBool( QFileInfo::exists( m_file.fileName() ) );
    return true;
}

bool BydaoFileObject::method_remove(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);

    if ( m_file.isOpen() ) {
        m_file.close();
    }
    result = BydaoValue::fromBool( m_file.remove() );
    return true;
}

// ========== Экспорт модуля ==========

extern "C" {
MODULE_EXPORT BydaoScript::BydaoModule* createModule() {
    return new BydaoFileModule();
}

MODULE_EXPORT void destroyModule(BydaoScript::BydaoModule* module) {
    delete module;
}
}

} // namespace Modules
} // namespace BydaoScript
