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

BydaoVM::BydaoVM() : m_pc(0), m_running(false), m_traceMode(false) {}

BydaoVM::~BydaoVM() {
//    qDebug() << "~BydaoVM()";
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

void BydaoVM::setTraceMode(bool enable) {
    m_traceMode = enable;
}

bool BydaoVM::run() {

    m_running = true;
    m_pc = 0;

    while (m_running && m_pc >= 0 && m_pc < m_code.size()) {
        const BydaoInstruction& instr = m_code[m_pc++];

        if (m_traceMode) {
            if ( instr.arg.isEmpty() ) {
                qDebug().noquote() << QString("EXEC: %1 %2").arg( m_pc-1 ).arg( BydaoBytecode::opcodeToString(instr.op) );
            }
            else {
                qDebug().noquote() << QString("EXEC: %1 %2 '%3'").arg( m_pc-1 ).arg( BydaoBytecode::opcodeToString(instr.op), instr.arg);
            }
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

    case BydaoOpCode::Drop: {
        QString varName = instr.arg;

        if (m_traceMode) {
            qDebug() << "  Dropping variable:" << varName;
        }

        // Удаляем переменную из текущей области видимости
        // В нашей простой VM пока просто удаляем из глобалов
        // В будущем нужно будет работать со стеком областей видимости
        m_globals.remove(varName);

        break;
    }

    case BydaoOpCode::Load: {
        // qDebug() << "LOAD" << instr.arg;
        // qDebug() << "  globals contains:" << m_globals.contains(instr.arg);
        // if (m_globals.contains(instr.arg)) {
        //     qDebug() << "  value type:" << m_globals[instr.arg].typeId();
        // }
        m_stack.push(m_globals.value(instr.arg));
        break;
    }

    case BydaoOpCode::Store:
        m_globals[instr.arg] = m_stack.pop();
        break;

    case BydaoOpCode::PushInt:
        m_stack.push(BydaoValue(BydaoInt::create(instr.arg.toLongLong())));
        break;

    case BydaoOpCode::PushReal:
        m_stack.push(BydaoValue(BydaoReal::create(instr.arg.toDouble())));
        break;

    case BydaoOpCode::PushString:
        m_stack.push(BydaoValue(BydaoString::create(instr.arg)));
        break;

    case BydaoOpCode::PushNull:
        m_stack.push(BydaoValue(BydaoNull::instance()));
        break;

    case BydaoOpCode::PushFalse:
        m_stack.push(BydaoValue(BydaoBool::create(false)));
        break;

    case BydaoOpCode::PushTrue:
        m_stack.push(BydaoValue(BydaoBool::create(true)));
        break;

    case BydaoOpCode::PushArray: {
        int count = (int)instr.arg.toInt();

        // qDebug() << "Creating array with" << count << "elements";

        auto* array = new BydaoArray();

        // Извлекаем элементы в правильном порядке
        // (они лежат на стеке в порядке вычисления: первый элемент - внизу)
        QVector<BydaoValue> elements;
        for (int i = 0; i < count; i++) {
            // Так как первый элемент внизу, а мы pop берём сверху,
            // нужно использовать prepend
            elements.prepend(m_stack.pop());
        }

        for (const auto& elem : elements) {
            array->append(elem);
        }

        m_stack.push(BydaoValue(array));
        break;
    }

    case BydaoOpCode::Add: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();

        if (!a.isObject() || !b.isObject()) {
            error("Add on non-object", instr);
            return false;
        }

        m_stack.push(a.toObject()->add(b));
        break;
    }

    case BydaoOpCode::Sub: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(a.toObject()->sub(b));
        break;
    }

    case BydaoOpCode::Mul: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(a.toObject()->mul(b));
        break;
    }

    case BydaoOpCode::Div: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(a.toObject()->div(b));
        break;
    }

    case BydaoOpCode::Mod: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(a.toObject()->mod(b));
        break;
    }

    case BydaoOpCode::Neg: {
        BydaoValue a = m_stack.pop();
        m_stack.push(a.toObject()->neg());
        break;
    }

    case BydaoOpCode::Eq: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        if ( a.isNull() && b.isNull() ) {
            m_stack.push( BydaoValue::fromBool( true ) );
        }
        else if ( b.isNull() ) {
            m_stack.push( BydaoValue::fromBool( false ) );
        }
        else {
            m_stack.push(a.toObject()->eq(b));
        }
        break;
    }

    case BydaoOpCode::Neq: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        if ( a.isNull() && b.isNull() ) {
            m_stack.push( BydaoValue::fromBool( false ) );
        }
        else if ( b.isNull() ) {
            m_stack.push( BydaoValue::fromBool( true ) );
        }
        else {
            m_stack.push(a.toObject()->neq(b));
        }
        break;
    }

    case BydaoOpCode::Lt: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        // qDebug() << "Lt: a type =" << a.typeId() << "b type =" << b.typeId();
        m_stack.push(a.toObject()->lt(b));
        break;
    }

    case BydaoOpCode::Gt: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(a.toObject()->gt(b));
        break;
    }

    case BydaoOpCode::Le: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(a.toObject()->le(b));
        break;
    }

    case BydaoOpCode::Ge: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(a.toObject()->ge(b));
        break;
    }

    case BydaoOpCode::And: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(a.toObject()->and_(b));
        break;
    }

    case BydaoOpCode::Or: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        m_stack.push(a.toObject()->or_(b));
        break;
    }

    case BydaoOpCode::Not: {
        BydaoValue a = m_stack.pop();
        m_stack.push(a.toObject()->not_());
        break;
    }

    case BydaoOpCode::Member: {
        QString propName = instr.arg;
        BydaoValue obj = m_stack.pop();

        if (!obj.isObject()) {
            error("Cannot access property of non-object", instr);
            return false;
        }

        BydaoObject* objPtr = obj.toObject();

        if (auto* native = dynamic_cast<BydaoNative*>(objPtr)) {
            if (native->hasProperty(propName)) {
                m_stack.push(native->getProperty(propName));
                break;
            }
        }

        // Свойство не найдено — ошибка!
        error("Property not found: " + propName, instr);
        return false;
    }

    case BydaoOpCode::Index: {
        // На стеке: сверху объект, под ним индекс
        BydaoValue obj = m_stack.pop();    // забираем объект (сверху)
        BydaoValue index = m_stack.pop();  // забираем индекс (следующий)

        // qDebug() << "Index: obj type:" << obj.toObject()->typeName()
        //          << "index:" << index.toInt();

        if (auto* array = dynamic_cast<BydaoArray*>(obj.toObject())) {
            m_stack.push(array->get(index));
        }
        else {
            error("Index not supported for type: " + obj.toObject()->typeName(), instr);
            return false;
        }
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
        bool value;
        if ( cond.typeId() == TYPE_BOOL ) {
            value = static_cast<const BydaoBool*>(cond.toObject())->value();
        }
        else {
            value = cond.toBool();
        }
        if (! value) {
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

    if (m_traceMode) {
        qDebug() << "❌ ERROR:" << m_lastError;
    }
}

} // namespace BydaoScript
