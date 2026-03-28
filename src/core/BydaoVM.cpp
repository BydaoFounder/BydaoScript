// Copyright 2026 Oleh Horshkov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
#include <QElapsedTimer>
#include <QDebug>

namespace BydaoScript {

// ========== BydaoVM ==========

BydaoVM::BydaoVM()
    : m_pc(0)
    , m_running(false)
    , m_errorLine(-1)
    , m_traceMode(false)
    , m_profileMode(false)
    , m_lastInstrStart(0)
{
    // Инициализируем область видимости
    m_scopeStack.clear();
}

BydaoVM::~BydaoVM() {
    // Очищаем стек областей
    m_scopeStack.clear();
}

// ========== Загрузка байткода ==========

bool BydaoVM::load(const QVector<BydaoConstant>& constants,
                   const QVector<QString>& stringTable,
                   const QVector<BydaoInstruction>& code) {
    
    m_constants = constants;
    m_stringTable = stringTable;
    m_code = code;
    
    // Преобразуем константы в готовые значения
    m_constantValues.clear();
    m_constantValues.reserve(constants.size());
    
    foreach (const auto& c, constants) {
        switch (c.type) {
        case CONST_INT:
            m_constantValues.append(BydaoValue::fromInt(c.intValue));
            break;
        case CONST_REAL:
            m_constantValues.append(BydaoValue::fromReal(c.realValue));
            break;
        case CONST_STRING:
            if (c.stringIndex < (quint32)m_stringTable.size()) {
                m_constantValues.append(BydaoValue::fromString(m_stringTable[c.stringIndex]));
            } else {
                m_constantValues.append(BydaoValue(BydaoNull::instance()));
            }
            break;
        case CONST_BOOL:
            m_constantValues.append(BydaoValue::fromBool(c.boolValue != 0));
            break;
        case CONST_NULL:
            m_constantValues.append(BydaoValue(BydaoNull::instance()));
            break;
        default:
            m_constantValues.append(BydaoValue(BydaoNull::instance()));
            break;
        }
    }
    
    // Сброс состояния
    m_pc = 0;
    m_stack.clear();
    m_scopeStack.clear();

    // В стек значений добавим специальную переменную

    RuntimeVar var;
    var.name = SPECIAL_VAR;
    var.value = BydaoValue();
    m_scopeStack.append( var );

    return true;
}

// ========== Выполнение ==========

bool BydaoVM::run() {
    m_running = true;
    m_pc = 0;
    
    QElapsedTimer timer;
    
    while (m_running && m_pc >= 0 && m_pc < m_code.size()) {
        const BydaoInstruction& instr = m_code[m_pc++];
        
        if (m_traceMode) {
            QString opName = BydaoBytecode::opcodeToString(instr.op);
            if (instr.arg1 != 0 || instr.arg2 != 0) {
                qDebug().noquote() << QString("EXEC: %1 %2 [%3,%4]")
                                       .arg(m_pc-1, 4)
                                       .arg(opName, -10)
                                       .arg(instr.arg1)
                                       .arg(instr.arg2);
            } else {
                qDebug().noquote() << QString("EXEC: %1 %2")
                                       .arg(m_pc-1, 4)
                                       .arg(opName);
            }
        }
        
        if (m_profileMode) {
            timer.start();
        }
        
        if (!execute(instr)) {
            return false;
        }
        
        if (m_profileMode) {
            qint64 duration = timer.nsecsElapsed();
            QString opName = BydaoBytecode::opcodeToString(instr.op);
            
            ProfileData& data = m_profile[opName];
            data.totalTime += duration;
            data.callCount++;
        }
    }
    
    return true;
}

void BydaoVM::stop() {
    m_running = false;
}

// ========== Профилирование ==========

QVector<BydaoVM::ProfileItem> BydaoVM::takeProfile() {
    QVector<ProfileItem> result;
    
    for (auto it = m_profile.begin(); it != m_profile.end(); ++it) {
        ProfileItem item;
        item.name = it.key();
        item.time = it.value().totalTime;
        item.count = it.value().callCount;
        result.append(item);
    }
    
    // Сортируем по времени выполнения
    std::sort(result.begin(), result.end(),
              [](const ProfileItem& a, const ProfileItem& b) {
                  return a.time > b.time;
              });
    
    m_profile.clear();
    return result;
}

void BydaoVM::dumpStack(const QString& label) {
    if (!label.isEmpty()) {
        qDebug() << "=== " << label << " ===";
    }
    for (int i = 0; i < m_stack.size(); i++) {
        QString value;
        if (m_stack[i].isObject()) {
            if (m_stack[i].typeId() == TYPE_STRING)
                value = "\"" + m_stack[i].toString() + "\"";
            else
                value = m_stack[i].toObject()->typeName();
        } else {
            value = "null";
        }
        qDebug() << "    [" << i << "]:" << value;
    }
}

// ========== Доступ к переменным ==========

BydaoValue& BydaoVM::getVariable(int varIndex, const BydaoInstruction& instr) {
    static BydaoValue nullValue(BydaoNull::instance());
    
    if (varIndex < 0 || varIndex >= m_scopeStack.size()) {
        error("Invalid variable index", instr);
        return nullValue;
    }
    
    return m_scopeStack[varIndex].value;
}

const BydaoValue& BydaoVM::getVariable(int varIndex, const BydaoInstruction& instr) const {
    static BydaoValue nullValue(BydaoNull::instance());

    Q_UNUSED(instr);

    if (varIndex < 0 || varIndex >= m_scopeStack.size()) {
        return nullValue;
    }
    
    return m_scopeStack[varIndex].value;
}

void BydaoVM::setVariable(int varIndex, const BydaoValue& value, const BydaoInstruction& instr) {
    if (varIndex < 0 || varIndex >= m_scopeStack.size()) {
        error("Invalid variable index", instr);
        return;
    }
    
    m_scopeStack[varIndex].value = value;
}

// ========== Обработка ошибок ==========

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

// ========== Выполнение инструкций ==========

bool BydaoVM::execute(const BydaoInstruction& instr) {
    switch (instr.op) {

    case BydaoOpCode::Load: {
        m_stack.push( getVariable(instr.arg1, instr) );
        break;
    }

    case BydaoOpCode::Store: {
        if ( instr.arg2 > 0 ) {
            setVariable(instr.arg1, getVariable( instr.arg2, instr ), instr);
        }
        else if ( instr.arg2 < 0 ) {
            setVariable(instr.arg1, m_constantValues[ -instr.arg2 ], instr);
        }
        else {
            setVariable(instr.arg1, m_stack.pop(), instr);
        }
        break;
    }

    // ===== Области видимости =====

    case BydaoOpCode::ScopeDrop:
        m_scopeStack.resize(instr.arg1);
        break;

    // ===== Переходы =====
    case BydaoOpCode::Jump: {
        m_pc = instr.arg1;
        break;
    }

    case BydaoOpCode::JumpIfFalse: {
        BydaoValue cond = m_stack.pop();
        if (!cond.toBool()) {
            m_pc = instr.arg1;
        }
        break;
    }

    case BydaoOpCode::JumpIfTrue: {
        BydaoValue cond = m_stack.pop();
        if (cond.toBool()) {
            m_pc = instr.arg1;
        }
        break;
    }
    
    // ===== Переменные =====

    case BydaoOpCode::ConstDecl: {
        QString name = m_stringTable[instr.arg1];
        RuntimeVar var;
        var.name = name;
        var.value = m_constantValues[ instr.arg2 ];
        m_scopeStack.append(var);
        break;
    }

    case BydaoOpCode::VarDecl: {
        if ( instr.arg1 >= 0 ) {    // задано название переменной

            RuntimeVar var;
            var.name = m_stringTable[instr.arg1];

            if ( instr.arg2 < 0 ) {
                // создание переменной и инициализация константным значением
                var.value = m_constantValues[ -instr.arg2 ].copy();
            }
            else if ( instr.arg2 == 1 ) {
                // инициализация значением со стека
                var.value = m_stack.pop();
            }
            m_scopeStack.append(var);
        }
        else {
            // Анонимная переменная (например, временная)

            int varIndex = m_scopeStack.size();
            RuntimeVar var;
            var.name = QString("__tmp_%1").arg(varIndex);
            m_scopeStack.append(var);
        }
        break;
    }
    
    case BydaoOpCode::Drop: {
        int varIndex = instr.arg1;
        if (varIndex >= 0 && varIndex < m_scopeStack.size()) {
            m_scopeStack.removeAt( varIndex );
        }
        break;
    }

    case BydaoOpCode::VarAdd: {

        if ( instr.arg2 < 0 ) { // сложение с константой
            BydaoValue& a = getVariable(instr.arg1, instr);
            if (!a.isObject()) {
                error("'+' operation on non-object", instr);
                return false;
            }
            m_stack.push( a.toObject()->add( m_constantValues[ -instr.arg2 ] ) );
        }
        else {                  // сложение двух переменных
            BydaoValue& b = getVariable(instr.arg2, instr);
            BydaoValue& a = getVariable(instr.arg1, instr);
            if (!a.isObject() || !b.isObject()) {
                error("'+' operation on non-object", instr);
                return false;
            }
            m_stack.push( a.toObject()->add(b) );
        }
        break;
    }

    case BydaoOpCode::AddStore: {

        if ( instr.arg2 > 0 ) {         // прибавить к переменной значение переменной
            BydaoValue& b = getVariable(instr.arg2, instr);
            BydaoValue& a = getVariable(instr.arg1, instr);
            if (!a.isObject() || !b.isObject()) {
                error("'+=' operation on non-object", instr);
                return false;
            }
            a.toObject()->addToValue(b);
        }
        else if ( instr.arg2 < 0 ) {    // прибавить к переменной значение константы
            BydaoValue& a = getVariable(instr.arg1, instr);
            if ( ! a.isObject() ) {
                error("'+=' operation on non-object", instr);
                return false;
            }
            a.toObject()->addToValue( m_constantValues[ -instr.arg2 ] );
        }
        else {                          // прибавить к переменной значение со стека
            BydaoValue b = m_stack.pop();
            BydaoValue& a = getVariable(instr.arg1, instr);
            if (!a.isObject() || !b.isObject()) {
                error("'+=' operation on non-object", instr);
                return false;
            }
            a.toObject()->addToValue(b);
        }
        break;
    }

    case BydaoOpCode::SubStore: {
        BydaoValue b = m_stack.pop();
        BydaoValue& a = getVariable(instr.arg1, instr);
        if (!a.isObject() || !b.isObject()) {
            error("'-=' operation on non-object", instr);
            return false;
        }

        BydaoValue val = a.toObject()->sub(b);
        setVariable(instr.arg1, val, instr);
        break;
    }

    case BydaoOpCode::MulStore: {
        BydaoValue b = m_stack.pop();
        BydaoValue& a = getVariable(instr.arg1, instr);
        if (!a.isObject() || !b.isObject()) {
            error("'*=' operation on non-object", instr);
            return false;
        }

        BydaoValue val = a.toObject()->mul(b);
        setVariable(instr.arg1, val, instr);
        break;
    }

    case BydaoOpCode::DivStore: {
        BydaoValue b = m_stack.pop();
        BydaoValue& a = getVariable(instr.arg1, instr);
        if (!a.isObject() || !b.isObject()) {
            error("'/=' operation on non-object", instr);
            return false;
        }

        BydaoValue val = a.toObject()->div(b);
        setVariable(instr.arg1, val, instr);
        break;
    }

    case BydaoOpCode::ModStore: {
        BydaoValue b = m_stack.pop();
        BydaoValue& a = getVariable(instr.arg1, instr);
        if (!a.isObject() || !b.isObject()) {
            error("'%=' operation on non-object", instr);
            return false;
        }

        BydaoValue val = a.toObject()->mod(b);
        setVariable(instr.arg1, val, instr);
        break;
    }

    // ===== Константы =====
    case BydaoOpCode::PushConst: {
        if ( instr.arg1 >= 0 && instr.arg1 < m_constantValues.size() ) {
//            m_stack.push( m_constantValues[instr.arg1].copy() );
            m_stack.push( m_constantValues[instr.arg1] );
        }
        else {
            error("Invalid constant index", instr);
            return false;
        }
        break;
    }
    
    // ===== Арифметика =====
    case BydaoOpCode::Add: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        if (!a.isObject() || !b.isObject()) {
            error("Add operation on non-object", instr);
            return false;
        }
        
        m_stack.push(a.toObject()->add(b));
        break;
    }
    
    case BydaoOpCode::Sub: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        if (!a.isObject() || !b.isObject()) {
            error("Sub operation on non-object", instr);
            return false;
        }
        
        m_stack.push(a.toObject()->sub(b));
        break;
    }
    
