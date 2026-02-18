#pragma once

#include "../../../include/BydaoScript/BydaoModule.h"
#include <QDir>

#ifdef BYDAODIR_LIBRARY
#include "BydaoDir_global.h"
#else
#define BYDAODIR_EXPORT
#endif

namespace BydaoScript {
namespace Modules {

// Объект директории
class BYDAODIR_EXPORT BydaoDirObject : public BydaoModule {
    Q_OBJECT
    // Q_PROPERTY(QString path READ path)
    // Q_PROPERTY(QString name READ name)

public:
    explicit BydaoDirObject(const QString& path = QString(), QObject* parent = nullptr);

    // Обязательные методы от BydaoObject
    QString typeName() const override { return "DirObject"; }

    // Обязательные методы от BydaoModule
    QString name() const override { return "DirObject"; }  // не модуль, но метод нужен
    QString version() const override { return "1.0.0"; }
    BydaoModuleInfo* info() const override { return nullptr; }  // у объекта нет info

    bool getProperty(const QString& name, BydaoValue& result) override;

private:
    // Методы объекта
    bool method_list(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_cd(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_mkdir(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_rmdir(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_exists(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_current(const QVector<BydaoValue>& args, BydaoValue& result);

    QDir m_dir;
};

// Модуль Dir
class BYDAODIR_EXPORT BydaoDirModule : public BydaoModule {
    Q_OBJECT

public:
    explicit BydaoDirModule(QObject* parent = nullptr);
    ~BydaoDirModule();

    // Обязательные методы от BydaoObject
    QString typeName() const override { return "DirModule"; }

    // Обязательные методы от BydaoModule
    QString name() const override { return "Dir"; }
    QString version() const override { return "1.0.0"; }
    BydaoModuleInfo* info() const override;

protected:
    bool initialize() override;
    bool shutdown() override;

private:
    // Методы модуля
    bool method_open(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_list(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_cd(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_current(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_mkdir(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_rmdir(const QVector<BydaoValue>& args, BydaoValue& result);

    static BydaoModuleInfoImpl* createInfo();
    static BydaoModuleInfoImpl* s_info;
};

} // namespace Modules
} // namespace BydaoScript
