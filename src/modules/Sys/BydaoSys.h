#pragma once

#include "../../../include/BydaoScript/BydaoModule.h"
#include <QProcess>
#include <QElapsedTimer>

#ifdef BYDAOSYS_LIBRARY
#include "BydaoSys_global.h"
#else
#define BYDAOSYS_EXPORT
#endif

namespace BydaoScript {
namespace Modules {

class BYDAOSYS_EXPORT BydaoSysModule : public BydaoModule {
    Q_OBJECT

public:
    explicit BydaoSysModule(QObject* parent = nullptr);
    ~BydaoSysModule();

    // Обязательные методы от BydaoObject
    QString typeName() const override { return "SysModule"; }

    // Обязательные методы от BydaoModule
    QString name() const override { return "Sys"; }
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
    bool method_out(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_outln(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_in(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_err(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_errln(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_run(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_exec(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_shell(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_getenv(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_setenv(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_unsetenv(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_env(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_sleep(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_time(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_date(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_datetime(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_platform(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_currentDir(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_setCurrentDir(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_tempDir(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_homeDir(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_drives(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoSysModule::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    static BydaoModuleInfoImpl* createInfo();

    QProcess* m_process;
    QElapsedTimer m_timer;
    QTextStream*    m_outStream;
};

} // namespace Modules
} // namespace BydaoScript