    case BydaoOpCode::Mul: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        if (!a.isObject() || !b.isObject()) {
            error("Mul operation on non-object", instr);
            return false;
        }
        
        m_stack.push(a.toObject()->mul(b));
        break;
    }
    
    case BydaoOpCode::Div: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        if (!a.isObject() || !b.isObject()) {
            error("Div operation on non-object", instr);
            return false;
        }
        
        m_stack.push(a.toObject()->div(b));
        break;
    }
    
    case BydaoOpCode::Mod: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        if (!a.isObject() || !b.isObject()) {
            error("Mod operation on non-object", instr);
            return false;
        }
        
        m_stack.push(a.toObject()->mod(b));
        break;
    }
    
    case BydaoOpCode::Neg: {
        BydaoValue a = m_stack.pop();
        
        if (!a.isObject()) {
            error("Neg operation on non-object", instr);
            return false;
        }
        
        m_stack.push(a.toObject()->neg());
        break;
    }
    
    // ===== Сравнение =====
    case BydaoOpCode::Eq: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        if (!a.isObject() || !b.isObject()) {
            // Сравнение с null
            bool result = (a.isNull() && b.isNull());
            m_stack.push(BydaoValue::fromBool(result));
            break;
        }
        
        m_stack.push(a.toObject()->eq(b));
        break;
    }
    
    case BydaoOpCode::Neq: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        if (!a.isObject() || !b.isObject()) {
            bool result = !(a.isNull() && b.isNull());
            m_stack.push(BydaoValue::fromBool(result));
            break;
        }
        
        m_stack.push(a.toObject()->neq(b));
        break;
    }

    case BydaoOpCode::Lt: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();

        if (!a.isObject() || !b.isObject()) {
            error("Comparison with non-object", instr);
            return false;
        }

        m_stack.push(a.toObject()->lt(b));
        break;
    }

    case BydaoOpCode::VarLt: {
        if ( instr.arg2 < 0 ) {

            // Сравнение переменной с константой
            BydaoValue& a = getVariable( instr.arg1, instr );
            BydaoValue& b = m_constantValues[ -instr.arg2 ];
            if (!a.isObject() || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push(a.toObject()->lt(b));

        }
        else {

            // Сравнение двух переменных
            BydaoValue& a = getVariable( instr.arg1, instr );
            BydaoValue& b = getVariable( instr.arg2, instr );
            if (!a.isObject() || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push(a.toObject()->lt(b));
        }
        break;
    }

    case BydaoOpCode::Gt: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        if (!a.isObject() || !b.isObject()) {
            error("Comparison with non-object", instr);
            return false;
        }
        
        m_stack.push(a.toObject()->gt(b));
        break;
    }
    
    case BydaoOpCode::Le: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        if (!a.isObject() || !b.isObject()) {
            error("Comparison with non-object", instr);
            return false;
        }
        
        m_stack.push(a.toObject()->le(b));
        break;
    }
    
    case BydaoOpCode::Ge: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        if (!a.isObject() || !b.isObject()) {
            error("Comparison with non-object", instr);
            return false;
        }
        
        m_stack.push(a.toObject()->ge(b));
        break;
    }
    
    // ===== Логические операции =====
    case BydaoOpCode::And: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        bool result = a.toBool() && b.toBool();
        m_stack.push(BydaoValue::fromBool(result));
        break;
    }
    
    case BydaoOpCode::Or: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        
        bool result = a.toBool() || b.toBool();
        m_stack.push(BydaoValue::fromBool(result));
        break;
    }
    
    case BydaoOpCode::Not: {
        BydaoValue a = m_stack.pop();
        m_stack.push(BydaoValue::fromBool(!a.toBool()));
        break;
    }
    
    // ===== Итераторы =====

    case BydaoOpCode::GetIter: {
        BydaoValue obj = m_stack.pop();

        if ( ! obj.isObject() ) {
            error("GETITER on non-object", instr);
            return false;
        }

        BydaoValue it = obj.toObject()->iter();
        if ( it.isNull() ) {
            error("Object does not have an iterator", instr);
            return false;
        }

        m_stack.push( it );
        break;
    }

    case BydaoOpCode::ItNext: {
        auto* iter = (BydaoIterator*)( m_stack.pop().toObject() );
        if (!iter) {
            error("ITNEXT on non-iterator", instr);
            return false;
        }
        m_stack.push( BydaoValue( BydaoBool::create( iter->next() ) ) ); // BydaoValue::fromBool( iter->next() ) );
        break;
    }
    
    case BydaoOpCode::ItValue: {
        auto* iter = (BydaoIterator*)( m_stack.pop().toObject() );
        if (!iter) {
            error("ITVALUE on non-iterator", instr);
            return false;
        }
        m_stack.push(iter->value());
        break;
    }
    
    case BydaoOpCode::ItKey: {
        auto* iter = (BydaoIterator*)(m_stack.pop().toObject());
        if (!iter) {
            error("ITKEY on non-iterator", instr);
            return false;
        }
        m_stack.push(iter->key());
        break;
    }

    case BydaoOpCode::Index: {
        BydaoValue obj = m_stack.pop();
        BydaoValue index = m_stack.pop();

        if (auto* array = (BydaoArray*)(obj.toObject())) {
            m_stack.push(array->get(index));
        } else {
            error("INDEX not supported for Null value", instr);
            return false;
        }
        break;
    }

    // ===== Доступ к членам =====
    case BydaoOpCode::Member: {
        if (m_traceMode ) {
            dumpStack("Before MEMBER at PC " + QString::number(m_pc-1));
        }

        if (m_stack.isEmpty()) {
            error("Stack underflow in MEMBER", instr);
            return false;
        }

        BydaoValue obj = m_stack.pop();

        if (!obj.isObject()) {
            error("MEMBER on non-object", instr);
            return false;
        }

        QString memberName;
        if (instr.arg1 >= 0 && instr.arg1 < m_stringTable.size()) {
            memberName = m_stringTable[instr.arg1];
//            qDebug() << "  memberName:" << memberName;
        } else {
            error("Invalid member name index", instr);
            return false;
        }

        // Проверяем, является ли объект нативным со свойствами
        if (auto* native = (BydaoNative*)(obj.toObject())) {
            if (native->hasProperty(memberName)) {
                // Это свойство - возвращаем его значение
//                qDebug() << "  -> property found, getting value";
                m_stack.push(native->getProperty(memberName));
                if ( m_traceMode ) {
                    dumpStack("After MEMBER (property)");
                }
                break;
            }
        }

        // Свойство не найдено
        QString objType = obj.isObject() ? obj.toObject()->typeName() : "non-object";
        error(QString("Property not found: '%1' on object of type '%2'").arg(memberName, objType), instr);
        return false;
    }

    case BydaoOpCode::Method: {
        if ( m_traceMode ) {
            dumpStack("Before METHOD at PC " + QString::number(m_pc-1));
        }

        if (m_stack.isEmpty()) {
            error("Stack underflow in METHOD", instr);
            return false;
        }

        BydaoValue obj = m_stack.pop();
        if (!obj.isObject()) {
            error("METHOD on non-object", instr);
            return false;
        }

        QString methodName;
        if (instr.arg1 >= 0 && instr.arg1 < m_stringTable.size()) {
            methodName = m_stringTable[instr.arg1];
//            qDebug() << "  methodName:" << methodName;
        } else {
            error("Invalid method name index", instr);
            return false;
        }

        // Кладём объект и имя метода на стек для последующего CALL
        m_stack.push(obj);
        m_stack.push(BydaoValue::fromString(methodName));

        if ( m_traceMode ) {
            qDebug() << "  prepared for method call";
            dumpStack("After METHOD");
        }
        break;
    }

    case BydaoOpCode::Call: {
        int argCount = instr.arg1;
        if ( m_traceMode ) {
            dumpStack("Before CALL at PC " + QString::number(m_pc-1) +
                      ", argCount=" + QString::number(argCount));
        }

        // Забираем аргументы
        QVector<BydaoValue> args;
        for (int i = 0; i < argCount; i++) {
            if (m_stack.isEmpty()) {
                error("Stack underflow: not enough arguments", instr);
                return false;
            }
            args.prepend(m_stack.pop());
        }

        // Имя метода должно быть на стеке (от предыдущей инструкции METHOD)
        if (m_stack.size() < 2) {
            error("Stack underflow: no method name and object for CALL", instr);
            return false;
        }

        // Проверяем, что на стеке строка - имя метода
        const BydaoValue& top = m_stack.pop();
        if ( ! top.isObject() || top.typeId() != TYPE_STRING) {

            QString valueDesc = top.isObject()
                ? top.toObject()->typeName()
                : "non-object";

            // Пытаемся определить, что хотели вызвать
            QString hint;
            if (instr.arg2 >= 0 && instr.arg2 < m_stringTable.size()) {
                hint = m_stringTable[instr.arg2];
            } else {
                hint = "unknown";
            }

            error(QString("Cannot call '%1' as function - it is a %2")
                      .arg(hint)
                      .arg(valueDesc), instr);
            return false;
        }
        QString methodName = top.toString();

        BydaoValue obj = m_stack.pop();
        if ( ! obj.isObject() ) {
            error("CALL on non-object", instr);
            return false;
        }

//        qDebug() << "  calling method:" << methodName << "on object type:" << obj.toObject()->typeName();

        BydaoValue result;
        if (obj.toObject()->callMethod(methodName, args, result)) {
            m_stack.push(result);
            if ( m_traceMode ) {
                dumpStack("After CALL");
            }
        } else {
            error(QString("Method not found: '%1' on object of type '%2'")
                      .arg(methodName)
                      .arg(obj.toObject()->typeName()), instr);
            return false;
        }
        break;
    }

    case BydaoOpCode::CallVoid: {
        int argCount = instr.arg1;
        if ( m_traceMode ) {
            dumpStack("Before CALLVOID at PC " + QString::number(m_pc-1) +
                      ", argCount=" + QString::number(argCount));
        }

        // Забираем аргументы
        QVector<BydaoValue> args;
        for (int i = 0; i < argCount; i++) {
            if (m_stack.isEmpty()) {
                error("Stack underflow: not enough arguments", instr);
                return false;
            }
            args.prepend(m_stack.pop());
        }

        // Имя метода должно быть на стеке (от предыдущей инструкции METHOD)
        if (m_stack.size() < 2) {
            error("Stack underflow: no method name and object for CALLVOID", instr);
            return false;
        }

        // Проверяем, что на стеке строка - имя метода
        if (!m_stack.top().isObject() || m_stack.top().typeId() != TYPE_STRING) {
            BydaoValue badValue = m_stack.top();
            QString valueDesc;
            if (badValue.isObject()) {
                valueDesc = badValue.toObject()->typeName();
            } else {
                valueDesc = "non-object";
            }

            // Пытаемся определить, что хотели вызвать
            QString hint;
            if (instr.arg2 >= 0 && instr.arg2 < m_stringTable.size()) {
                hint = m_stringTable[instr.arg2];
            } else {
                hint = "unknown";
            }

            error(QString("Cannot call '%1' as function - it is a %2")
                      .arg(hint)
                      .arg(valueDesc), instr);
            return false;
        }

        QString methodName = m_stack.pop().toString();
        BydaoValue obj = m_stack.pop();

        if (!obj.isObject()) {
            error("CALLVOID on non-object", instr);
            return false;
        }

        //        qDebug() << "  calling method:" << methodName << "on object type:" << obj.toObject()->typeName();

        BydaoValue result;
        if (obj.toObject()->callMethod(methodName, args, result)) {
            if ( m_traceMode ) {
                dumpStack("After CALLVOID");
            }
        } else {
            error(QString("Method not found: '%1' on object of type '%2'")
                      .arg(methodName)
                      .arg(obj.toObject()->typeName()), instr);
            return false;
        }
        break;
    }

    // ===== Модули и типы =====
    case BydaoOpCode::UseModule: {

        QString moduleName;
        QString alias;
        
        if (instr.arg1 >= 0 && instr.arg1 < m_stringTable.size()) {
            moduleName = m_stringTable[instr.arg1];
        } else {
            error("Invalid module name index", instr);
            return false;
        }
        
        if (instr.arg2 >= 0 && instr.arg2 < m_stringTable.size()) {
            alias = m_stringTable[instr.arg2];
        } else {
            alias = moduleName;
        }
        
        // Проверяем, не загружен ли уже модуль
        if ( m_moduleName.contains( alias ) ) {
            break;
        }
        
        QString errorMsg;
        BydaoModule* module = BydaoModuleManager::instance().loadModule(moduleName, &errorMsg);
        
        if (!module) {
            error("Cannot load module: " + errorMsg, instr);
            return false;
        }
        
        // Добавляем модуль как переменную в глобальной области

        RuntimeVar var;
        var.name = alias;
        var.value = BydaoValue(module);
        
        int varIndex = m_scopeStack.size();
        m_scopeStack.append(var);
//        m_scopeStack.append( BydaoValue(module) );
        m_moduleName[alias] = varIndex;
        
        break;
    }
    
    case BydaoOpCode::TypeClass: {
        QString typeName;
        if (instr.arg1 >= 0 && instr.arg1 < m_stringTable.size()) {
            typeName = m_stringTable[instr.arg1];
        } else {
            error("Invalid type name index", instr);
            return false;
        }
        
        BydaoValue typeClass = BydaoTypeRegistry::getClass(typeName);
        if (typeClass.isNull()) {
            error("Unknown type class: " + typeName, instr);
            return false;
        }
        
        m_stack.push(typeClass);
        break;
    }
    
    // ===== Массивы =====
    case BydaoOpCode::PushArray: {
        int count = instr.arg1;

        // Создаём новый массив
        auto* array = new BydaoArray();

        // Извлекаем элементы со стека
        // Элементы лежат на стеке в порядке вычисления:
        // первый элемент вычислен первым и лежит внизу стека,
        // последний элемент - наверху стека
        QVector<BydaoValue> elements;
        elements.reserve(count);

        for (int i = 0; i < count; i++) {
            // Так как последний элемент наверху, а нам нужно сохранить порядок,
            // используем prepend
            elements.prepend(m_stack.pop());
        }

        // Добавляем элементы в массив
        for (const auto& elem : elements) {
            array->append(elem);
        }

        // Кладём массив на стек
        m_stack.push(BydaoValue(array));
        break;
    }

    // ===== Управление =====
    case BydaoOpCode::Nop:
        break;

    case BydaoOpCode::Halt:
        m_running = false;
        break;

    default:
        error("Unknown opcode: " + QString::number((int)instr.op), instr);
        return false;
    }
    
    return true;
}

} // namespace BydaoScript
