#include "BydaoScript/BydaoVM.h"
#include "BydaoScript/BydaoNative.h"
#include "BydaoScript/BydaoModule.h"
#include "BydaoScript/BydaoString.h"    // +++
#include "BydaoScript/BydaoInt.h"       // +++
#include "BydaoScript/BydaoBool.h"      // +++
#include "BydaoScript/BydaoReal.h"      // +++
#include "BydaoScript/BydaoArray.h"      // +++
#include "BydaoScript/BydaoNull.h"       // +++
#include <QDebug>

namespace BydaoScript {

// ========== BydaoVM ==========

BydaoVM::BydaoVM() : m_pc(0), m_running(false), m_debugMode(false) {}

BydaoVM::~BydaoVM() {
    qDebug() << "~BydaoVM()";
    // Очищаем ссылки на модули, но не удаляем их
    m_globals.clear();
//    m_globalObjects.clear();
}

bool BydaoVM::load(const QVector<BydaoInstruction>& code) {
    m_code = code;
    m_pc = 0;
    m_stack.clear();
    m_globals.clear();
    return true;
}

void BydaoVM::setDebugMode(bool enable) {
    m_debugMode = enable;
}

bool BydaoVM::run() {

    m_running = true;
    m_pc = 0;

    while (m_running && m_pc >= 0 && m_pc < m_code.size()) {
        const BydaoInstruction& instr = m_code[m_pc++];

        if (m_debugMode) {
            qDebug() << "EXEC:" << m_pc-1 << opcodeToString(instr.op) << instr.arg;
        }

        if (!execute(instr)) {
            return false;
        }
    }

    return true;
}

void BydaoVM::stop() {
    m_running = false;
}

QString BydaoVM::opcodeToString(BydaoOpCode op) {
    static QHash<BydaoOpCode, QString> names;
    if (names.isEmpty()) {
        names[BydaoOpCode::Nop] = "NOP";
        names[BydaoOpCode::Halt] = "HALT";
        names[BydaoOpCode::VarDecl] = "VarDecl";
        names[BydaoOpCode::Load] = "LOAD";
        names[BydaoOpCode::Store] = "STORE";
        names[BydaoOpCode::PushNull] = "PushNull";
        names[BydaoOpCode::PushInt] = "PushInt";
        names[BydaoOpCode::PushReal] = "PushReal";
        names[BydaoOpCode::PushString] = "PushString";
        names[BydaoOpCode::PushArray] = "PushArray";

        names[BydaoOpCode::Add] = "Add";
        names[BydaoOpCode::Sub] = "Sub";
        names[BydaoOpCode::Mul] = "Mul";
        names[BydaoOpCode::Div] = "Div";
        names[BydaoOpCode::Neg] = "Neg";
        names[BydaoOpCode::Eq] = "Eq";
        names[BydaoOpCode::Neq] = "Neq";
        names[BydaoOpCode::Lt] = "Lt";
        names[BydaoOpCode::Gt] = "Gt";
        names[BydaoOpCode::Le] = "Le";
        names[BydaoOpCode::Ge] = "Ge";
        names[BydaoOpCode::And] = "And";
        names[BydaoOpCode::Or] = "Or";
        names[BydaoOpCode::Not] = "Not";

        names[BydaoOpCode::Call] = "CALL";
        names[BydaoOpCode::Member] = "MEMBER";
        names[BydaoOpCode::Index] = "Index";

        names[BydaoOpCode::Jump] = "JUMP";
        names[BydaoOpCode::JumpIfFalse] = "JMPF";
        names[BydaoOpCode::JumpIfTrue] = "JMPT";

        names[BydaoOpCode::Break] = "Break";
        names[BydaoOpCode::Next] = "Next";

        names[BydaoOpCode::UseModule] = "USE";
        names[BydaoOpCode::ScopeBegin] = "SCOPEBEG";
        names[BydaoOpCode::ScopeEnd] = "SCOPEEND";
    }
    return names.value(op, "???");
}

bool BydaoVM::execute(const BydaoInstruction& instr) {
    switch (instr.op) {
    case BydaoOpCode::Halt:
        m_running = false;
        break;

    case BydaoOpCode::ScopeBegin:
        // Начало области видимости - можно игнорировать или использовать для отладки
        break;

    case BydaoOpCode::ScopeEnd:
        // Конец области видимости
        break;

    case BydaoOpCode::VarDecl:
        m_globals[instr.arg] = BydaoValue(BydaoNull::instance());
        break;

    case BydaoOpCode::Load: {
        m_stack.push(m_globals.value(instr.arg));
        break;
    }

    case BydaoOpCode::Store:
        m_globals[instr.arg] = m_stack.pop();
        break;

    case BydaoOpCode::PushNull:
        m_stack.push(BydaoValue(BydaoNull::instance()));
        break;

    case BydaoOpCode::PushInt:
        m_stack.push(BydaoValue(new BydaoInt(instr.arg.toInt())));
        break;

    case BydaoOpCode::PushReal:
        m_stack.push(BydaoValue(new BydaoReal(instr.arg.toDouble())));
        break;

    case BydaoOpCode::PushString:
        m_stack.push(BydaoValue(new BydaoString(instr.arg)));
        break;

    case BydaoOpCode::PushArray: {
        m_stack.push(BydaoValue(new BydaoArray()));
        break;
    }

    case BydaoOpCode::Add: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        // TODO: реализовать сложение для разных типов
        m_stack.push(BydaoValue(BydaoNull::instance()));
        break;
    }

    case BydaoOpCode::Sub: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(BydaoNull::instance()));
        break;
    }

    case BydaoOpCode::Mul: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(BydaoNull::instance()));
        break;
    }

    case BydaoOpCode::Div: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(BydaoNull::instance()));
        break;
    }

    case BydaoOpCode::Neg: {
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(BydaoNull::instance()));
        break;
    }

    case BydaoOpCode::Eq: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        // TODO: сравнение
        m_stack.push(BydaoValue(new BydaoBool(false)));
        break;
    }

    case BydaoOpCode::Neq: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(new BydaoBool(false)));
        break;
    }

    case BydaoOpCode::Lt: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(new BydaoBool(false)));
        break;
    }

    case BydaoOpCode::Gt: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(new BydaoBool(false)));
        break;
    }

    case BydaoOpCode::Le: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(new BydaoBool(false)));
        break;
    }

    case BydaoOpCode::Ge: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(new BydaoBool(false)));
        break;
    }

    case BydaoOpCode::And: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(new BydaoBool(false)));
        break;
    }

    case BydaoOpCode::Or: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(new BydaoBool(false)));
        break;
    }

    case BydaoOpCode::Not: {
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue(new BydaoBool(false)));
        break;
    }

    case BydaoOpCode::Member: {
        // Получаем имя свойства
        QString propName = instr.arg;
        BydaoValue obj = m_stack.pop();

        if (!obj.isObject()) {
            error("Cannot access property of non-object", instr);
            return false;
        }

        if (auto* native = dynamic_cast<BydaoNative*>(obj.toObject())) {
            BydaoValue result;
            if (native->getProperty(propName, result)) {
                m_stack.push(result);
                break;
            }
        }

        // Если свойства нет — кладём объект и имя для Call
        m_stack.push(obj);
        m_stack.push(BydaoValue(new BydaoString(propName)));
        break;
    }

    case BydaoOpCode::Index: {
        BydaoValue index = m_stack.pop();
        BydaoValue obj = m_stack.pop();
        // TODO: реализовать индексацию
        m_stack.push(BydaoValue(BydaoNull::instance()));
        break;
    }

    case BydaoOpCode::Call: {
        int argCount = instr.arg.toInt();

        QVector<BydaoValue> args;
        for (int i = 0; i < argCount; i++) {
            args.prepend(m_stack.pop());
        }

        QString methodName = m_stack.pop().toString();
        BydaoValue obj = m_stack.pop();

        if (!obj.isObject()) {
            error("Call on non-object", instr);
            return false;
        }

        BydaoValue result;
        if (obj.toObject()->callMethod(methodName, args, result)) {
            m_stack.push(result);
        } else {
            error("Method not found: " + methodName, instr);
            return false;
        }
        break;
    }

    case BydaoOpCode::Jump: {
        m_pc = instr.arg.toInt();
        break;
    }

    case BydaoOpCode::JumpIfFalse: {
        BydaoValue cond = m_stack.pop();
        if (!cond.toBool()) {
            m_pc = instr.arg.toInt();
        }
        break;
    }

    case BydaoOpCode::JumpIfTrue: {
        BydaoValue cond = m_stack.pop();
        if (cond.toBool()) {
            m_pc = instr.arg.toInt();
        }
        break;
    }

    case BydaoOpCode::Break:
        // TODO: реализовать break
        break;

    case BydaoOpCode::Next:
        // TODO: реализовать next (continue)
        break;

    case BydaoOpCode::UseModule: {
        QString arg = instr.arg;
        QString moduleName, alias;

        if (arg.contains(" as ")) {
            QStringList parts = arg.split(" as ");
            moduleName = parts[0];
            alias = parts[1];
        } else {
            moduleName = arg;
            alias = arg;
        }

        if (m_globals.contains(alias)) {
            break;
        }

        QString errorMsg;
        BydaoModule* module = BydaoModuleManager::instance().loadModule(moduleName, &errorMsg);

        if (!module) {
            error("Cannot load module: " + errorMsg, instr);
            return false;
        }

        m_globals[alias] = BydaoValue(module);
        break;
    }

    default:
        qDebug() << "Unknown opcode:" << (int)instr.op;
        error("Unknown opcode", instr);
        return false;
    }

    return true;
}

void BydaoVM::error(const QString& msg, const BydaoInstruction& instr) {
    m_lastError = QString("%1 at %2:%3")
    .arg(msg)
        .arg(instr.line)
        .arg(instr.column);
    m_errorLine = instr.line;
    m_running = false;

    if (m_debugMode) {
        qDebug() << "❌ ERROR:" << m_lastError;
    }
}

} // namespace BydaoScript
