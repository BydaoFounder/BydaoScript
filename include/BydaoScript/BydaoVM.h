#pragma once

#include "BydaoBytecode.h"
#include "BydaoObject.h"
#include "BydaoValue.h"
#include <QStack>
#include <QHash>
#include <QMetaMethod>

namespace BydaoScript {

class BydaoVM {
public:
    BydaoVM();
    ~BydaoVM();

    bool load(const QVector<BydaoInstruction>& code);
    bool run();
    void stop();

    void registerGlobal(const QString& name, BydaoObject* obj);

    void setTraceMode(bool enable);

private:
    bool execute(const BydaoInstruction& instr);
    void error(const QString& msg, const BydaoInstruction& instr);

    bool callMethod(BydaoValue& obj, const QString& method, const QVector<BydaoValue>& args);

    QVector<BydaoInstruction> m_code;
    int m_pc;
    bool m_running;

    QStack<BydaoValue> m_stack;

    struct Scope {                          // область видимости
        QHash<QString, BydaoValue> vars;    // переменные в этой области
    };
    QStack<Scope> m_scopeStack;             // стек областей видимости

    QString m_lastError;
    int m_errorLine;

    bool m_traceMode = false;
};

} // namespace BydaoScript
