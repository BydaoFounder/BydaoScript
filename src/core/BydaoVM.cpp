#include "BydaoScript/BydaoVM.h"
#include "BydaoScript/BydaoNative.h"
#include "BydaoScript/BydaoModule.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoArray.h"
#include "BydaoScript/BydaoNull.h"
#include "BydaoScript/BydaoTypeRegistry.h"
#include "BydaoScript/BydaoIterator.h"
#include <QDebug>

namespace BydaoScript {

// ========== BydaoVM ==========

BydaoVM::BydaoVM() : m_pc(0), m_running(false), m_traceMode(false) {}

BydaoVM::~BydaoVM() {
//    qDebug() << "~BydaoVM()";
    // Очищаем ссылки на модули, но не удаляем их
//    m_globalObjects.clear();
    m_scopeStack.clear();
}

bool BydaoVM::load(const QVector<BydaoInstruction>& code) {
    m_code = code;
    m_pc = 0;
    m_stack.clear();
    m_scopeStack.clear();
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

    case BydaoOpCode::ScopeBegin:
    case BydaoOpCode::ScopeEnd:
        // Ничего не делаем — быстрый случай
        break;

    case BydaoOpCode::ScopePush:
        m_scopeStack.push(Scope());
        break;

    case BydaoOpCode::ScopePop:
        m_scopeStack.pop();
        break;

    case BydaoOpCode::VarDecl: {
        QString name = instr.arg;

        if (m_scopeStack.isEmpty()) {
            error("No active scope for variable declaration: " + name, instr);
            return false;
        }

        // Проверяем, не объявлена ли уже в этой области
        if (m_scopeStack.top().vars.contains(name)) {
            error("Variable already declared in this scope: " + name, instr);
            return false;
        }

        m_scopeStack.top().vars[name] = BydaoValue(BydaoNull::instance());
        break;
    }

    case BydaoOpCode::Drop: {
        QString name = instr.arg;

        if (m_scopeStack.isEmpty()) {
            error("No active scope for drop: " + name, instr);
            return false;
        }

        if (!m_scopeStack.top().vars.contains(name)) {
            error("Cannot drop undeclared variable: " + name, instr);
            return false;
        }

        m_scopeStack.top().vars.remove(name);
        break;
    }

    case BydaoOpCode::Load: {
        QString name = instr.arg;
        BydaoValue value;

        // Ищем в текущей области видимости (снизу вверх по стеку)
        bool found = false;
        for (int i = m_scopeStack.size() - 1; i >= 0; i--) {
            if (m_scopeStack[i].vars.contains(name)) {
                found = true;
                value = m_scopeStack[i].vars[name];
                break;
            }
        }

        if ( ! found ) {
            error("Undefined variable: " + name, instr);
            return false;
        }

        m_stack.push(value);
        break;
    }

    case BydaoOpCode::Store: {
        QString name = instr.arg;
        BydaoValue value = m_stack.pop();

        // Ищем переменную в текущих областях
        bool found = false;
        for (int i = m_scopeStack.size() - 1; i >= 0; i--) {
            if (m_scopeStack[i].vars.contains(name)) {
                m_scopeStack[i].vars[name] = value;
                found = true;
                break;
            }
        }

        // Если не нашли, создаём в текущей области
        if (!found) {
            if (m_scopeStack.isEmpty()) {
                error("No active scope for variable: " + name, instr);
                return false;
            }
            m_scopeStack.top().vars[name] = value;
        }
        break;
    }

    case BydaoOpCode::TypeClass: {
        QString typeName = instr.arg;
        BydaoValue typeClass = BydaoTypeRegistry::getClass(typeName);

        if (typeClass.isNull()) {
            error("Unknown type class: " + typeName, instr);
            return false;
        }

        m_stack.push(typeClass);
        break;
    }

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

    case BydaoOpCode::Next: {
        BydaoValue obj = m_stack.pop();

        auto* iter = dynamic_cast<BydaoIterator*>(obj.toObject());
        if (!iter) {
            error("NEXT on non-iterator", instr);
            return false;
        }

        m_stack.push(BydaoValue::fromBool(iter->next()));
        break;
    }

    case BydaoOpCode::Value: {
        BydaoValue obj = m_stack.pop();

        auto* iter = dynamic_cast<BydaoIterator*>(obj.toObject());
        if (!iter) {
            error("VALUE on non-iterator", instr);
            return false;
        }

        m_stack.push(iter->value());
        break;
    }

    case BydaoOpCode::Key: {
        BydaoValue obj = m_stack.pop();

        auto* iter = dynamic_cast<BydaoIterator*>(obj.toObject());
        if (!iter) {
            error("KEY on non-iterator", instr);
            return false;
        }

        m_stack.push(iter->key());
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
        // Модули можно загружать только в корневой области
        if (m_scopeStack.size() != 1) {
            error("Cannot load module in nested scope", instr);
            return false;
        }

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

        // Проверяем, не загружен ли уже модуль в корневой области
        if (m_scopeStack.top().vars.contains(alias)) {
            // Уже загружен — просто игнорируем
            break;
        }

        QString errorMsg;
        BydaoModule* module = BydaoModuleManager::instance().loadModule(moduleName, &errorMsg);

        if (!module) {
            error("Cannot load module: " + errorMsg, instr);
            return false;
        }

        // Сохраняем модуль в корневой области
        m_scopeStack.top().vars[alias] = BydaoValue(module);
        break;
    }

    case BydaoOpCode::Halt:
        m_running = false;
        break;

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
