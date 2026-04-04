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
        metaData
            ->append( "new",     FuncMetaData("File", true, true)
                                    << FuncArgMetaData("fileName","String",false)
                                    )
            .append( "exists",   FuncMetaData("Bool", true, true)
                                    << FuncArgMetaData("fileName","String",false,"\"\"")
                                    )
            .append( "copy",     FuncMetaData("Bool", true, true)
                                    << FuncArgMetaData("fromName","String",false)
                                    << FuncArgMetaData("toName","String",false)
                                    )
            .append( "move",     FuncMetaData("Bool", true, true)
                                    << FuncArgMetaData("fromName","String",false)
                                    << FuncArgMetaData("toName","String",false)
                                    )
            .append( "rename",   FuncMetaData("Bool", true, true)
                                    << FuncArgMetaData("fromName","String",false)
                                    << FuncArgMetaData("toName","String",false)
                                    )
            .append( "remove",   FuncMetaData("Bool", true, true)
                                    << FuncArgMetaData("fileName","String",false)
                                    )
            .append( "size",     FuncMetaData("Int", true, true)
                                    << FuncArgMetaData("fileName","String",false)
                                    )
            .append( "readAll",  FuncMetaData("String", true, true)
                                    << FuncArgMetaData("fileName","String",false)
                                    )
            .append( "writeAll", FuncMetaData("Bool", true, true)
                                    << FuncArgMetaData("fileName","String",false)
                                    << FuncArgMetaData("data","String",false)
                                    )
            ;
        // TODO: Добавить мета-данные для объекта файла (методы и переменные)
        metaData
            // переменные объекта
            ->append( "name",     VarMetaData("String",false,false) );
    }
    return metaData;
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
                                 const BydaoValueList& args,
                                 BydaoValue* result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

// ========== Методы модуля ==========

bool BydaoFileModule::method_new(const BydaoValueList& args, BydaoValue* result) {
    QString name = (args.size() == 1) ? args[0]->toString() : "";
    result->set( new BydaoFileObject( name ) );
    return true;
}

bool BydaoFileModule::method_exists(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) return false;
    result->set( BydaoBool::create( QFileInfo::exists(args[0]->toString()) ) );
    return true;
}

bool BydaoFileModule::method_copy(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 2) return false;
    bool ok = QFile::copy(args[0]->toString(), args[1]->toString());
    result->set( BydaoBool::create(ok) );
    return true;
}

bool BydaoFileModule::method_move(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 2) return false;
    bool ok = QFile::rename(args[0]->toString(), args[1]->toString());
    result->set( BydaoBool::create(ok) );
    return true;
}

bool BydaoFileModule::method_rename(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 2) return false;
    bool ok = QFile::rename(args[0]->toString(), args[1]->toString());
    result->set( BydaoBool::create(ok) );
    return true;
}

bool BydaoFileModule::method_remove(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) return false;
    bool ok = QFile::remove(args[0]->toString());
    result->set( BydaoBool::create(ok) );
    return true;
}

bool BydaoFileModule::method_size(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) return false;
    qint64 size = QFileInfo(args[0]->toString()).size();
    result->set( BydaoInt::create( size ) );
    return true;
}

bool BydaoFileModule::method_readAll(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) return false;

    QFile file(args[0]->toString());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();
        result->set( BydaoString::create(content) );
        return true;
    }
    result = BydaoValue::get();
    return false;
}

bool BydaoFileModule::method_writeAll(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 2) return false;

    QFile file(args[0]->toString());
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << args[1]->toString();
        file.close();
        result->set( BydaoBool::create( true ) );
        return true;
    }
    result->set( BydaoBool::create( false ) );
    return false;
}

// ========== BydaoFileObject ==========

