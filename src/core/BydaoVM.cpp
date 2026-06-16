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
#include "BydaoScript/BydaoObject.h"
#include "BydaoScript/BydaoModule.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoIntClass.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoArray.h"
#include "BydaoScript/BydaoNull.h"
#include "BydaoScript/BydaoIterator.h"
#include "BydaoScript/BydaoIntRange.h"
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
    m_scopeStack.append( VarScope() );
    m_scopeOffset = 0;

    // Встроенные типы
    // Порядок добавления встроенных типов должен совпадать с порядком в парсере

    auto* intClass = new BydaoIntClass();
    intClass->ref();
    s_builtinTypes.append( {"Int", BydaoValue(intClass, BydaoTypeId::TYPE_OBJECT)} );

    auto* stringClass = new BydaoStringClass();
    stringClass->ref();
    s_builtinTypes.append( {"String", BydaoValue(stringClass, BydaoTypeId::TYPE_OBJECT)} );

    // auto* realClass = new BydaoReal();
    // realClass->ref();
    // s_builtinTypes.append( {"Real", BydaoValue(realClass, BydaoTypeId::TYPE_OBJECT)} );

    // auto* boolClass = new BydaoBool();
    // boolClass->ref();
    // s_builtinTypes.append( {"Bool", BydaoValue(boolClass, BydaoTypeId::TYPE_OBJECT)} );

    // auto* nullClass = new BydaoNull();
    // nullClass->ref();
    // s_builtinTypes.append( {QStringLiteral("Null"), BydaoValue(nullClass, BydaoTypeId::TYPE_OBJECT)} );

    auto* arrayClass = new BydaoArray();
    arrayClass->ref();
    s_builtinTypes.append( {"Array", BydaoValue(arrayClass, BydaoTypeId::TYPE_OBJECT)} );
}

BydaoVM::~BydaoVM() {
    // Очищаем стек областей
    m_scopeStack.clear();
}

// ========== Загрузка байткода ==========

bool BydaoVM::loadModule(const ModuleInfo& module) {
    // Загружаем глобальные строки и константы
    m_stringTable = module.globalStringTable;
    m_constants = module.globalConstants;

    // Преобразуем константы в значения
    m_constantValues.clear();
    loadConstants( m_constants );

    // Создаём глобальный фрейм для модуля
    m_scopeStack.clear();
    m_scopeStack.append( VarScope() );
    m_scopeOffset = 0;

    // В стек значений добавим специальную переменную

    RuntimeVar var;
    var.name = SPECIAL_VAR;
    var.value = BydaoValue();
    m_scopeStack.append( var );

    // Добавляем публичные переменные
/*
    for (const auto& pubVar : module.pubVars) {
        RuntimeVar var;
        var.name = pubVar.name;
        var.value = pubVar.initValue.isNull() ? BydaoValue() : pubVar.initValue;
        m_scopeStack.append(var);
    }
*/
    // Загружаем глобальный код
    m_code = module.globalCode;
    m_pc = 0;

    // Создаём функции
    for (const FunctionInfo& funcInfo : module.functions) {
        auto* funcObj = new BydaoFuncObject();
        funcObj->ref();
        funcObj->name = funcInfo.name;
        // funcObj->bytecode = funcInfo.code;
        // funcObj->entryPc = funcInfo.entryPc;

        funcObj->entryPc = m_code.size();
        m_code.append( funcInfo.code );
        for ( int pc = funcObj->entryPc; pc < m_code.size(); ++pc ) {
            BydaoInstruction& instr = m_code[ pc ];
            switch ( instr.op ) {
            case BydaoOpCode::Jump:
            case BydaoOpCode::JumpFalsePeek:
            case BydaoOpCode::JumpTruePeek:
            case BydaoOpCode::JumpFalse:
            case BydaoOpCode::JumpTrue:
                instr.arg1 += funcObj->entryPc;
            default:

                break;
            }
        }

        funcObj->arity = funcInfo.arity;
        funcObj->selfIndex = m_scopeOffset;
        funcObj->selfVarIndices = funcInfo.selfRefs;
        funcObj->scopeOffset = funcInfo.scopeOffset;

        funcObj->funcMetaData.retType = funcInfo.retType;
        funcObj->funcMetaData.isPublic = funcInfo.isPublic;
        funcObj->funcMetaData.isImmutable = funcInfo.isImmutable;
        funcObj->funcMetaData.isStatic = funcInfo.isStatic;

        for (const auto& argInfo : funcInfo.args) {
            FuncArgMetaData argMeta;
            argMeta.name = argInfo.name;
            argMeta.types = argInfo.types;
            argMeta.isOut = argInfo.isOut;
            argMeta.defVal = ""; // TODO
            funcObj->funcMetaData.argList.append(argMeta);
        }

        // Сохраняем функцию в реестре модуля

        m_funcs.append( funcObj );

        // Если функция публичная, добавляем в глобальный скоуп
/*
        NOTE: Не уверен, что нужно так

        if (funcInfo.isPublic) {
            RuntimeVar var;
            var.name = funcInfo.name;
            var.value = BydaoValue(funcObj,TYPE_FUNC);
            m_scopeStack.append(var);
        }
*/
    }

    return true;
}

