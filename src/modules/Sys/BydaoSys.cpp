#include "BydaoSys.h"
#include "../include/BydaoScript/BydaoString.h"
#include "../include/BydaoScript/BydaoInt.h"
#include "../include/BydaoScript/BydaoBool.h"
#include "../include/BydaoScript/BydaoNull.h"
#include "../include/BydaoScript/BydaoArray.h"
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QThread>
#include <QProcessEnvironment>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace BydaoScript {
namespace Modules {

BydaoModuleInfoImpl* BydaoSysModule::createInfo() {
    auto* info = new BydaoModuleInfoImpl();
    info->m_name = "Sys";
    info->m_version = "1.0.0";

    info->m_methods = {
        {"out",         {"text"}},
        {"outln",       {"text"}},
        {"in",          {"prompt"}},
        {"err",         {"text"}},
        {"errln",       {"text"}},
        {"run",         {"command"}},
        {"exec",        {"program", "args"}},
        {"shell",       {"command"}},
        {"getenv",      {"name"}},
        {"setenv",      {"name", "value"}},
        {"unsetenv",    {"name"}},
        {"env",         {}},
        {"sleep",       {"ms"}},
        {"time",        {}},
        {"date",        {}},
        {"datetime",    {}},
        {"platform",    {}},
        {"currentDir",  {}},
        {"setCurrentDir", {"path"}},
        {"tempDir",     {}},
        {"homeDir",     {}},
        {"drives",      {}}
    };

    return info;
}

BydaoModuleInfo* BydaoSysModule::info() const {
    static BydaoModuleInfoImpl* s_info = nullptr;  // статический внутри функции
    if (!s_info) {
        s_info = createInfo();
    }
    return s_info;
}

// ========== BydaoSysModule ==========

#define REGISTER_MODULE_METHOD(name) \
registerMethod(#name, [this](const QVector<BydaoValue>& args, BydaoValue& result) { \
        return method_##name(args, result); \
})

BydaoSysModule::BydaoSysModule(QObject* parent)
    : BydaoModule(parent)
    , m_process(nullptr)
{
    registerMethod("out",     &BydaoSysModule::method_out);
    registerMethod("outln",   &BydaoSysModule::method_outln);
    registerMethod("in",      &BydaoSysModule::method_in);
    registerMethod("err",     &BydaoSysModule::method_err);
    registerMethod("errln",   &BydaoSysModule::method_errln);
    registerMethod("run",     &BydaoSysModule::method_run);
    registerMethod("exec",     &BydaoSysModule::method_exec);
    registerMethod("shell",     &BydaoSysModule::method_shell);
    registerMethod("getenv",     &BydaoSysModule::method_getenv);
    registerMethod("setenv",     &BydaoSysModule::method_setenv);
    registerMethod("unsetenv",     &BydaoSysModule::method_unsetenv);
    registerMethod("env",     &BydaoSysModule::method_env);
    registerMethod("sleep",     &BydaoSysModule::method_sleep);
    registerMethod("time",     &BydaoSysModule::method_time);
    registerMethod("date",     &BydaoSysModule::method_date);
    registerMethod("datetime",     &BydaoSysModule::method_datetime);
    registerMethod("platform",     &BydaoSysModule::method_platform);
    registerMethod("currentDir",     &BydaoSysModule::method_currentDir);
    registerMethod("setCurrentDir",     &BydaoSysModule::method_setCurrentDir);
    registerMethod("tempDir",     &BydaoSysModule::method_tempDir);
    registerMethod("homeDir",     &BydaoSysModule::method_homeDir);
    registerMethod("drives",     &BydaoSysModule::method_drives);

    m_timer.start();
    m_outStream = new QTextStream( stdout );
    m_outStream->setEncoding( QStringConverter::Utf8 );
}

BydaoSysModule::~BydaoSysModule() {
//    qDebug() << "~BydaoSysModule()";
}

bool BydaoSysModule::initialize() {
    m_process = new QProcess(this);
    return true;
}

bool BydaoSysModule::shutdown() {
    // qDebug() << "BydaoSysModule::shutdown()";
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
    return true;
}

void BydaoSysModule::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoSysModule::callMethod(const QString& name,
                                 const QVector<BydaoValue>& args,
                                 BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

// ========== Методы модуля (С КВАЛИФИКАТОРОМ КЛАССА) ==========

bool BydaoSysModule::method_out(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QTextStream& out = *m_outStream;
    out << args[0].toString();
    out.flush();

    result = BydaoValue();
    return true;
}

bool BydaoSysModule::method_outln(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() > 1) return false;

    if (args.size() == 1) {
        // Выводим текст
        QTextStream& out = *m_outStream;
        out << args[0].toString();
    }

    // В любом случае добавляем перевод строки
    QTextStream& out = *m_outStream;
    out << Qt::endl;
    out.flush();

    result = BydaoValue(BydaoNull::instance());
    return true;
}

bool BydaoSysModule::method_in(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() > 1) return false;

    if (args.size() == 1) {
        QTextStream out(stdout);
#ifdef Q_OS_WIN
        out.setEncoding(QStringConverter::Utf8);
#endif
        out << args[0].toString();
        out.flush();
    }

    QTextStream in(stdin);
    QString line = in.readLine();

    result = BydaoValue( BydaoString::create(line) );
    return true;
}

