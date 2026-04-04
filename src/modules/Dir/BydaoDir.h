#pragma once

#include <QDir>

#include "../../../include/BydaoScript/BydaoModule.h"
#include "../../../include/BydaoScript/BydaoArrayIterator.h"

#ifdef BYDAODIR_LIBRARY
#include "BydaoDir_global.h"
#else
#define BYDAODIR_EXPORT
#endif

namespace BydaoScript {
namespace Modules {

// Объект директории
class BYDAODIR_EXPORT BydaoDirObject : public BydaoModule {
    // Q_PROPERTY(QString path READ path)
    // Q_PROPERTY(QString name READ name)

public:
    explicit BydaoDirObject(const QString& path = QString());

    // Обязательные методы от BydaoObject
    QString typeName() const override { return "DirObject"; }

    // Обязательные методы от BydaoModule
    QString name() const override { return "DirObject"; }  // не модуль, но метод нужен
    QString version() const override { return "1.0.0"; }

    // Доступ к директории
    QString path() const { return m_dir.path(); }
    QString dirName() const { return m_dir.dirName(); }
    bool exists() const { return m_dir.exists(); }

    bool callMethod(const QString& name,
                    const BydaoValueList& args,
                    BydaoValue* result) override;

private:
    // Методы объекта
    bool method_list(const BydaoValueList& args, BydaoValue* result);
    bool method_cd(const BydaoValueList& args, BydaoValue* result);
    bool method_mkdir(const BydaoValueList& args, BydaoValue* result);
    bool method_rmdir(const BydaoValueList& args, BydaoValue* result);
    bool method_exists(const BydaoValueList& args, BydaoValue* result);
    bool method_current(const BydaoValueList& args, BydaoValue* result);

    using MethodPtr = bool (BydaoDirObject::*)(const BydaoValueList&, BydaoValue*);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    QDir m_dir;
};

// Модуль Dir
class BYDAODIR_EXPORT BydaoDirModule : public BydaoModule {

public:
    explicit BydaoDirModule();
    ~BydaoDirModule();

    // Обязательные методы от BydaoObject
    QString typeName() const override { return "DirModule"; }

    // Обязательные методы от BydaoModule
    QString name() const override { return "Dir"; }
    QString version() const override { return "1.0.0"; }

    MetaData*   metaData() override;

    // Получить список используемых мета-данных
    UsedMetaDataList usedMetaData() override;

    bool callMethod(const QString& name,
                    const BydaoValueList& args,
                    BydaoValue* result) override;

protected:
    bool initialize() override;
    bool shutdown() override;

private:
    // Методы модуля
    bool method_open(const BydaoValueList& args, BydaoValue* result);
    bool method_list(const BydaoValueList& args, BydaoValue* result);
    bool method_cd(const BydaoValueList& args, BydaoValue* result);
    bool method_current(const BydaoValueList& args, BydaoValue* result);
    bool method_mkdir(const BydaoValueList& args, BydaoValue* result);
    bool method_rmdir(const BydaoValueList& args, BydaoValue* result);

    using MethodPtr = bool (BydaoDirModule::*)(const BydaoValueList&, BydaoValue*);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов
};

} // namespace Modules
} // namespace BydaoScript
