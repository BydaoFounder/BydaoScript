#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QTextStream>
#include <QElapsedTimer>

#include "BydaoScript/BydaoLexer.h"
#include "BydaoScript/BydaoParser.h"
#include "BydaoScript/BydaoVM.h"
#include "BydaoScript/BydaoBytecode.h"
#include "BydaoScript/BydaoDisassembler.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

using namespace BydaoScript;

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
    setlocale(LC_ALL, "");
    SetConsoleOutputCP(CP_UTF8);
#endif

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("BydaoScript");
    QCoreApplication::setApplicationVersion("2.0.0");

    QTextStream out(stdout);
    out.setEncoding(QStringConverter::Utf8);

    QCommandLineParser cmdLineParser;
    cmdLineParser.setApplicationDescription("BydaoScript Interpreter v2.0 (Optimized Bytecode)");
    cmdLineParser.addHelpOption();
    cmdLineParser.addVersionOption();

    cmdLineParser.addPositionalArgument("file", "Script file to execute");

    QCommandLineOption bytecodeOption("c", "Compile to bytecode only");
    cmdLineParser.addOption(bytecodeOption);

    QCommandLineOption outputOption(QStringList() << "o" << "out", "Output bytecode file", "file");
    cmdLineParser.addOption(outputOption);

    QCommandLineOption traceOption(QStringList() << "t" << "trace", "Show execution trace");
    cmdLineParser.addOption(traceOption);

    QCommandLineOption listingOption(QStringList() << "l" << "list", "Show bytecode listing");
    cmdLineParser.addOption(listingOption);

    QCommandLineOption profileOption(QStringList() << "p" << "profile", "Profile bytecode execution");
    cmdLineParser.addOption(profileOption);

    QCommandLineOption timeOption(QStringList() << "time", "Show execution time");
    cmdLineParser.addOption(timeOption);

    cmdLineParser.process(app);

    QString eol = "\r\n";
    QStringList args = cmdLineParser.positionalArguments();

    if (args.isEmpty()) {
        out << "BydaoScript 2.0.0 - Interactive mode not implemented yet" << eol;
        return 0;
    }

    QString fileName = args.first();
    if (!QFile::exists(fileName)) {
        if (QFile::exists(fileName + ".bds")) {
            fileName += ".bds";
        } else {
            out << "File not found: " << fileName << eol;
            return 1;
        }
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        out << "Cannot open file: " << fileName << eol;
        return 1;
    }

    QString source = QString::fromUtf8(file.readAll());
    file.close();

    QElapsedTimer totalTimer;
    totalTimer.start();

    // 1. Лексический анализ
    BydaoLexer lexer(source);
    auto tokens = lexer.tokenize();

    if (!lexer.errorMessage().isEmpty()) {
        out << "Lexer error: " << lexer.errorMessage() << eol;
        return 1;
    }

    // 2. Парсинг + генерация байт-кода
    BydaoParser parser(tokens);
    if (!parser.parse()) {
        for (const auto& err : parser.errors()) {
            out << err << eol;
        }
        return 1;
    }

    auto bytecode = parser.takeBytecode();
    auto constants = parser.takeConstants();
    auto stringTable = parser.takeStringTable();
    auto debugInfo = parser.takeDebugInfo();

    // Дизассемблирование
    if (cmdLineParser.isSet(listingOption)) {
        out << "\n=== BYTECODE DISASSEMBLY ===\n";
        BydaoDisassembler da;
        out << da.disassemble(constants,stringTable,bytecode);
        out << "============================\n\n";
        
        return 0;
    }

    // Сохранение байт-кода
    if (cmdLineParser.isSet(outputOption)) {
        QString outFile = cmdLineParser.value(outputOption);
        if (BydaoBytecode::save(constants, stringTable, bytecode, outFile, &debugInfo)) {
            out << "Bytecode saved to: " << outFile << eol;
        } else {
            out << "Cannot save bytecode" << eol;
            return 1;
        }
    }

    // Только компиляция
    if (cmdLineParser.isSet(bytecodeOption)) {
        out << "Compilation successful" << eol;
        return 0;
    }

    // 3. Выполнение
    BydaoVM vm;
    vm.setTraceMode(cmdLineParser.isSet(traceOption));
    vm.setProfileMode(cmdLineParser.isSet(profileOption));

    if (!vm.load(constants, stringTable, bytecode)) {
        out << "Cannot load bytecode" << eol;
        return 1;
    }

    qint64 execTime = 0;
    if (cmdLineParser.isSet(timeOption)) {
        QElapsedTimer execTimer;
        execTimer.start();
        
        if (!vm.run()) {
            out << "Runtime error: " << vm.lastError() << eol;
            return 1;
        }
        
        execTime = execTimer.elapsed();
    } else {
        if (!vm.run()) {
            out << "Runtime error: " << vm.lastError() << eol;
            return 1;
        }
    }

    // Вывод профиля
    if (cmdLineParser.isSet(profileOption)) {
        auto profile = vm.takeProfile();
        qint64 total = 0;
        
        out << "\n======= Start profile ======\n";
        for (const auto& item : profile) {
            double ms = item.time / 1000000.0;
            out << QString("%1 %2 ms (%3 calls)\n")
                       .arg(item.name, -12)
                       .arg(ms, 8, 'f', 3)
                       .arg(item.count, 6);
            total += item.time;
        }
        out << "----------------------------\n";
        out << QString("Total       %1 ms\n").arg(total / 1000000.0, 8, 'f', 3);
        out << "======= End profile ========\n";
    }

    // Вывод времени выполнения
    if (cmdLineParser.isSet(timeOption)) {
        out << QString("\nExecution time: %1 ms\n").arg(execTime);
        out << QString("Total time (incl. parsing): %1 ms\n").arg(totalTimer.elapsed());
    }

    return 0;
}
