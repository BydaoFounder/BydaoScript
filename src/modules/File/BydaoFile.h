#pragma once

#include "../../../include/BydaoScript/BydaoModule.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#ifdef BYDAOFILE_LIBRARY
#include "BydaoFile_global.h"
#else
#define BYDAOFILE_EXPORT
#endif

namespace BydaoScript {
namespace Modules {

// Объект файла
class BYDAOFILE_EXPORT BydaoFileObject : public BydaoModule {
    Q_OBJECT

public:
    explicit BydaoFileObject(const QString& path, QObject* parent = nullptr);
    ~BydaoFileObject();

    // Обязательные методы от BydaoObject
    QString typeName() const override { return "FileObject"; }

    // Обязательные методы от BydaoModule (для объекта они не нужны, но должны быть)
    QString name() const override { return "FileObject"; }
    QString version() const override { return "1.0.0"; }
    BydaoModuleInfo* info() const override { return nullptr; }

    // Свойства (только чтение)
    QString fileName() const { return QFileInfo(m_file).fileName(); }
    QString filePath() const { return m_file.fileName(); }
    qint64 fileSize() const { return QFileInfo(m_file).size(); }
    bool fileExists() const { return QFileInfo::exists(m_file.fileName()); }
    bool isOpen() const { return m_file.isOpen(); }
    bool isReadable() const { return QFileInfo(m_file).isReadable(); }
    bool isWritable() const { return QFileInfo(m_file).isWritable(); }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

private:
    // Методы объекта
    bool method_open(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_close(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_read(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_readLine(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_readLines(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_write(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_writeLine(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_append(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_copy(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_move(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_rename(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_remove(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_pos(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_seek(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_atEnd(const QVector<BydaoValue>& args, BydaoValue& result);

    QIODevice::OpenMode parseMode(const QString& mode);

    using MethodPtr = bool (BydaoFileObject::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    QFile m_file;
    QTextStream* m_stream;
};

// Модуль File
class BYDAOFILE_EXPORT BydaoFileModule : public BydaoModule {
    Q_OBJECT

public:
    explicit BydaoFileModule(QObject* parent = nullptr);
    ~BydaoFileModule();

    // Обязательные методы от BydaoObject
    QString typeName() const override { return "FileModule"; }

    // Обязательные методы от BydaoModule
    QString name() const override { return "File"; }
    QString version() const override { return "1.0.0"; }
    BydaoModuleInfo* info() const override;

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

protected:
    bool initialize() override;
    bool shutdown() override;

private:
    // Методы модуля
    bool method_new(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_exists(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_copy(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_move(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_rename(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_remove(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_size(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_readAll(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_writeAll(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoFileModule::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    static BydaoModuleInfoImpl* createInfo();
    static BydaoModuleInfoImpl* s_info;
};

} // namespace Modules
} // namespace BydaoScript
