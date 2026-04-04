#pragma once

#include "../../../include/BydaoScript/BydaoModule.h"
#include "../../../include/BydaoScript/BydaoMetaData.h"
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

public:
    explicit BydaoSysModule();
    ~BydaoSysModule();

    // Обязательные методы от BydaoObject
    QString typeName() const override { return "SysModule"; }

    // Обязательные методы от BydaoModule
    QString name() const override { return "Sys"; }
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
    bool method_out(const BydaoValueList& args, BydaoValue* result);
    bool method_outln(const BydaoValueList& args, BydaoValue* result);
    bool method_in(const BydaoValueList& args, BydaoValue* result);
    bool method_err(const BydaoValueList& args, BydaoValue* result);
    bool method_errln(const BydaoValueList& args, BydaoValue* result);
    bool method_run(const BydaoValueList& args, BydaoValue* result);
    bool method_exec(const BydaoValueList& args, BydaoValue* result);
    bool method_shell(const BydaoValueList& args, BydaoValue* result);
    bool method_getenv(const BydaoValueList& args, BydaoValue* result);
    bool method_setenv(const BydaoValueList& args, BydaoValue* result);
    bool method_unsetenv(const BydaoValueList& args, BydaoValue* result);
    bool method_env(const BydaoValueList& args, BydaoValue* result);
    bool method_sleep(const BydaoValueList& args, BydaoValue* result);
    bool method_time(const BydaoValueList& args, BydaoValue* result);
    bool method_date(const BydaoValueList& args, BydaoValue* result);
    bool method_datetime(const BydaoValueList& args, BydaoValue* result);
    bool method_platform(const BydaoValueList& args, BydaoValue* result);
    bool method_currentDir(const BydaoValueList& args, BydaoValue* result);
    bool method_setCurrentDir(const BydaoValueList& args, BydaoValue* result);
    bool method_tempDir(const BydaoValueList& args, BydaoValue* result);
    bool method_homeDir(const BydaoValueList& args, BydaoValue* result);
    bool method_drives(const BydaoValueList& args, BydaoValue* result);

    using MethodPtr = bool (BydaoSysModule::*)(const BydaoValueList&, BydaoValue*);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    QProcess* m_process;
    QElapsedTimer m_timer;
    QTextStream*    m_outStream;
};

} // namespace Modules
} // namespace BydaoScript
