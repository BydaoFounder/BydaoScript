#pragma once

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "../../../include/BydaoScript/BydaoModule.h"
#include "../../../include/BydaoScript/BydaoString.h"

#ifdef BYDAOFILE_LIBRARY
#include "BydaoFile_global.h"
#else
#define BYDAOFILE_EXPORT
#endif

namespace BydaoScript {
namespace Modules {

// Объект файла
class BYDAOFILE_EXPORT BydaoFileObject : public BydaoModule {

public:
    explicit BydaoFileObject(const QString& path);
    ~BydaoFileObject();

    // Обязательные методы от BydaoObject
    QString typeName() const override { return "File"; }

    // Обязательные методы от BydaoModule (для объекта они не нужны, но должны быть)
    QString name() const override { return "FileObject"; }
    QString version() const override { return "1.0.0"; }

    virtual bool    getVar( const QString& varName, BydaoValue* value ) override {
        if ( varName == "name" ) {
            value->set( BydaoString::create( m_file.fileName() ) );
            return true;
        }
        qWarning() << QString("Member '%1' not exists in file object").arg( varName );
        return false;
    };

    virtual bool    setVar( const QString& varName, const BydaoValue* value ) override {
        if ( varName == "name" ) {
            m_file.setFileName( value->toString() );
            return true;
        }
        qWarning() << QString("Member '%1' not exists in file object").arg( varName );
        return false;
    };


    // Свойства (только чтение)
    QString fileName() const { return QFileInfo(m_file).fileName(); }
    QString filePath() const { return m_file.fileName(); }
    qint64 fileSize() const { return QFileInfo(m_file).size(); }
    bool fileExists() const { return QFileInfo::exists(m_file.fileName()); }
    bool isOpen() const { return m_file.isOpen(); }
    bool isReadable() const { return QFileInfo(m_file).isReadable(); }
    bool isWritable() const { return QFileInfo(m_file).isWritable(); }

    bool callMethod(const QString& name,
                    const BydaoValueList& args,
                    BydaoValue* result) override;

private:
    // Методы объекта
    bool method_open(const BydaoValueList& args, BydaoValue* result);
    bool method_close(const BydaoValueList& args, BydaoValue* result);
    bool method_read(const BydaoValueList& args, BydaoValue* result);
    bool method_readLine(const BydaoValueList& args, BydaoValue* result);
    bool method_readLines(const BydaoValueList& args, BydaoValue* result);
    bool method_write(const BydaoValueList& args, BydaoValue* result);
    bool method_writeLine(const BydaoValueList& args, BydaoValue* result);
    bool method_append(const BydaoValueList& args, BydaoValue* result);
    bool method_copy(const BydaoValueList& args, BydaoValue* result);
    bool method_move(const BydaoValueList& args, BydaoValue* result);
    bool method_rename(const BydaoValueList& args, BydaoValue* result);
    bool method_remove(const BydaoValueList& args, BydaoValue* result);
    bool method_pos(const BydaoValueList& args, BydaoValue* result);
    bool method_seek(const BydaoValueList& args, BydaoValue* result);
    bool method_atEnd(const BydaoValueList& args, BydaoValue* result);

    QIODevice::OpenMode parseMode(const QString& mode);

    using MethodPtr = bool (BydaoFileObject::*)(const BydaoValueList&, BydaoValue*);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    QFile m_file;
    QTextStream* m_stream;
};

// Модуль File
class BYDAOFILE_EXPORT BydaoFileModule : public BydaoModule {

public:
    explicit BydaoFileModule();
    ~BydaoFileModule();

    // Обязательные методы от BydaoObject
    QString typeName() const override { return "FileModule"; }

    // Обязательные методы от BydaoModule
    QString name() const override { return "File"; }
    QString version() const override { return "1.0.0"; }

    MetaData*   metaData() override;

    bool callMethod(const QString& name,
                    const BydaoValueList& args,
                    BydaoValue* result) override;

protected:
    bool initialize() override;
    bool shutdown() override;

private:
    // Методы модуля
    bool method_new(const BydaoValueList& args, BydaoValue* result);
    bool method_exists(const BydaoValueList& args, BydaoValue* result);
    bool method_copy(const BydaoValueList& args, BydaoValue* result);
    bool method_move(const BydaoValueList& args, BydaoValue* result);
    bool method_rename(const BydaoValueList& args, BydaoValue* result);
    bool method_remove(const BydaoValueList& args, BydaoValue* result);
    bool method_size(const BydaoValueList& args, BydaoValue* result);
    bool method_readAll(const BydaoValueList& args, BydaoValue* result);
    bool method_writeAll(const BydaoValueList& args, BydaoValue* result);

    using MethodPtr = bool (BydaoFileModule::*)(const BydaoValueList&, BydaoValue*);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов
};

} // namespace Modules
} // namespace BydaoScript
