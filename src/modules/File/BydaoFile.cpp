#include "../include/BydaoScript/BydaoBool.h"
#include "../include/BydaoScript/BydaoInt.h"
#include "../include/BydaoScript/BydaoString.h"
#include "../include/BydaoScript/BydaoNull.h"
#include "../include/BydaoScript/BydaoArray.h"
#include "BydaoFile.h"
#include "BydaoFile_global.h"
#include <QFileInfo>

namespace BydaoScript {
namespace Modules {

// ========== Статический info ==========
BydaoModuleInfoImpl* BydaoFileModule::s_info = nullptr;

BydaoModuleInfoImpl* BydaoFileModule::createInfo() {
    auto* info = new BydaoModuleInfoImpl();
    info->m_name = "File";
    info->m_version = "1.0.0";

    info->m_properties = {"name", "path", "size", "exists",
                          "isOpen", "isReadable", "isWritable"};

    info->m_methods = {
        {"open",      {"mode"}},
        {"exists",    {"path"}},
        {"copy",      {"src", "dst"}},
        {"move",      {"src", "dst"}},
        {"rename",    {"oldPath", "newPath"}},
        {"remove",    {"path"}},
        {"size",      {"path"}},
        {"readAll",   {"path"}},
        {"writeAll",  {"path", "content"}}
    };

    return info;
}

BydaoModuleInfo* BydaoFileModule::info() const {
    if (!s_info) {
        s_info = createInfo();
    }
    return s_info;
}

// ========== BydaoFileModule ==========

BydaoFileModule::BydaoFileModule(QObject* parent)
    : BydaoModule(parent)
{
    registerMethod("open",     [this](auto& args, auto& result) { return this->method_open(args, result); });
    registerMethod("exists",   [this](auto& args, auto& result) { return this->method_exists(args, result); });
    registerMethod("copy",     [this](auto& args, auto& result) { return this->method_copy(args, result); });
    registerMethod("move",     [this](auto& args, auto& result) { return this->method_move(args, result); });
    registerMethod("rename",   [this](auto& args, auto& result) { return this->method_rename(args, result); });
    registerMethod("remove",   [this](auto& args, auto& result) { return this->method_remove(args, result); });
    registerMethod("size",     [this](auto& args, auto& result) { return this->method_size(args, result); });
    registerMethod("readAll",  [this](auto& args, auto& result) { return this->method_readAll(args, result); });
    registerMethod("writeAll", [this](auto& args, auto& result) { return this->method_writeAll(args, result); });
}

BydaoFileModule::~BydaoFileModule() {
    shutdown();
}

bool BydaoFileModule::initialize() {
    return true;
}

bool BydaoFileModule::shutdown() {
    return true;
}

// ========== Методы модуля ==========

bool BydaoFileModule::method_open(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    result = BydaoValue(new BydaoFileObject(args[0].toString()));
    return true;
}

bool BydaoFileModule::method_exists(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    result = BydaoValue(new BydaoBool(QFileInfo::exists(args[0].toString())));
    return true;
}

bool BydaoFileModule::method_copy(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;
    bool ok = QFile::copy(args[0].toString(), args[1].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoFileModule::method_move(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;
    bool ok = QFile::rename(args[0].toString(), args[1].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoFileModule::method_rename(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;
    bool ok = QFile::rename(args[0].toString(), args[1].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoFileModule::method_remove(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    bool ok = QFile::remove(args[0].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoFileModule::method_size(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    qint64 size = QFileInfo(args[0].toString()).size();
    result = BydaoValue(new BydaoInt((int)size));
    return true;
}

bool BydaoFileModule::method_readAll(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QFile file(args[0].toString());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();
        result = BydaoValue(new BydaoString(content));
        return true;
    }
    result = BydaoValue(BydaoNull::instance());
    return false;
}

bool BydaoFileModule::method_writeAll(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;

    QFile file(args[0].toString());
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << args[1].toString();
        file.close();
        result = BydaoValue(new BydaoBool(true));
        return true;
    }
    result = BydaoValue(new BydaoBool(false));
    return false;
}

// ========== BydaoFileObject ==========

BydaoFileObject::BydaoFileObject(const QString& path, QObject* parent)
    : BydaoModule(parent)
    , m_file(path)
    , m_stream(nullptr)
{
    registerMethod("open",      [this](auto& args, auto& result) { return this->method_open(args, result); });
    registerMethod("close",     [this](auto& args, auto& result) { return this->method_close(args, result); });
    registerMethod("read",      [this](auto& args, auto& result) { return this->method_read(args, result); });
    registerMethod("readLine",  [this](auto& args, auto& result) { return this->method_readLine(args, result); });
    registerMethod("readLines", [this](auto& args, auto& result) { return this->method_readLines(args, result); });
    registerMethod("write",     [this](auto& args, auto& result) { return this->method_write(args, result); });
    registerMethod("writeLine", [this](auto& args, auto& result) { return this->method_writeLine(args, result); });
    registerMethod("append",    [this](auto& args, auto& result) { return this->method_append(args, result); });
    registerMethod("copy",      [this](auto& args, auto& result) { return this->method_copy(args, result); });
    registerMethod("move",      [this](auto& args, auto& result) { return this->method_move(args, result); });
    registerMethod("rename",    [this](auto& args, auto& result) { return this->method_rename(args, result); });
    registerMethod("remove",    [this](auto& args, auto& result) { return this->method_remove(args, result); });
    registerMethod("pos",       [this](auto& args, auto& result) { return this->method_pos(args, result); });
    registerMethod("seek",      [this](auto& args, auto& result) { return this->method_seek(args, result); });
    registerMethod("atEnd",     [this](auto& args, auto& result) { return this->method_atEnd(args, result); });

    registerProperty("name");
    registerProperty("path");
    registerProperty("size");
    registerProperty("exists");
    registerProperty("isOpen");
    registerProperty("isReadable");
    registerProperty("isWritable");
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

bool BydaoFileObject::getProperty(const QString& name, BydaoValue& result) {
    if (name == "name") {
        result = BydaoValue(new BydaoString(this->getName()));
        return true;
    }
    if (name == "path") {
        result = BydaoValue(new BydaoString(this->getPath()));
        return true;
    }
    if (name == "size") {
        result = BydaoValue(new BydaoInt((int)this->size()));
        return true;
    }
    if (name == "exists") {
        result = BydaoValue(new BydaoBool(this->exists()));
        return true;
    }
    if (name == "isOpen") {
        result = BydaoValue(new BydaoBool(this->isOpen()));
        return true;
    }
    if (name == "isReadable") {
        result = BydaoValue(new BydaoBool(this->isReadable()));
        return true;
    }
    if (name == "isWritable") {
        result = BydaoValue(new BydaoBool(this->isWritable()));
        return true;
    }
    return BydaoModule::getProperty(name, result);
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

    if (ok && !(flags & QIODevice::Text)) {
        if (m_stream) delete m_stream;
        m_stream = new QTextStream(&m_file);
    }

    result = BydaoValue(new BydaoBool(ok));
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
    result = BydaoValue(BydaoNull::instance());
    return true;
}

bool BydaoFileObject::method_read(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (!m_stream) {
        result = BydaoValue(BydaoNull::instance());
        return false;
    }

    qint64 maxlen = args.size() > 0 ? args[0].toInt() : -1;

    if (maxlen < 0) {
        result = BydaoValue(new BydaoString(m_stream->readAll()));
    } else {
        result = BydaoValue(new BydaoString(m_stream->read(maxlen)));
    }
    return true;
}

bool BydaoFileObject::method_readLine(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);

    if (!m_stream) {
        result = BydaoValue(BydaoNull::instance());
        return false;
    }

    result = BydaoValue(new BydaoString(m_stream->readLine()));
    return true;
}

bool BydaoFileObject::method_readLines(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);

    if (!m_stream) {
        result = BydaoValue(new BydaoArray());
        return false;
    }

    auto* array = new BydaoArray();
    while (!m_stream->atEnd()) {
        QString line = m_stream->readLine();
        array->append(BydaoValue(new BydaoString(line)));
    }

    result = BydaoValue(array);
    return true;
}

bool BydaoFileObject::method_write(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (!m_stream || args.size() != 1) {
        result = BydaoValue(new BydaoBool(false));
        return false;
    }

    *m_stream << args[0].toString();
    result = BydaoValue(new BydaoBool(true));
    return true;
}

bool BydaoFileObject::method_writeLine(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (!m_stream || args.size() != 1) {
        result = BydaoValue(new BydaoBool(false));
        return false;
    }

    *m_stream << args[0].toString() << Qt::endl;
    result = BydaoValue(new BydaoBool(true));
    return true;
}

bool BydaoFileObject::method_append(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) {
        result = BydaoValue(new BydaoBool(false));
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
        appender << args[0].toString();
        m_file.close();
    }

    if (wasOpen) {
        m_file.open(oldMode);
    }

    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoFileObject::method_copy(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) {
        result = BydaoValue(new BydaoBool(false));
        return false;
    }

    bool ok = QFile::copy(m_file.fileName(), args[0].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoFileObject::method_move(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) {
        result = BydaoValue(new BydaoBool(false));
        return false;
    }

    bool ok = QFile::rename(m_file.fileName(), args[0].toString());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoFileObject::method_rename(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) {
        result = BydaoValue(new BydaoBool(false));
        return false;
    }

    QFileInfo info(m_file.fileName());
    QString newPath = info.absolutePath() + "/" + args[0].toString();
    bool ok = QFile::rename(m_file.fileName(), newPath);
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoFileObject::method_remove(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);

    bool wasOpen = m_file.isOpen();
    if (wasOpen) {
        m_file.close();
    }
    bool ok = m_file.remove();
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoFileObject::method_pos(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoInt((int)m_file.pos()));
    return true;
}

bool BydaoFileObject::method_seek(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) {
        result = BydaoValue(new BydaoBool(false));
        return false;
    }

    bool ok = m_file.seek(args[0].toInt());
    result = BydaoValue(new BydaoBool(ok));
    return true;
}

bool BydaoFileObject::method_atEnd(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(m_file.atEnd()));
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
