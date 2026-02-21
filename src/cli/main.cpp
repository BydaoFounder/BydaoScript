#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QTextStream>

// +++ ПРАВИЛЬНЫЕ ЗАГОЛОВКИ +++
#include "BydaoScript/BydaoLexer.h"
#include "BydaoScript/BydaoParser.h"
#include "BydaoScript/BydaoVM.h"
#include "BydaoScript/BydaoBytecode.h"
#include "BydaoScript/BydaoModule.h"
#include "BydaoScript/BydaoDisassembler.h"

// Модули
//#include "BydaoScript/modules/BydaoDir.h"
//#include "BydaoScript/modules/BydaoFile.h"
//#include "BydaoScript/modules/BydaoSys.h"

// +++ ИСПОЛЬЗОВАТЬ ПРОСТРАНСТВО ИМЕН +++
using namespace BydaoScript;
//using namespace BydaoScript::Modules;

int main(int argc, char *argv[]) {

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("BydaoScript");
    QCoreApplication::setApplicationVersion("1.0.0");

    // Устанавливаем кодировку для QString -> консоль
    QTextStream out(stdout);
    out.setEncoding(QStringConverter::Utf8);

    QCommandLineParser parser;
    parser.setApplicationDescription("BydaoScript Interpreter");
    parser.addHelpOption();
    parser.addVersionOption();
    
    parser.addPositionalArgument("file", "Script file to execute");
    
    QCommandLineOption bytecodeOption("c", "Compile to bytecode only");
    parser.addOption(bytecodeOption);
    
    QCommandLineOption outputOption("o", "Output bytecode file", "file");
    parser.addOption(outputOption);

    QCommandLineOption traceOption( QStringList() << "t" << "trace", "Show execution trace");
    parser.addOption(traceOption);

    QCommandLineOption listingOption( QStringList() << "l" << "list", "Show bytecode listing");
    parser.addOption(listingOption);

    parser.process(app);
    
    QTextStream cout(stdout);
    QString eol = "\r\n";

    QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        cout << "BydaoScript 1.0.0 - Interactive mode not implemented yet" << eol;
        return 0;
    }
    
    QString fileName = args.first();
    QFile file(fileName);
    
    if (!file.open(QIODevice::ReadOnly)) {
        cout << "Cannot open file:" << fileName << eol;
        return 1;
    }
    
    QString source = QString::fromUtf8(file.readAll());
    file.close();
    
    // 1. Лексический анализ +++ BydaoLexer +++
    BydaoLexer lexer(source);
    auto tokens = lexer.tokenize();
    
    if (!lexer.errorMessage().isEmpty()) {
        cout << "Lexer error:" << lexer.errorMessage() << eol;
        return 1;
    }
    
    // 2. Парсинг + генерация байт-кода +++ BydaoParser +++
    BydaoParser parserCore(tokens);
    if (!parserCore.parse()) {
        for (const auto& err : parserCore.errors()) {
            cout << err << eol;
        }
        return 1;
    }
    
    auto bytecode = parserCore.takeBytecode();
    
    // qDebug() << "=== BYTECODE ===";
    // for (int i = 0; i < bytecode.size(); i++) {
    //     qDebug() << i << ":" << (int)bytecode[i].op << bytecode[i].arg;
    // }

    // Дизассемблировать если нужно
    if (parser.isSet(listingOption)) {
        BydaoDisassembler disasm;
//        disasm.setColorOutput(true); // для цветного вывода в терминале
        disasm.setColorOutput(false); // для цветного вывода в терминале
        QTextStream out(stdout);
        out << "\n=== BYTECODE DISASSEMBLY ===\n";
        disasm.disassemble(bytecode, out);
        out << "============================\n\n";
    }

    // Сохраняем байт-код если нужно +++ BydaoBytecode +++
    if (parser.isSet(outputOption)) {
        QString outFile = parser.value(outputOption);
        if (BydaoBytecode::save(bytecode, outFile)) {
            cout << "Bytecode saved to:" << outFile << eol;
        } else {
            cout << "Cannot save bytecode" << eol;
            return 1;
        }
    }
    
    // Только компиляция?
    if (parser.isSet(bytecodeOption)) {
        cout << "Compilation successful" << eol;
        return 0;
    }
    
    // 3. Выполнение +++ BydaoVM +++
    BydaoVM vm;
    
    vm.setTraceMode( parser.isSet(traceOption) );

    if (!vm.load(bytecode)) {
        cout << "Cannot load bytecode" << eol;
        return 1;
    }
    if (!vm.run()) {
        // TODO: вывести ошибку выполнения
        return 1;
    }
    
    return 0;
}
