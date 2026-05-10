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
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"

namespace BydaoScript {

QVector<BydaoInt*> BydaoInt::s_cache;

// Получить мета-данные
MetaData*   BydaoInt::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData

            // методы объекта
            ->append( "abs",    FuncMetaData(0,"Int", false, true) )
            .append( "negate",  FuncMetaData(1,"Int", false, true) )
            .append( "toString",FuncMetaData(2,"String", false, true)
                                    << FuncArgMetaData("arg","Int",false,"10") )
            .append( "toHex",   FuncMetaData(3,"String", false, true) )
            .append( "toBin",   FuncMetaData(4,"String", false, true) )
            .append( "toReal",  FuncMetaData(5,"Real", false, true) )
            .append( "toBool",  FuncMetaData(6,"Bool", false, true) )
            .append( "isNull",  FuncMetaData(7,"Bool", false, true) )
            ;

        metaData

            // операции сравнения
            ->append( "eq",     OperMetaData("Any", "Bool" ) )
            .append( "neq",     OperMetaData("Any", "Bool" ) )
            .append( "lt",      OperMetaData("Any", "Bool" ) )
            .append( "le",      OperMetaData("Any", "Bool" ) )
            .append( "gt",      OperMetaData("Any", "Bool" ) )
            .append( "ge",      OperMetaData("Any", "Bool" ) )

            // операции арифметические
            .append( "add",     OperMetaData("Int", "Int" )
                               .append( "Real", "Real" )
                               .append( "String", "String" )
                               .append( "Any", "Int" )
                    )
            .append( "addToValue",  OperMetaData("Int", "Void" )
                                      .append( "Real", "Void" )
                                      .append( "Any", "Void" )
                    )
            .append( "sub",     OperMetaData("Int", "Int" )
                               .append( "Real", "Real" )
                               .append( "String", "Int" )
                               .append( "Any", "Int" )
                    )
            .append( "mul",     OperMetaData("Int", "Int" )
                               .append( "Real", "Real" )
                               .append( "Any", "Int" )
                    )
            .append( "div",     OperMetaData("Int", "Int" )
                               .append( "Real", "Real" )
                               .append( "Any", "Int" )
                    )
            .append( "mod",     OperMetaData("Any", "Int" ) )
            .append( "neg",     OperMetaData("", "Int" ) )
            ;
    }
    return metaData;
}

BydaoInt::BydaoInt(qint64 value)
    : BydaoObject()
    , m_value(value)
{
    registerMethod("abs",      &BydaoInt::method_abs);
    registerMethod("negate",   &BydaoInt::method_negate);
    registerMethod("toString", &BydaoInt::method_toString);
    registerMethod("toHex",    &BydaoInt::method_toHex);
    registerMethod("toBin",    &BydaoInt::method_toBin);
    registerMethod("toReal",   &BydaoInt::method_toReal);
    registerMethod("toBool",   &BydaoInt::method_toBool);
    registerMethod("isNull",   &BydaoInt::method_isNull);

    // Регистрация функций для вызова по индексу

    m_stdMethodTable.resize(8);
    m_stdMethodTable[0] = &BydaoInt::absImpl;
    m_stdMethodTable[1] = &BydaoInt::negImpl;
    m_stdMethodTable[2] = &BydaoInt::toStringImpl;
    m_stdMethodTable[3] = &BydaoInt::toHexImpl;
    m_stdMethodTable[4] = &BydaoInt::toBinImpl;
    m_stdMethodTable[5] = &BydaoInt::toRealImpl;
    m_stdMethodTable[6] = &BydaoInt::toBoolImpl;
    m_stdMethodTable[7] = &BydaoInt::isNullImpl;
}

void BydaoInt::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoInt::callMethod(const QString& name,
                          const QVector<BydaoValue>& args,
                          BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

bool BydaoInt::method_abs(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoInt::create(m_value >= 0 ? m_value : -m_value), BydaoTypeId::TYPE_INT );
    return true;
}

bool BydaoInt::method_negate(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoInt::create(-m_value), BydaoTypeId::TYPE_INT);
    return true;
}

bool BydaoInt::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    int base = ( args.size() == 1 ) ? args[0].toInt() : 10;
    result = BydaoValue(BydaoString::create(QString::number(m_value,base)), BydaoTypeId::TYPE_STRING );
    return true;
}

bool BydaoInt::method_toHex(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoString::create( QString::number(m_value, 16)), BydaoTypeId::TYPE_STRING );
    return true;
}

bool BydaoInt::method_toBin(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoString::create( QString::number(m_value, 2)), BydaoTypeId::TYPE_STRING );
    return true;
}

bool BydaoInt::method_toReal(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue::fromReal( (double)m_value );
    return true;
}

bool BydaoInt::method_toBool(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoBool::create(m_value != 0), BydaoTypeId::TYPE_BOOL);
    return true;
}

bool BydaoInt::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoBool::create(false), BydaoTypeId::TYPE_BOOL );
    return true;
}

// ========== Операции ==========

BydaoValue BydaoInt::bitAnd(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        return BydaoValue( BydaoInt::create(m_value & ((BydaoInt*)other.toObject())->m_value), BydaoTypeId::TYPE_INT );
    }
    default:
        return BydaoValue::fromInt(m_value & other.toInt());
    }
}

