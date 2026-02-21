#include "BydaoScript/BydaoBytecode.h"
#include <QFile>
#include <QDataStream>
#include <QHash>

namespace BydaoScript {

static const quint32 BYDAO_MAGIC = 0x42594453;
static const quint32 BYDAO_VERSION = 0x00010000;

QString BydaoBytecode::opcodeToString(BydaoOpCode op) {
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
        names[BydaoOpCode::Mod] = "MOD";
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

bool BydaoBytecode::save(const QVector<BydaoInstruction>& code, const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QDataStream ds(&file);
    ds.setVersion(QDataStream::Qt_6_0);
    ds << BYDAO_MAGIC;
    ds << BYDAO_VERSION;
    ds << (quint32)code.size();

    for (const auto& i : code) {
        ds << (quint8)i.op;
        ds << i.arg;
        ds << (qint32)i.line;
        ds << (qint32)i.column;
    }
    return true;
}

QVector<BydaoInstruction> BydaoBytecode::load(const QString& filename, QString* error) {
    QVector<BydaoInstruction> result;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = "Cannot open file";
        return result;
    }

    QDataStream ds(&file);
    ds.setVersion(QDataStream::Qt_6_0);
    quint32 magic, version, count;
    ds >> magic >> version >> count;

    if (magic != BYDAO_MAGIC) {
        if (error) *error = "Invalid bytecode format";
        return result;
    }

    for (quint32 i = 0; i < count; ++i) {
        quint8 op; QString arg; qint32 line, col;
        ds >> op >> arg >> line >> col;
        result.append(BydaoInstruction((BydaoOpCode)op, arg, line, col));
    }
    return result;
}

} // namespace BydaoScript