void BydaoVM::loadConstants( const QVector<BydaoConstant>& constants ) {
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
                m_constantValues.append(BydaoValue::fromNull());
            }
            break;
        case CONST_BOOL:
            m_constantValues.append(BydaoValue::fromBool(c.boolValue != 0));
            break;
        case CONST_NULL:
            m_constantValues.append(BydaoValue::fromNull());
            break;
        default:
            m_constantValues.append(BydaoValue::fromNull());
            break;
        }
    }
}

bool BydaoVM::load(const QVector<BydaoConstant>& constants,
                   const QVector<QString>& stringTable,
                   const QVector<BydaoInstruction>& code) {
    
    m_constants = constants;
    m_stringTable = stringTable;
    m_code = code;
    
    // Преобразуем константы в готовые значения
    m_constantValues.clear();
    loadConstants( constants );
    
    // Сброс состояния
    m_pc = 0;
    m_stack.clear();
    m_scopeStack.clear();
    m_scopeStack.append( VarScope() );
    m_scopeOffset = 0;

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
    
    return m_lastError.isEmpty();
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
    static BydaoValue nullValue = BydaoValue::fromNull();

    int index = varIndex + m_scopeOffset;
    if ( index < 0 || index >= m_scopeStack.size()) {
        error("Invalid variable index", instr);
        return nullValue;
    }
    return m_scopeStack[ index ].value;
}

const BydaoValue& BydaoVM::getVariable(int varIndex, const BydaoInstruction& instr) const {
    static BydaoValue nullValue = BydaoValue::fromNull();

    Q_UNUSED(instr);

    int index = varIndex + m_scopeOffset;
    if ( index < 0 || index >= m_scopeStack.size()) {
        return nullValue;
    }
    return m_scopeStack[ index ].value;
}

void BydaoVM::setVariable(int varIndex, const BydaoValue& value, const BydaoInstruction& instr) {
    int index = varIndex + m_scopeOffset;
    if ( index < 0 || index >= m_scopeStack.size()) {
        error("Invalid variable index", instr);
        return;
    }
    m_scopeStack[ index ].value = value;
}