BydaoValue BydaoInt::bitOr(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        return BydaoValue( BydaoInt::create(m_value | ((BydaoInt*)other.toObject())->m_value), BydaoTypeId::TYPE_INT );
    }
    default:
        return BydaoValue::fromInt(m_value | other.toInt());
    }
}

BydaoValue BydaoInt::bitXor(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        return BydaoValue( BydaoInt::create(m_value ^ ((BydaoInt*)other.toObject())->m_value), BydaoTypeId::TYPE_INT );
    }
    default:
        return BydaoValue::fromInt(m_value ^ other.toInt());
    }
}

BydaoValue BydaoInt::add(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        return BydaoValue( BydaoInt::create(m_value + ((BydaoInt*)other.toObject())->m_value), BydaoTypeId::TYPE_INT );
    }
    case TYPE_REAL: {
        const auto* otherReal = (BydaoReal*)(other.toObject());
        return BydaoValue::fromReal(m_value + otherReal->value());
    }
    case TYPE_STRING: {
        // Конкатенация: число + строка = строка
        QString str = QString::number(m_value) + other.toString();
        return BydaoValue::fromString(str);
    }
    default:
        return BydaoValue::fromInt(m_value + other.toInt());
    }
}

void        BydaoInt::addToValue(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        m_value += ((BydaoInt*)other.toObject())->m_value;
        break;
    }
    case TYPE_REAL: {
        const auto* otherReal = (BydaoReal*)(other.toObject());
        m_value += qint64( otherReal->value() );
        break;
    }
    default:
        m_value += other.toInt();
    }
}

BydaoValue BydaoInt::sub(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (BydaoInt*)(other.toObject());
        return BydaoValue::fromInt(m_value - otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = (BydaoReal*)(other.toObject());
        return BydaoValue::fromReal(m_value - otherReal->value());
    }
    case TYPE_STRING: {
        bool ok;
        qint64 val = other.toString().toLongLong(&ok);
        if (ok) return BydaoValue::fromInt(m_value - val);
        return BydaoValue();
    }
    default:
        return BydaoValue::fromInt(m_value - other.toInt());
    }
}

BydaoValue BydaoInt::mul(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (BydaoInt*)(other.toObject());
        return BydaoValue::fromInt(m_value * otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = (BydaoReal*)(other.toObject());
        return BydaoValue::fromReal(m_value * otherReal->value());
    }
    default:
        return BydaoValue::fromInt(m_value * other.toInt());
    }
}

BydaoValue BydaoInt::div(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (BydaoInt*)(other.toObject());
        qint64 otherVal = otherInt->m_value;
        if (otherVal != 0) {
            return BydaoValue::fromInt( m_value / otherVal );
        }
        return BydaoValue();  // null
    }
    case TYPE_REAL: {
        const auto* otherReal = (BydaoReal*)(other.toObject());
        if (otherReal->value() == 0.0) {
            return BydaoValue();
        }
        return BydaoValue::fromReal( double(m_value) / otherReal->value() );
    }
    default:
        if (other.toInt() == 0) {
            return BydaoValue();
        }
        return BydaoValue::fromInt(m_value / other.toInt());
    }
}

BydaoValue BydaoInt::mod(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (BydaoInt*)(other.toObject());
        if (otherInt->m_value == 0) {
            return BydaoValue();  // null
        }
        return BydaoValue::fromInt(m_value % otherInt->m_value );
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other.toObject());
        if (otherReal->value() == 0.0) {
            return BydaoValue();
        }
        return BydaoValue::fromInt(m_value % qint64(otherReal->value()) );
    }
    default:
        return BydaoValue::fromInt(m_value % other.toInt());
    }
}

BydaoValue BydaoInt::neg() {
    return BydaoValue::fromInt(-m_value);
}

BydaoValue BydaoInt::eq(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other.toObject());
        return BydaoValue::fromBool(m_value == otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other.toObject());
        return BydaoValue::fromBool(m_value == otherReal->value());
    }
    default:
        return BydaoValue::fromBool(m_value == other.toInt());
    }
}

BydaoValue BydaoInt::neq(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other.toObject());
        return BydaoValue::fromBool(m_value != otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other.toObject());
        return BydaoValue::fromBool(m_value != otherReal->value());
    }
    default:
        return BydaoValue::fromBool(m_value != other.toInt());
    }
}

BydaoValue BydaoInt::lt(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other.toObject());
        return BydaoValue::fromBool(m_value < otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other.toObject());
        return BydaoValue::fromBool(m_value < otherReal->value());
    }
    default:
        return BydaoValue::fromBool(m_value < other.toInt());
    }
}

BydaoValue BydaoInt::le(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other.toObject());
        return BydaoValue::fromBool(m_value <= otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other.toObject());
        return BydaoValue::fromBool(m_value <= otherReal->value());
    }
    default:
        return BydaoValue::fromBool(m_value <= other.toInt());
    }
}

BydaoValue BydaoInt::gt(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other.toObject());
        return BydaoValue::fromBool(m_value > otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other.toObject());
        return BydaoValue::fromBool(m_value > otherReal->value());
    }
    default:
        return BydaoValue::fromBool(m_value > other.toInt());
    }
}

BydaoValue BydaoInt::ge(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other.toObject());
        return BydaoValue::fromBool(m_value >= otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other.toObject());
        return BydaoValue::fromBool(m_value >= otherReal->value());
    }
    default:
        return BydaoValue::fromBool(m_value >= other.toInt());
    }
}

} // namespace BydaoScript