bool BydaoSysModule::method_err(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QTextStream err(stderr);
#ifdef Q_OS_WIN
    err.setEncoding(QStringConverter::Utf8);
#endif
    err << args[0].toString();
    err.flush();

    result = BydaoValue();
    return true;
}

bool BydaoSysModule::method_errln(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QTextStream err(stderr);
#ifdef Q_OS_WIN
    err.setEncoding(QStringConverter::Utf8);
#endif
    err << args[0].toString() << Qt::endl;

    result = BydaoValue();
    return true;
}

bool BydaoSysModule::method_run(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    int code = system(args[0].toString().toLocal8Bit().constData());
    result = BydaoValue( BydaoInt::create(code) );
    return true;
}

bool BydaoSysModule::method_exec(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    if (!m_process) {
        m_process = new QProcess(this);
    }

    QString program = args[0].toString();
    QStringList arguments;

    // Если есть второй аргумент - это должен быть массив аргументов
    if (args.size() == 2) {
        if (args[1].typeId() != TYPE_ARRAY) {
            return false;
        }

        auto* array = dynamic_cast<BydaoArray*>(args[1].toObject());
        if (!array) return false;

        for (int i = 0; i < array->size(); i++) {
            arguments << array->at(i).toString();
        }
    }

    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }

    m_process->start(program, arguments);
    m_process->waitForFinished(-1);

    result = BydaoValue(BydaoInt::create(m_process->exitCode()));
    return true;
}

bool BydaoSysModule::method_shell(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    if (!m_process) {
        m_process = new QProcess(this);
    }

    m_process->startCommand(args[0].toString());
    m_process->waitForFinished(-1);

    QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    result = BydaoValue( BydaoString::create( output ) );
    return true;
}

bool BydaoSysModule::method_getenv(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString value = qgetenv(args[0].toString().toLatin1().constData());
    result = BydaoValue( BydaoString::create( value ) );
    return true;
}

bool BydaoSysModule::method_setenv(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;

#ifdef Q_OS_WIN
    bool ok = SetEnvironmentVariableW(
        (LPCWSTR)args[0].toString().utf16(),
        (LPCWSTR)args[1].toString().utf16());
#else
    bool ok = setenv(
                  args[0].toString().toLatin1().constData(),
                  args[1].toString().toLatin1().constData(),
                  1) == 0;
#endif

    result = BydaoValue( BydaoBool::create(ok) );
    return true;
}

bool BydaoSysModule::method_unsetenv(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

#ifdef Q_OS_WIN
    bool ok = SetEnvironmentVariableW(
        (LPCWSTR)args[0].toString().utf16(),
        nullptr);
#else
    bool ok = unsetenv(args[0].toString().toLatin1().constData()) == 0;
#endif

    result = BydaoValue( BydaoBool::create(ok) );
    return true;
}

bool BydaoSysModule::method_env(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList keys = env.keys();

    auto* array = new BydaoArray();

    for (const QString& key : keys) {
        QString entry = key + "=" + env.value(key);
        array->append(BydaoValue(BydaoString::create(entry)));
    }

    result = BydaoValue(array);
    return true;
}

bool BydaoSysModule::method_sleep(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QThread::msleep( (unsigned long) args[0].toInt());
    result = BydaoValue();
    return true;
}

bool BydaoSysModule::method_time(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoInt::create(QDateTime::currentMSecsSinceEpoch()));
    return true;
}

bool BydaoSysModule::method_date(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoString::create( QDate::currentDate().toString(Qt::ISODate)) );
    return true;
}

bool BydaoSysModule::method_datetime(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoString::create( QDateTime::currentDateTime().toString(Qt::ISODate)) );
    return true;
}

bool BydaoSysModule::method_platform(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);

#ifdef Q_OS_WIN
    result = BydaoValue( BydaoString::create("windows") );
#elif Q_OS_MAC
    result = BydaoValue( BydaoString::create("macos") );
#elif Q_OS_LINUX
    result = BydaoValue( BydaoString::create("linux") );
#else
    result = BydaoValue( BydaoString::create("unknown") );
#endif
    return true;
}

bool BydaoSysModule::method_currentDir(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoString::create(QDir::currentPath()) );
    return true;
}

bool BydaoSysModule::method_setCurrentDir(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    bool ok = QDir::setCurrent(args[0].toString());
    result = BydaoValue( BydaoBool::create(ok) );
    return true;
}

bool BydaoSysModule::method_tempDir(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoString::create(QDir::tempPath()));
    return true;
}

bool BydaoSysModule::method_homeDir(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoString::create(QDir::homePath()) );
    return true;
}

bool BydaoSysModule::method_drives(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);

    auto* array = new BydaoArray();

#ifdef Q_OS_WIN
    // Получаем список логических дисков в Windows
    DWORD drives = GetLogicalDrives();
    for (char c = 'A'; c <= 'Z'; c++) {
        if (drives & 1) {
            QString drive = QString(c) + ":";
            array->append(BydaoValue(BydaoString::create(drive)));
        }
        drives >>= 1;
    }
#else
    // На Unix-like системах корневая директория
    array->append(BydaoValue(BydaoString::create("/")));
#endif

    result = BydaoValue(array);
    return true;
}

// ========== Экспорт модуля ==========

extern "C" {
MODULE_EXPORT BydaoScript::BydaoModule* createModule() {
    return new BydaoSysModule();
}

MODULE_EXPORT void destroyModule(BydaoScript::BydaoModule* module) {
    delete module;
}
}

} // namespace Modules
} // namespace BydaoScript
