#include "BydaoScript/BydaoDisassembler.h"
#include <QHash>

namespace BydaoScript {

// ANSI color codes
const QString BydaoDisassembler::RESET = "\033[0m";
const QString BydaoDisassembler::RED = "\033[31m";
const QString BydaoDisassembler::GREEN = "\033[32m";
const QString BydaoDisassembler::YELLOW = "\033[33m";
const QString BydaoDisassembler::BLUE = "\033[34m";
const QString BydaoDisassembler::MAGENTA = "\033[35m";
const QString BydaoDisassembler::CYAN = "\033[36m";

BydaoDisassembler::BydaoDisassembler() {}

QString BydaoDisassembler::opcodeToString(BydaoOpCode op) {
    static QHash<BydaoOpCode, QString> names;
    if (names.isEmpty()) {
        names[BydaoOpCode::Nop] = "NOP";
        names[BydaoOpCode::Halt] = "HALT";
        names[BydaoOpCode::VarDecl] = "VARDECL";
        names[BydaoOpCode::Load] = "LOAD";
        names[BydaoOpCode::Store] = "STORE";
        names[BydaoOpCode::PushNull] = "PUSHNULL";
        names[BydaoOpCode::PushInt] = "PUSHINT";
        names[BydaoOpCode::PushReal] = "PUSHREAL";
//        names[BydaoOpCode::PushBool] = "PUSHBOOL";
        names[BydaoOpCode::PushString] = "PUSHSTRING";
        names[BydaoOpCode::PushArray] = "PUSHARRAY";
        names[BydaoOpCode::Add] = "ADD";
        names[BydaoOpCode::Sub] = "SUB";
        names[BydaoOpCode::Mul] = "MUL";
        names[BydaoOpCode::Div] = "DIV";
        names[BydaoOpCode::Neg] = "NEG";
        names[BydaoOpCode::Eq] = "EQ";
        names[BydaoOpCode::Neq] = "NEQ";
        names[BydaoOpCode::Lt] = "LT";
        names[BydaoOpCode::Gt] = "GT";
        names[BydaoOpCode::Le] = "LE";
        names[BydaoOpCode::Ge] = "GE";
        names[BydaoOpCode::And] = "AND";
        names[BydaoOpCode::Or] = "OR";
        names[BydaoOpCode::Not] = "NOT";
        names[BydaoOpCode::Member] = "MEMBER";
        names[BydaoOpCode::Index] = "INDEX";
        names[BydaoOpCode::Call] = "CALL";
        names[BydaoOpCode::Jump] = "JUMP";
        names[BydaoOpCode::JumpIfFalse] = "JMPF";
        names[BydaoOpCode::JumpIfTrue] = "JMPT";
        names[BydaoOpCode::Break] = "BREAK";
        names[BydaoOpCode::Next] = "NEXT";
        names[BydaoOpCode::Label] = "LABEL";
        names[BydaoOpCode::ScopeBegin] = "SCOPEBEG";
        names[BydaoOpCode::ScopeEnd] = "SCOPEEND";
        names[BydaoOpCode::UseModule] = "USE";
    }
    return names.value(op, "???");
}

QString BydaoDisassembler::formatArg(const BydaoInstruction& instr) {
    if (instr.arg.isEmpty()) return QString();
    
    switch (instr.op) {
        case BydaoOpCode::PushString:
            return "'" + instr.arg + "'";
        case BydaoOpCode::UseModule:
            return instr.arg;
        default:
            return instr.arg;
    }
}

QString BydaoDisassembler::formatInstruction(int index, const BydaoInstruction& instr) {
    QString result;
    
    if (m_showOffsets) {
        if (m_colorOutput)
            result += CYAN + QString("[%1]").arg(index, 4) + RESET + " ";
        else
            result += QString("[%1] ").arg(index, 4);
    }
    
    if (m_showLineNumbers) {
        if (m_colorOutput)
            result += YELLOW + QString("(%1:%2)").arg(instr.line).arg(instr.column) + RESET + " ";
        else
            result += QString("(%1:%2) ").arg(instr.line).arg(instr.column);
    }
    
    // Имя инструкции
    if (m_colorOutput)
        result += GREEN + opcodeToString(instr.op) + RESET;
    else
        result += opcodeToString(instr.op);
    
    // Аргумент
    QString arg = formatArg(instr);
    if (!arg.isEmpty()) {
        if (m_colorOutput)
            result += " " + BLUE + arg + RESET;
        else
            result += " " + arg;
    }
    
    return result;
}

QString BydaoDisassembler::disassemble(const QVector<BydaoInstruction>& code) {
    QString result;
    QTextStream out(&result);
    disassemble(code, out);
    return result;
}

void BydaoDisassembler::disassemble(const QVector<BydaoInstruction>& code, QTextStream& out) {
    for (int i = 0; i < code.size(); i++) {
        out << formatInstruction(i, code[i]) << Qt::endl;
    }
}

} // namespace BydaoScript