BydaoFileObject::BydaoFileObject(const QString& path)
    : BydaoModule()
    , m_file(path)
    , m_stream(nullptr)
{
    registerMethod("open",      &BydaoFileObject::method_open);
    registerMethod("open",      &BydaoFileObject::method_open);
    registerMethod("close",     &BydaoFileObject::method_close);
    registerMethod("read",      &BydaoFileObject::method_read);
    registerMethod("readLine",  &BydaoFileObject::method_readLine);
    registerMethod("readLines", &BydaoFileObject::method_readLines);
    registerMethod("write",     &BydaoFileObject::method_write);
    registerMethod("writeLine", &BydaoFileObject::method_writeLine);
    registerMethod("append",    &BydaoFileObject::method_append);
    registerMethod("copy",      &BydaoFileObject::method_copy);
    registerMethod("move",      &BydaoFileObject::method_move);
    registerMethod("rename",    &BydaoFileObject::method_rename);
    registerMethod("remove",    &BydaoFileObject::method_remove);
    registerMethod("pos",       &BydaoFileObject::method_pos);
    registerMethod("seek",      &BydaoFileObject::method_seek);
    registerMethod("atEnd",     &BydaoFileObject::method_atEnd);

    // Регистрация свойств (ReadOnly)
    // registerProperty("name",
    //                  [this]() { return BydaoValue::fromString(this->fileName()); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));

    // registerProperty("path",
    //                  [this]() { return BydaoValue::fromString(this->filePath()); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));

    // registerProperty("size",
    //                  [this]() { return BydaoValue::fromInt((int)this->fileSize()); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));

    // registerProperty("exists",
    //                  [this]() { return BydaoValue::fromBool(this->fileExists()); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));

    // registerProperty("isOpen",
    //                  [this]() { return BydaoValue::fromBool(this->isOpen()); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));

    // registerProperty("isReadable",
    //                  [this]() { return BydaoValue::fromBool(this->isReadable()); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));

    // registerProperty("isWritable",
    //                  [this]() { return BydaoValue::fromBool(this->isWritable()); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));
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

void BydaoFileObject::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoFileObject::callMethod(const QString& name,
                                const BydaoValueList& args,
                                BydaoValue* result) {
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

bool BydaoFileObject::method_open(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) return false;

    if (m_file.isOpen()) {
        m_file.close();
    }

    QIODevice::OpenMode flags = parseMode(args[0]->toString());
    bool ok = m_file.open(flags);

    if (ok && (flags & QIODevice::Text)) {
        if (m_stream) delete m_stream;
        m_stream = new QTextStream(&m_file);
    }

    result->set( BydaoBool::create( ok ) );
    return true;
}

bool BydaoFileObject::method_close(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    Q_UNUSED(result);

    if (m_stream) {
        delete m_stream;
        m_stream = nullptr;
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
    return true;
}

bool BydaoFileObject::method_read(const BydaoValueList& args, BydaoValue* result) {
    if (!m_stream) {
        return false;
    }

    qint64 maxlen = args.size() > 0 ? args[0]->toInt() : -1;

    if (maxlen < 0) {
        result->set( BydaoString::create(m_stream->readAll()) );
    }
    else {
        result->set( BydaoString::create(m_stream->read(maxlen)) );
    }
    return true;
}

bool BydaoFileObject::method_readLine(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);

    if (!m_stream) {
        return false;
    }

    result->set( BydaoString::create(m_stream->readLine()) );
    return true;
}

bool BydaoFileObject::method_readLines(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);

    if (!m_stream) {
        result->set( new BydaoArray() );
        return false;
    }

    auto* array = new BydaoStringArray();
    while (!m_stream->atEnd()) {
        QString line = m_stream->readLine();
        array->append( BydaoValue::get( BydaoString::create(line) ) );
    }
    result->set( array );
    return true;
}

bool BydaoFileObject::method_write(const BydaoValueList& args, BydaoValue* result) {
    if (!m_stream || args.size() != 1) {
        result->set( BydaoBool::create( false ) );
        return false;
    }

    *m_stream << args[0]->toString();
    result->set( BydaoBool::create( true ) );
    return true;
}

bool BydaoFileObject::method_writeLine(const BydaoValueList& args, BydaoValue* result) {
    if (!m_stream || args.size() != 1) {
        result->set( BydaoBool::create( false ) );
        return false;
    }

    *m_stream << args[0]->toString() << Qt::endl;
    result->set( BydaoBool::create( true ) );
    return true;
}

bool BydaoFileObject::method_append(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) {
        result->set( BydaoBool::create( false ) );
        return false;
    }

    bool wasOpen = m_file.isOpen();
    QIODevice::OpenMode oldMode;

    if (wasOpen) {
        oldMode = m_file.openMode();
        m_file.close();
    }

    bool ok = m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    if (ok) {
        QTextStream appender(&m_file);
        appender << args[0]->toString();
        m_file.close();
    }

    if (wasOpen) {
        m_file.open(oldMode);
    }

    result->set( BydaoBool::create( ok ) );
    return true;
}

bool BydaoFileObject::method_copy(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) {
        result->set( BydaoBool::create( false ) );
        return false;
    }

    bool ok = QFile::copy(m_file.fileName(), args[0]->toString());
    result->set( BydaoBool::create( ok ) );
    return true;
}

bool BydaoFileObject::method_move(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) {
        result->set( BydaoBool::create( false ) );
        return false;
    }

    bool ok = QFile::rename(m_file.fileName(), args[0]->toString());
    result->set( BydaoBool::create( ok ) );
    return true;
}

bool BydaoFileObject::method_rename(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) {
        result->set( BydaoBool::create( false ) );
        return false;
    }

    QFileInfo info(m_file.fileName());
    QString newPath = info.absolutePath() + "/" + args[0]->toString();
    bool ok = QFile::rename(m_file.fileName(), newPath);
    result->set( BydaoBool::create( ok ) );
    return true;
}

bool BydaoFileObject::method_remove(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);

    if ( m_file.isOpen() ) {
        m_file.close();
    }
    result->set( BydaoBool::create( m_file.remove() ) );
    return true;
}

bool BydaoFileObject::method_pos(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoInt::create( m_file.pos() ) );
    return true;
}

bool BydaoFileObject::method_seek(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) {
        result->set( BydaoBool::create( false ) );
        return false;
    }

    bool ok = m_file.seek(args[0]->toInt());
    result->set( BydaoBool::create( ok ) );
    return true;
}

bool BydaoFileObject::method_atEnd(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoBool::create( m_file.atEnd() ) );
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
