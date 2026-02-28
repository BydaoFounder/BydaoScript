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
    SetConsoleOutputCP(CP_UTF8);
#endif

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("BydaoScript");
    QCoreApplication::setApplicationVersion("2.0.0");

    QTextStream out(stdout);
    out.setEncoding(QStringConverter::Utf8);

    QCommandLineParser parser;
    parser.setApplicationDescription("BydaoScript Interpreter v2.0 (Optimized Bytecode)");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("file", "Script file to execute");

    QCommandLineOption bytecodeOption("c", "Compile to bytecode only");
    parser.addOption(bytecodeOption);

    QCommandLineOption outputOption(QStringList() << "o" << "out", "Output bytecode file", "file");
    parser.addOption(outputOption);

    QCommandLineOption traceOption(QStringList() << "t" << "trace", "Show execution trace");
    parser.addOption(traceOption);

    QCommandLineOption listingOption(QStringList() << "l" << "list", "Show bytecode listing");
    parser.addOption(listingOption);

    QCommandLineOption profileOption(QStringList() << "p" << "profile", "Profile bytecode execution");
    parser.addOption(profileOption);

    QCommandLineOption timeOption(QStringList() << "time", "Show execution time");
    parser.addOption(timeOption);

    parser.process(app);

    QString eol = "\r\n";
    QStringList args = parser.positionalArguments();

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
    BydaoParser parserCore(tokens);
    if (!parserCore.parse()) {
        for (const auto& err : parserCore.errors()) {
            out << err << eol;
        }
        return 1;
    }

    auto bytecode = parserCore.takeBytecode();
    auto constants = parserCore.takeConstants();
    auto stringTable = parserCore.takeStringTable();
    auto debugInfo = parserCore.takeDebugInfo();

    // Дизассемблирование
    if (parser.isSet(listingOption)) {
        out << "\n=== BYTECODE DISASSEMBLY ===\n";
        BydaoDisassembler da;
        out << da.disassemble(constants,stringTable,bytecode);
/*
        out << "Constants (" << constants.size() << "):\n";
        for (int i = 0; i < constants.size(); i++) {
            const auto& c = constants[i];
            out << QString("  %1: ").arg(i, 4);
            switch (c.type) {
            case CONST_INT: out << "INT " << c.intValue; break;
            case CONST_REAL: out << "REAL " << c.realValue; break;
            case CONST_STRING: 
                if (c.stringIndex < (quint32)stringTable.size()) {
                    out << "STRING '" << stringTable[c.stringIndex] << "'";
                } else {
                    out << "STRING <invalid>";
                }
                break;
            case CONST_BOOL: out << "BOOL " << (c.boolValue ? "true" : "false"); break;
            case CONST_NULL: out << "NULL"; break;
            }
            out << "\n";
        }

        out << "\nString Table (" << stringTable.size() << "):\n";
        for (int i = 0; i < stringTable.size(); i++) {
            if (!stringTable[i].isEmpty()) {
                out << QString("  %1: '%2'\n").arg(i, 4).arg(stringTable[i]);
            }
        }

        out << "\nCode (" << bytecode.size() << " instructions):\n";
        for (int i = 0; i < bytecode.size(); i++) {
            const auto& instr = bytecode[i];
            QString opName = BydaoBytecode::opcodeToString(instr.op);
            out << QString("  %1: %2").arg(i, 4).arg(opName, -12);
            
            if (instr.arg1 != 0 || instr.arg2 != 0) {
                out << QString(" %1, %2").arg(instr.arg1).arg(instr.arg2);
            }
            
            if (instr.line != 0) {
                out << QString(" (line %1)").arg(instr.line);
            }
            
            out << "\n";
        }
*/
        out << "============================\n\n";
        
        return 0;
    }

    // Сохранение байт-кода
    if (parser.isSet(outputOption)) {
        QString outFile = parser.value(outputOption);
        if (BydaoBytecode::save(constants, stringTable, bytecode, outFile, &debugInfo)) {
            out << "Bytecode saved to: " << outFile << eol;
        } else {
            out << "Cannot save bytecode" << eol;
            return 1;
        }
    }

    // Только компиляция
    if (parser.isSet(bytecodeOption)) {
        out << "Compilation successful" << eol;
        return 0;
    }

    // 3. Выполнение
    BydaoVM vm;
    vm.setTraceMode(parser.isSet(traceOption));
    vm.setProfileMode(parser.isSet(profileOption));

    if (!vm.load(constants, stringTable, bytecode)) {
        out << "Cannot load bytecode" << eol;
        return 1;
    }

    qint64 execTime = 0;
    if (parser.isSet(timeOption)) {
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
    if (parser.isSet(profileOption)) {
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
    if (parser.isSet(timeOption)) {
        out << QString("\nExecution time: %1 ms\n").arg(execTime);
        out << QString("Total time (incl. parsing): %1 ms\n").arg(totalTimer.elapsed());
    }

    return 0;
}