BydaoValue BydaoVM::convertValue(const BydaoValue& val, const QString& toType) {
    if (!val.isObject()) {
        if (toType == QStringLiteral( "String" ) ) {
            return BydaoValue::fromString(val.toString());
        }
        return BydaoValue();
    }

    BydaoObject* obj = val.toObject();
    QString fromType = obj->typeName();

    if (fromType == toType) {
        return val;
    }

    // Встроенные приведения
    if (fromType == QStringLiteral( "Int" ) ) {
        qint64 i = static_cast<BydaoInt*>(obj)->value();
        if (toType == QStringLiteral( "Real" )) {
            return BydaoValue::fromReal(static_cast<double>(i));
        }
        if (toType == QStringLiteral( "Bool" ) ) {
            return BydaoValue::fromBool(i != 0);
        }
        if (toType == QStringLiteral( "String" ) ) {
            return BydaoValue::fromString(QString::number(i));
        }
    }
    else if (fromType == QStringLiteral( "Real" )) {
        double r = static_cast<BydaoReal*>(obj)->value();
        if (toType == QStringLiteral( "Int" )) {
            return BydaoValue::fromInt(static_cast<qint64>(r));
        }
        if (toType == QStringLiteral( "Bool" )) {
            return BydaoValue::fromBool(r != 0.0);
        }
        if (toType == QStringLiteral( "String" )) {
            return BydaoValue::fromString(QString::number(r));
        }
    }
    else if (fromType == QStringLiteral( "Bool" ) ) {
        bool b = static_cast<BydaoBool*>(obj)->value();
        if (toType == QStringLiteral( "Int" )) {
            return BydaoValue::fromInt(b ? 1 : 0);
        }
        if (toType == QStringLiteral( "String" )) {
            return BydaoValue::fromString(b ? "true" : "false");
        }
    }
    else if (fromType == QStringLiteral( "String" )) {
        QString s = static_cast<BydaoString*>(obj)->value();
        if (toType == QStringLiteral( "Int" )) {
            bool ok;
            qint64 i = s.toLongLong(&ok);
            if (ok) return BydaoValue::fromInt(i);
        }
        if (toType == QStringLiteral( "Real" )) {
            bool ok;
            double r = s.toDouble(&ok);
            if (ok) return BydaoValue::fromReal(r);
        }
        if (toType == QStringLiteral( "Bool" )) {
            return BydaoValue::fromBool(!s.isEmpty());
        }
    }
    else if (fromType == QStringLiteral("Null")) {
        if (toType == QStringLiteral( "String" )) {
            return BydaoValue::fromString("null");
        }
        if (toType == QStringLiteral( "Bool" )) {
            return BydaoValue::fromBool(false);
        }
        if (toType == QStringLiteral( "Int" )) {
            return BydaoValue::fromInt(0);
        }
        if (toType == QStringLiteral( "Real" )) {
            return BydaoValue::fromReal(0.0);
        }
    }

    // Пытаемся вызвать метод toType
    QString methodName = "to" + toType;
    BydaoValue result;
    if (obj->callMethod(methodName, QVector<BydaoValue>(), result)) {
        return result;
    }

    return BydaoValue();  // null — не удалось привести
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
    int arg1 = instr.arg1;
    int arg2 = instr.arg2;
    switch (instr.op) {

    // ===== Переходы =====
    case BydaoOpCode::Jump: {
        m_pc = arg1;
        break;
    }

    case BydaoOpCode::JumpFalse: {
        BydaoValue cond = m_stack.pop();
        if (!cond.toBool()) {
            m_pc = arg1;
        }
        break;
    }

    case BydaoOpCode::JumpTrue: {
        BydaoValue cond = m_stack.pop();
        if (cond.toBool()) {
            m_pc = arg1;
        }
        break;
    }

    case BydaoOpCode::JumpFalsePeek: {
        BydaoValue cond = m_stack.top();
        if (!cond.toBool()) {
            m_pc = arg1;
        }
        break;
    }

    case BydaoOpCode::JumpTruePeek: {
        BydaoValue cond = m_stack.top();
        if (cond.toBool()) {
            m_pc = arg1;
        }
        break;
    }

    case BydaoOpCode::Load: {
        m_stack.push( getVariable(arg1, instr) );
        break;
    }

    case BydaoOpCode::Store: {
        if ( arg2 > 0 ) {
            setVariable(arg1, getVariable( arg2, instr ), instr);
        }
        else if ( arg2 < 0 ) {
            setVariable(arg1, m_constantValues[ -arg2 ].copy(), instr);
        }
        else {
            setVariable(arg1, m_stack.pop(), instr);
        }
        break;
    }

    case BydaoOpCode::LoadScope: {
        m_stack.push( getVariable(arg1, instr) );
        break;
/*
        int varIndex = arg1;

        if (!m_currentSelfFrame) {
            error("LoadSelf outside module function", instr);
            return false;
        }

        if (varIndex < 0 || varIndex >= m_currentSelfFrame->size()) {
            error("Invalid self variable index", instr);
            return false;
        }

        m_stack.push((*m_currentSelfFrame)[varIndex].value);
        break;
*/
    }

    case BydaoOpCode::StoreScope: {
        setVariable(arg1, m_stack.pop(), instr);
        break;
/*
        int varIndex = arg1;

        if (!m_currentSelfFrame) {
            error("StoreSelf outside module function", instr);
            return false;
        }

        if (varIndex < 0 || varIndex >= m_currentSelfFrame->size()) {
            error("Invalid self variable index", instr);
            return false;
        }

        if (m_stack.isEmpty()) {
            error("Stack underflow in StoreSelf", instr);
            return false;
        }

        (*m_currentSelfFrame)[varIndex].value = m_stack.pop();
        break;
*/
    }

    // ===== Области видимости =====

    case BydaoOpCode::ScopeDrop:
        m_scopeStack.resize(arg1 + m_scopeOffset);
        break;

    case BydaoOpCode::StkPop:
        m_stack.pop();
        break;

    // ===== Переменные =====

    case BydaoOpCode::ConstDecl: {
        QString name = m_stringTable[arg1];
        RuntimeVar var;
        var.name = name;
        var.value = m_constantValues[ arg2 ];
        m_scopeStack.append(var);
        break;
    }

    case BydaoOpCode::VarDecl: {
        if ( arg1 >= 0 ) {    // задано название переменной

            RuntimeVar var;
            var.name = m_stringTable[arg1];

            if ( arg2 < 0 ) {
                // создание переменной и инициализация константным значением
                var.value = m_constantValues[ -arg2 ].copy();
            }
            else if ( arg2 == 1 ) {
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
        int varIndex = arg1 + m_scopeOffset;
        if (varIndex >= 0 && varIndex < m_scopeStack.size()) {
            m_scopeStack.removeAt( varIndex );
        }
        break;
    }

    case BydaoOpCode::VarAdd: {

        if ( arg2 < 0 ) {         // сложение переменной с константой
            BydaoValue& a = getVariable(arg1, instr);
            if (!a.isObject()) {
                error("'+' operation on non-object", instr);
                return false;
            }
            m_stack.push( a.toObject()->add( m_constantValues[ -arg2 ] ) );
        }
        else if ( arg2 > 0 ) {    // сложение двух переменных
            BydaoValue& b = getVariable(arg2, instr);
            BydaoValue& a = getVariable(arg1, instr);
            if (!a.isObject() || !b.isObject()) {
                error("'+' operation on non-object", instr);
                return false;
            }
            m_stack.push( a.toObject()->add(b) );
        }
        else {                          // сложение переменной со значением на стеке
            BydaoValue& a = getVariable(arg1, instr);
            BydaoValue b = m_stack.pop();
            if (!a.isObject() || !b.isObject()) {
                error("'+' operation on non-object", instr);
                return false;
            }
            m_stack.push( a.toObject()->add(b) );
        }
        break;
    }

    case BydaoOpCode::VarSub: {

        if ( arg2 < 0 ) {         // вычитание константы из переменной
            BydaoObject* obj = getVariable(arg1, instr).toObject();
            if ( ! obj ) {
                error("'-' operation on non-object", instr);
                return false;
            }
            m_stack.push( obj->sub( m_constantValues[ -arg2 ] ) );
        }
        else if ( arg2 > 0 ) {    // вычитание двух переменных
            BydaoValue& b = getVariable(arg2, instr);
            BydaoObject* obj = getVariable(arg1, instr).toObject();
            if ( ! obj || !b.isObject()) {
                error("'-' operation on non-object", instr);
                return false;
            }
            m_stack.push( obj->sub(b) );
        }
        else {                          // вычитание значения со стека из переменной
            BydaoValue& b = getVariable(arg1, instr);
            BydaoValue a = m_stack.pop();
            if (!a.isObject() || !b.isObject()) {
                error("'-' operation on non-object", instr);
                return false;
            }
            m_stack.push( a.toObject()->sub(b) );
        }
        break;
    }

    case BydaoOpCode::VarDiv: {

        if ( arg2 < 0 ) {         // деление переменной на константу
            BydaoValue leftVal = getVariable(arg1, instr);
            BydaoObject* leftObj = leftVal.toObject();
            if ( ! leftObj ) {
                error("'/' operation on non-object", instr);
                return false;
            }
            m_stack.push( leftObj->div( m_constantValues[ -arg2 ] ) );
        }
        else if ( arg2 > 0 ) {    // деление двух переменных
            BydaoObject* leftObj = getVariable(arg1, instr).toObject();
            if ( ! leftObj ) {
                error("'/' operation on non-object", instr);
                return false;
            }
            m_stack.push( leftObj->div( getVariable(arg2, instr) ) );
        }
        else {                          // деление переменной на значение на стеке
            error("operation '/' not implemented", instr);
        }
        break;
    }

    case BydaoOpCode::AddStore: {

        if ( arg2 > 0 ) {         // прибавить к переменной значение переменной
            BydaoObject* obj = getVariable(arg1, instr).toObject();
            if ( obj == nullptr ) {
                error("'+=' operation on non-object", instr);
                return false;
            }
            obj->addToValue( getVariable(arg2, instr) );
        }
        else if ( arg2 < 0 ) {    // прибавить к переменной значение константы
            BydaoObject* obj = getVariable(arg1, instr).toObject();
            if ( obj == nullptr ) {
                error("'+=' operation on non-object", instr);
                return false;
            }
            obj->addToValue( m_constantValues[ -arg2 ] );
        }
        else {                          // прибавить к переменной значение со стека
            BydaoObject* obj = getVariable(arg1, instr).toObject();
            if ( obj == nullptr ) {
                error("'+=' operation on non-object", instr);
                return false;
            }
            obj->addToValue( m_stack.pop() );
        }
        break;
    }

    case BydaoOpCode::SubStore: {
        BydaoValue b = m_stack.pop();
        BydaoValue& a = getVariable(arg1, instr);
        if (!a.isObject() || !b.isObject()) {
            error("'-=' operation on non-object", instr);
            return false;
        }

        BydaoValue val = a.toObject()->sub(b);
        setVariable(arg1, val, instr);
        break;
    }

    case BydaoOpCode::MulStore: {
        BydaoValue b = m_stack.pop();
        BydaoValue& a = getVariable(arg1, instr);
        if (!a.isObject() || !b.isObject()) {
            error("'*=' operation on non-object", instr);
            return false;
        }

        BydaoValue val = a.toObject()->mul(b);
        setVariable(arg1, val, instr);
        break;
    }

    case BydaoOpCode::DivStore: {
        BydaoValue b = m_stack.pop();
        BydaoValue& a = getVariable(arg1, instr);
        if (!a.isObject() || !b.isObject()) {
            error("'/=' operation on non-object", instr);
            return false;
        }

        BydaoValue val = a.toObject()->div(b);
        setVariable(arg1, val, instr);
        break;
    }

    case BydaoOpCode::ModStore: {
        BydaoValue b = m_stack.pop();
        BydaoValue& a = getVariable(arg1, instr);
        if (!a.isObject() || !b.isObject()) {
            error("'%=' operation on non-object", instr);
            return false;
        }

        BydaoValue val = a.toObject()->mod(b);
        setVariable(arg1, val, instr);
        break;
    }

    // ===== Константы =====
    case BydaoOpCode::PushConst: {
        if ( arg1 < 0 || m_constantValues.size() <= arg1 ) {
            error("Invalid constant index", instr);
            return false;
        }
        m_stack.push( m_constantValues[arg1] );
        break;
    }
    
    // ===== Арифметика =====
    case BydaoOpCode::Add: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();
        BydaoObject* obj = a.toObject();
        if (!obj || !b.isObject()) {
            error("Add operation on non-object", instr);
            return false;
        }
        
        m_stack.push(obj->add(b));
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

    // ===== Битовые ======

    case BydaoOpCode::BitAnd: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();

        if (!a.isObject() || !b.isObject()) {
            error("bit AND operation on non-object", instr);
            return false;
        }

        m_stack.push(a.toObject()->bitAnd(b));
        break;
    }

    case BydaoOpCode::BitOr: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();

        if (!a.isObject() || !b.isObject()) {
            error("bit OR operation on non-object", instr);
            return false;
        }

        m_stack.push(a.toObject()->bitOr(b));
        break;
    }

    case BydaoOpCode::BitXor: {
        BydaoValue b = m_stack.pop();
        BydaoValue a = m_stack.pop();

        if (!a.isObject() || !b.isObject()) {
            error("bit XOR operation on non-object", instr);
            return false;
        }

        m_stack.push(a.toObject()->bitXor(b));
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

    case BydaoOpCode::VarEq: {
        if ( arg2 < 0 ) {         // Сравнение переменной с константой
            BydaoObject* obj = getVariable( arg1, instr ).toObject();
            BydaoValue& b = m_constantValues[ -arg2 ];
            if ( ! obj || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push( obj->eq(b));

        }
        else if ( arg2 > 0 ) {    // Сравнение двух переменных
            BydaoObject* obj = getVariable( arg1, instr ).toObject();
            BydaoValue& b = getVariable( arg2, instr );
            if ( ! obj || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push(obj->eq(b));
        }
        else {                          // сравнение переменной со значеним на стеке
            BydaoObject* obj = getVariable( arg1, instr ).toObject();
            BydaoValue  b = m_stack.pop();
            if (! obj || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push(obj->eq(b));
        }
        break;
    }

    case BydaoOpCode::VarNeq: {
        if ( arg2 < 0 ) {         // Сравнение переменной с константой
            BydaoValue& a = getVariable( arg1, instr );
            BydaoValue& b = m_constantValues[ -arg2 ];
            if (!a.isObject() || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push(a.toObject()->neq(b));
        }
        else if ( arg2 > 0 ) {    // Сравнение двух переменных
            BydaoValue& a = getVariable( arg1, instr );
            BydaoValue& b = getVariable( arg2, instr );
            if (!a.isObject() || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push(a.toObject()->neq(b));
        }
        else {                          // сравнение переменной со значеним на стеке
            BydaoValue& a = getVariable( arg1, instr );
            BydaoValue  b = m_stack.pop();
            if (!a.isObject() || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push(a.toObject()->neq(b));
        }
        break;
    }

    case BydaoOpCode::VarLt: {
        if ( arg2 < 0 ) {         // Сравнение переменной с константой
            BydaoValue& a = getVariable( arg1, instr );
            BydaoValue& b = m_constantValues[ -arg2 ];
            if (!a.isObject() || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push(a.toObject()->lt(b));

        }
        else if ( arg2 > 0 ) {    // Сравнение двух переменных
            BydaoValue& a = getVariable( arg1, instr );
            BydaoValue& b = getVariable( arg2, instr );
            if (!a.isObject() || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push(a.toObject()->lt(b));
        }
        else {                          // сравнение переменной со значеним на стеке
            BydaoValue& a = getVariable( arg1, instr );
            BydaoValue  b = m_stack.pop();
            if (!a.isObject() || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push(a.toObject()->lt(b));
        }
        break;
    }

    case BydaoOpCode::VarLe: {
        if ( arg2 < 0 ) {         // Сравнение переменной с константой
            BydaoObject* obj = getVariable( arg1, instr ).toObject();
            BydaoValue& b = m_constantValues[ -arg2 ];
            if ( ! obj || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push( obj->le(b));

        }
        else if ( arg2 > 0 ) {    // Сравнение двух переменных
            BydaoObject* obj = getVariable( arg1, instr ).toObject();
            BydaoValue& b = getVariable( arg2, instr );
            if ( ! obj || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push( obj->le(b));
        }
        else {                          // сравнение переменной со значеним на стеке
            BydaoObject* obj = getVariable( arg1, instr ).toObject();
            BydaoValue  b = m_stack.pop();
            if ( ! obj || !b.isObject()) {
                error("Comparison with non-object", instr);
                return false;
            }
            m_stack.push( obj->le(b));
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

    case BydaoOpCode::EqNull: {
        if ( arg1 > 0 ) {   // сравнение переменной на равенство null
            BydaoValue& a = getVariable( arg1, instr );
            m_stack.push( BydaoValue::fromBool( a.isNull() ) );
        }
        else {              // сравнение значения на стеке на равенство null
            BydaoValue a = m_stack.pop();
            m_stack.push( BydaoValue::fromBool( a.isNull() ) );
        }
        break;
    }

    case BydaoOpCode::NeqNull: {
        if ( arg1 > 0 ) {   // сравнение переменной на неравенство null
            BydaoValue& a = getVariable( arg1, instr );
            m_stack.push( BydaoValue::fromBool( ! a.isNull() ) );
        }
        else {              // сравнение значения на стеке на неравенство null
            BydaoValue a = m_stack.pop();
            m_stack.push( BydaoValue::fromBool( ! a.isNull() ) );
        }
        break;
    }

    case BydaoOpCode::IsType: {
        QString typeName = m_stringTable[ arg2 ];
        if ( arg1 > 0 ) {   // сравнение типа переменной
            BydaoValue& a = getVariable( arg1, instr );
            m_stack.push( a.isObject()
                             ? BydaoValue::fromBool( a.toObject()->typeName() == typeName )
                             : BydaoValue::fromBool( typeName == QStringLiteral("Null") )
                         );
        }
        else {              // сравнение типа значения на стеке
            BydaoValue a = m_stack.pop();
            m_stack.push( a.isObject()
                             ? BydaoValue::fromBool( a.toObject()->typeName() == typeName )
                             : BydaoValue::fromBool( typeName == QStringLiteral("Null") )
                         );
        }
        break;
    }

    case BydaoOpCode::NotType: {
        QString typeName = m_stringTable[ arg2 ];
        if ( arg1 > 0 ) {   // сравнение типа переменной
            BydaoValue& a = getVariable( arg1, instr );
            m_stack.push( a.isObject()
                             ? BydaoValue::fromBool( a.toObject()->typeName() != typeName )
                             : BydaoValue::fromBool( typeName != QStringLiteral("Null") )
                         );
        }
        else {              // сравнение типа значения на стеке
            BydaoValue a = m_stack.pop();
            m_stack.push( a.isObject()
                             ? BydaoValue::fromBool( a.toObject()->typeName() != typeName )
                             : BydaoValue::fromBool( typeName != QStringLiteral("Null") )
                         );
        }
        break;
    }

    // ===== Логические операции =====
    
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

        setVariable( arg1, it, instr );
        break;
    }

    case BydaoOpCode::ItNext: {
        auto* obj = getVariable(arg1, instr).toObject();
        if ( ! obj ) {
            error("ITNEXT on non-object", instr);
            return false;
        }
        m_stack.push( BydaoValue( BydaoBool::create( obj->callBoolMethod(0) ), BydaoTypeId::TYPE_BOOL ) );
        break;
    }
    
    case BydaoOpCode::ItValue: {
        auto* obj = getVariable(arg1, instr).toObject();
        if ( ! obj ) {
            error("ITVALUE on non-object", instr);
            return false;
        }
        if ( arg2 == 0 ) {
            m_stack.push( obj->callValueMethod(0) );
        }
        else {
            setVariable( arg2, obj->callValueMethod(0), instr );
        }
        break;
    }
    
    case BydaoOpCode::ItKey: {
        auto* obj = getVariable( arg1,instr).toObject();
        if ( ! obj ) {
            error("ITKEY on non-object", instr);
            return false;
        }
        if ( arg2 == 0 ) {
            m_stack.push( obj->callValueMethod( 1 ) );
        }
        else {
            setVariable( arg2, obj->callValueMethod( 1 ), instr );
        }
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

        if (arg1 < 0 || m_stringTable.size() <= arg1 ) {
            error("Invalid member name index", instr);
            return false;
        }
        QString memberName = m_stringTable[arg1];

        BydaoObject* obj;
        if ( arg2 > 0 ) {
            obj = getVariable( arg2, instr ).toObject();
        }
        else {
            if (m_stack.isEmpty()) {
                error("Stack underflow in MEMBER", instr);
                return false;
            }
            obj = m_stack.pop().toObject();
        }
        if ( ! obj ) {
            if ( memberName == QStringLiteral("type") ) {
                BydaoValue value( BydaoString::create(QStringLiteral("Null")), TYPE_STRING );
                m_stack.push( value );
                break;
            }
            error("MEMBER on non-object", instr);
            return false;
        }

        BydaoValue value;
        if ( ! obj->getVar( memberName, value ) ) {
            // Свойство не найдено
            QString objType = obj->typeName();
            error(QString("Object of type '%1' does not have variable '%2'").arg(objType, memberName), instr);
            return false;
        }

        m_stack.push( value );
        break;
    }

    case BydaoOpCode::SetMember: {
        if (m_traceMode ) {
            dumpStack("Before SETMEMBER at PC " + QString::number(m_pc-1));
        }

        if (m_stack.isEmpty()) {
            error("Stack underflow in SETMEMBER", instr);
            return false;
        }

        BydaoValue val = m_stack.pop();
        BydaoValue obj = m_stack.pop();

        if (!obj.isObject()) {
            error("MEMBER on non-object", instr);
            return false;
        }

        if (arg1 < 0 || m_stringTable.size() <= arg1 ) {
            error("Invalid member name index", instr);
            return false;
        }
        QString memberName = m_stringTable[arg1];

        if ( ! obj.toObject()->setVar( memberName, val ) ) {
            QString objType = obj.toObject()->typeName();
            error(QString("Cannot set value to member '%1' of object type '%2'").arg(memberName,objType), instr);
            return false;
        }

        break;
    }

    case BydaoOpCode::Method: {
        if ( m_traceMode ) {
            dumpStack("Before METHOD at PC " + QString::number(m_pc-1));
        }

        if (m_stack.isEmpty()) {
            error("Stack underflow in METHOD", instr);
            return false;
        }

        // BydaoValue obj = m_stack.pop();
        // if (!obj.isObject()) {
        //     error("METHOD on non-object", instr);
        //     return false;
        // }

        if (arg1 < 0 || m_stringTable.size() <= arg1 ) {
            error("Invalid method name index", instr);
            return false;
        }
        QString methodName = m_stringTable[arg1];

        // Кладём объект и имя метода на стек для последующего CALL
        // m_stack.push(obj);
        m_stack.push(BydaoValue::fromString(methodName));

        if ( m_traceMode ) {
            qDebug() << "  prepared for method call";
            dumpStack("After METHOD");
        }
        break;
    }

    case BydaoOpCode::Call:
    case BydaoOpCode::CallVoid: {
        int argCount = arg1;
        if ( m_traceMode ) {
            dumpStack( QString("Before CALL at PC %1, argCount = %2").arg( m_pc-1 ).arg( argCount ) );
        }

        if ( m_stack.size() < argCount ) {
            error("Stack underflow: not enough arguments", instr);
            return false;
        }

        // Забираем аргументы
        QVector<BydaoValue> args;
        args.resize( argCount );
        while ( --argCount >= 0 ) {
            m_stack.popTo( args[ argCount ] );
        }

        BydaoValue result;
        BydaoObject* obj;
        bool ok;
        if ( arg2 < 0 ) { // вызов метода объекта по имени

            // Имя метода должно быть на стеке (от предыдущей инструкции METHOD)
            if (m_stack.size() < 2) {
                error("Stack underflow: no method name and object for CALL", instr);
                return false;
            }

            // Получить и проверить имя метода
            const BydaoValue& nameValue = m_stack.pop();
            if ( ! nameValue.isObject() || nameValue.typeId() != TYPE_STRING) {

                QString valueDesc = nameValue.isObject() ? nameValue.toObject()->typeName() : "non-object";
                error( QString("try call unknown function '%1'").arg(valueDesc), instr);
                return false;
            }
            QString methodName = nameValue.toString();

            // Получить объект, метод которого вызывается
            const BydaoValue& objValue = m_stack.pop();
            obj = objValue.toObject();
            if ( ! obj ) {
                error( QString("call function '%1' of non-object").arg( methodName ), instr);
                return false;
            }

            // Вызвать метод объекта
            ok = obj->callMethod(methodName, args, result);
        }
        else {                  // вызов метода объекта по индексу

            // Получить объект, метод которого вызывается
            obj = m_stack.pop().toObject();
            if ( ! obj ) {
                error( QString("call function of non-object"), instr);
                return false;
            }

            // Вызывать метод объекта по индексу
            ok = obj->callStdMethod( arg2, args, result );
        }

        // Обработать результат вызова метода

        if ( ok ) {
            if ( instr.op == BydaoOpCode::Call ) {
                m_stack.push(result);
            }
            if ( m_traceMode ) {
                dumpStack("After CALL");
            }
        }
        else {
            QString err = obj->lastError();
            if ( ! err.isEmpty() ) {
                error( err, instr);
            }
            else {
                error( QString("Unknown error on call function of type '%1'").arg(obj->typeName()), instr);
            }
            return false;
        }
        break;
    }

    case BydaoOpCode::Ignore: {

        if ( m_stack.size() <= 0 ) {
            error("Stack underflow: not enough arguments", instr);
            return false;
        }
        m_stack.pop();
        break;
    }

    case BydaoOpCode::NewObj: {
        int argCount = arg1;
        if ( m_stack.size() < argCount ) {
            error("Stack underflow: not enough arguments", instr);
            return false;
        }

        // Забираем аргументы
        QVector<BydaoValue> args;
        args.resize( argCount );
        while ( --argCount >= 0 ) {
            m_stack.popTo( args[ argCount ] );
        }

        BydaoValue result;
        BydaoObject* obj;
        bool ok;
        if ( arg2 < 0 ) { // вызов метода объекта по имени

            QString methodName = "new";

            // Получить объект, метод которого вызывается
            const BydaoValue& objValue = m_stack.pop();
            obj = objValue.toObject();
            if ( ! obj ) {
                error( QString("create object of non-object").arg( methodName ), instr);
                return false;
            }

            // Вызвать метод объекта
            ok = obj->callMethod(methodName, args, result);
        }
        else {                  // вызов метода объекта по индексу

            // Получить объект, метод которого вызывается
            obj = m_stack.pop().toObject();
            if ( ! obj ) {
                error( QString("create object of non-object"), instr);
                return false;
            }

            // Вызывать метод объекта по индексу
            ok = obj->callStdMethod( arg2, args, result );
        }

        // Обработать результат вызова метода

        if ( ok ) {
            m_stack.push(result);
            if ( m_traceMode ) {
                dumpStack("After NEWOBJ");
            }
        }
        else {
            QString err = obj->lastError();
            if ( ! err.isEmpty() ) {
                error( err, instr);
            }
            else {
                error( QString("Unknown error on create object of type '%1'").arg(obj->typeName()), instr);
            }
            return false;
        }
        break;
    }

    // ===== Модули и типы =====
    case BydaoOpCode::UseModule: {

        QString moduleName;
        QString alias;
        
        if (arg1 >= 0 && arg1 < m_stringTable.size()) {
            moduleName = m_stringTable[arg1];
        } else {
            error("Invalid module name index", instr);
            return false;
        }
        
        if (arg2 >= 0 && arg2 < m_stringTable.size()) {
            alias = m_stringTable[arg2];
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

        int varIndex = m_scopeStack.size();

        RuntimeVar var;
        var.name = alias;
        var.value = BydaoValue(module, BydaoTypeId::TYPE_MODULE);
        m_scopeStack.append(var);

        m_moduleName[alias] = varIndex;
        
        break;
    }
    
    case BydaoOpCode::UseBuiltin: {
        if ( arg1 < 0 || m_stringTable.size() <= arg1 ) {
            error("Invalid builtin type name index", instr);
            return false;
        }
        BydaoValue& builtinType = getBuiltinType(arg1);
        if ( builtinType.isNull() ) {
            error( QString("Unknown builtin type '%1'").arg( m_stringTable[arg2] ), instr);
            return false;
        }
        
        m_stack.push(builtinType);
        break;
    }
    
    // ===== Массивы =====
    case BydaoOpCode::PushArray: {
        int count = arg1;

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
        m_stack.push( BydaoValue(array, BydaoTypeId::TYPE_ARRAY) );
        break;
    }

    // ===== Функции =====

    case BydaoOpCode::FuncDecl: {
        RuntimeVar var;
        var.name = m_stringTable[arg1];
        var.value = BydaoValue( m_funcs[arg2], TYPE_FUNC );
        m_scopeStack.append(var);
        break;
    }

    case BydaoOpCode::CallFunc:
    case BydaoOpCode::CallFuncVoid: {

        BydaoFuncObject* func;
        if ( arg2 < 0 ) {  // косвенный вызов через переменную

            BydaoValue val = m_stack.pop();
            if ( ! val.isObject() ) {
                error( "Call non-object as function", instr );
                return false;
            }
            func = (BydaoFuncObject*)val.toObject();
        }
        else {
            func = m_funcs[ arg2 ];
        }

        // Проверяем количество аргументов
        int argCount = arg1;
        if (argCount != func->arity) {
            error(QString("Argument count mismatch: expected %1, got %2")
                      .arg(func->arity).arg(argCount), instr);
            return false;
        }

        // Создаём новый фрейм
        int scopeDeep = m_scopeStack.size();
        CallFrame frame;
//        frame.func = func;
        frame.returnPc = m_pc;
        frame.scopeDeep = scopeDeep;
        frame.scopeOffset = m_scopeOffset;

        // Копируем аргументы в локальные переменные

//        appendScope( argCount );
        m_scopeStack.resizeForOverwrite( scopeDeep + argCount );
        m_scopeOffset = ( arg2 < 0 ) ? func->scopeOffset : scopeDeep;

        while ( --argCount >= 0 ) {
            RuntimeVar& var = m_scopeStack[ m_scopeOffset + argCount ];
            var.name = func->funcMetaData.argList[ argCount ].name;
            m_stack.popTo( var.value );
        }

        // Сохраняем фрейм
        m_callStack.push(frame);

        // Переключаемся на функцию
        m_pc = func->entryPc;

        break;
    }

    case BydaoOpCode::Return: {
/*
        bool hasValue = arg1;
        BydaoValue retVal;

        if (hasValue) {
            if (m_stack.isEmpty()) {
                error("Stack underflow in RETURN", instr);
                return false;
            }
            retVal = m_stack.pop();
        }
*/
        if (m_callStack.isEmpty()) {
            // Возврат из главного модуля — завершаем программу
            m_running = false;
            break;
        }

        // Восстанавливаем фрейм вызывающего
        CallFrame frame = m_callStack.pop();

        // Обрабатываем out-аргументы
        // for (int i = 0; i < frame.outVarIndices.size(); i++) {
        //     int outIdx = frame.outVarIndices[i];
        //     if (outIdx >= 0 && outIdx < frame.localVars.size()) {
        //         // Записываем значение out-аргумента в переменную вызывающего
        //         // (адрес был передан через PushAddr)
        //         // TODO: реализовать механизм адресов
        //     }
        // }

        // Восстанавливаем скоуп вызывающего

//        dropScope();
        m_scopeStack.resize( frame.scopeDeep );
        m_scopeOffset = frame.scopeOffset;

        m_pc = frame.returnPc;
/*
        // Кладём результат на стек, если функция не void
        if (frame.func->funcMetaData.retType != "Void") {
            m_stack.push(retVal);
        }
*/
        break;
    }

/*
    case BydaoOpCode::PushAddr: {
        int varIndex = arg1;

        // Создаём специальное значение-ссылку на переменную
        // Для простоты пока кладём индекс переменной вызывающего
        // В будущем можно сделать отдельный тип Reference

        // Сохраняем индекс в текущем фрейме для out-аргументов
        if (!m_callStack.isEmpty()) {
            m_callStack.top().outVarIndices.append(varIndex);
        }

        // Кладём на стек значение переменной (для совместимости)
        if (varIndex < 0 || m_scopeStack.size() <= varIndex ) {
            error("Invalid variable index in PushAddr", instr);
            return false;
        }
        m_stack.push(m_scopeStack[varIndex].value);
        break;
    }
*/

    // ===== Управление =====
    case BydaoOpCode::Nop:
        break;

    case BydaoOpCode::Halt:
        if ( arg1 > 0 ) {
            BydaoValue val = m_stack.pop();
            if ( val.isObject() ) {
                QString str;
                if ( val.typeId() != TYPE_STRING ) {
                    val = convertValue( val, "String" );
                }
                if ( val.typeId() == TYPE_STRING ) {
                    str = val.toString();
                }
                if ( ! str.isEmpty() ) {
                    QTextStream out(stdout);
                    out.setEncoding(QStringConverter::Utf8);
                    out << str << "\n";
                }
            }
        }
        m_exitStatus = arg2;
        m_running = false;
        break;

    default:
        error("Unknown opcode: " + QString::number((int)instr.op), instr);
        return false;
    }
    
    return true;
}

BydaoValue&     BydaoVM::getBuiltinType( int index ) {
    static BydaoValue nullValue = BydaoValue();
    if ( 0 <= index && index < s_builtinTypes.size() ) {
        return s_builtinTypes[ index ].typeValue;
    }
    return nullValue;
}

} // namespace BydaoScript
