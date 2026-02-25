#pragma once

#include <QString>
#include <QVector>
#include <QMetaType>

namespace BydaoScript {

//enum class BydaoOpCode : quint8 {
enum BydaoOpCode : quint8 {
    Nop, Halt,
    VarDecl, Drop, Load, Store,
    PushNull, PushInt, PushReal, PushString, PushArray,
    PushFalse, PushTrue,
    Add, Sub, Neg,
    Mul, Div, Mod,
    Eq, Neq, Lt, Gt, Le, Ge,
    And, Or, Not,
    Member, Index, Call,
    Jump, JumpIfFalse, JumpIfTrue,
    Label,
    ScopeBegin, ScopeEnd,
    UseModule
};

struct BydaoInstruction {
    BydaoOpCode op;
    QString arg;
    int line;
    int column;

    BydaoInstruction(BydaoOpCode o = BydaoOpCode::Nop, QString a = "", int l = 0, int c = 0)
        : op(o), arg(a), line(l), column(c) {}
};

class BydaoBytecode {
public:

    static QString opcodeToString(BydaoOpCode op);

    static bool save(const QVector<BydaoInstruction>& code, const QString& filename);
    static QVector<BydaoInstruction> load(const QString& filename, QString* error = nullptr);
};

} // namespace BydaoScript

Q_DECLARE_METATYPE(BydaoScript::BydaoInstruction)
