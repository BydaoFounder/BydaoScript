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
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"
#include <cmath>

namespace BydaoScript {

// Получить мета-данные
MetaData*   BydaoReal::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData
            // методы объекта
            ->append( "toString", FuncMetaData("String", false, true) )
            .append( "toFixed",   FuncMetaData("String", false, true) << FuncArgMetaData("arg","Int",false) )
            .append( "toInt",     FuncMetaData("Int", false, true) )
            .append( "toBool",    FuncMetaData("Bool", false, true) )
            .append( "abs",       FuncMetaData("Real", false, true) )
            .append( "isNull",    FuncMetaData("Bool", false, true) )
            .append( "round",     FuncMetaData("Real", false, true) << FuncArgMetaData("decimal","Int",false) )
            .append( "floor",     FuncMetaData("Real", false, true) )
            .append( "ceil",      FuncMetaData("Real", false, true) )
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
            .append( "add",     OperMetaData("Real", "Real" ).append( "Int", "Real" )
                               .append( "String", "String" ).append( "Any", "Real" ) )
            .append( "addToValue",  OperMetaData("Real", "Void" ).append( "Int", "Void" ).append( "Any", "Void" ) )
            .append( "sub",     OperMetaData("Real", "Real" )
                               .append( "Int", "Real" )
                               .append( "Any", "Real" )
                    )
            .append( "mul",     OperMetaData("Real", "Real" )
                               .append( "Any", "Real" )
                    )
            .append( "div",     OperMetaData("Real", "Real" )
                               .append( "Any", "Real" )
                    )
            .append( "neg",     OperMetaData("", "Real" ) )
            ;
    }
    return metaData;
}

QList<BydaoReal*> BydaoReal::s_cache;

BydaoReal::BydaoReal(double value)
    : BydaoObject()
    , m_value(value)
{
    registerMethod("toString", &BydaoReal::method_toString);
    registerMethod("toFixed", &BydaoReal::method_toFixed);
    registerMethod("toInt", &BydaoReal::method_toInt);
    registerMethod("toBool", &BydaoReal::method_toBool);
    registerMethod("abs", &BydaoReal::method_abs);
    registerMethod("isNull", &BydaoReal::method_isNull);
    registerMethod("round", &BydaoReal::method_round);
    registerMethod("floor", &BydaoReal::method_floor);
    registerMethod("ceil", &BydaoReal::method_ceil);
}

void BydaoReal::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoReal::callMethod(const QString& name,
                          const BydaoValueList& args,
                          BydaoValue* result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

bool BydaoReal::method_toString(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set(BydaoString::create(QString::number(m_value)));
    return true;
}

bool BydaoReal::method_toFixed(const BydaoValueList& args, BydaoValue* result) {
    int decimals = args.size() > 0 ? (int)args[0]->toInt() : 0;
    result->set(BydaoString::create(QString::number(m_value,'f',decimals)));
    return true;
}

bool BydaoReal::method_toInt(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set(BydaoInt::create((qint64)m_value));
    return true;
}

bool BydaoReal::method_toBool(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set(BydaoBool::create(m_value != 0.0));
    return true;
}

bool BydaoReal::method_abs(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoReal::create(std::abs(m_value)));
    return true;
}

bool BydaoReal::method_isNull(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set(BydaoBool::create(false));
    return true;
}

bool BydaoReal::method_round(const BydaoValueList& args, BydaoValue* result) {
    int decimals = args.size() > 0 ? (int)args[0]->toInt() : 0;
    double multiplier = std::pow(10.0, decimals);
    double rounded = std::round(m_value * multiplier) / multiplier;
    result->set( BydaoReal::create(rounded));
    return true;
}

bool BydaoReal::method_floor(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoReal::create(std::floor(m_value)));
    return true;
}

bool BydaoReal::method_ceil(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoReal::create(std::ceil(m_value)));
    return true;
}

// ========== Операции ==========

BydaoValue* BydaoReal::add(const BydaoValue* other) {
    switch (other->typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other->toObject());
        return BydaoValue::get( BydaoReal::create(m_value + otherInt->value()) );
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other->toObject());
        return BydaoValue::get( BydaoReal::create(m_value + otherReal->m_value) );
    }
    default:
        return BydaoValue::get( BydaoReal::create(m_value + other->toReal()) );
    }
}

void    BydaoReal::addToValue(const BydaoValue* other) {
    switch (other->typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other->toObject());
        m_value += otherInt->value();
        break;
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other->toObject());
        m_value += otherReal->m_value;
        break;
    }
    default:
        m_value += other->toReal();
    }
}

BydaoValue* BydaoReal::sub(const BydaoValue* other) {
    switch (other->typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other->toObject());
        return BydaoValue::get( BydaoReal::create(m_value - otherInt->value()) );
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other->toObject());
        return BydaoValue::get( BydaoReal::create(m_value - otherReal->m_value) );
    }
    default:
        return BydaoValue::get( BydaoReal::create(m_value - other->toReal()) );
    }
}

BydaoValue* BydaoReal::mul(const BydaoValue* other) {
    switch (other->typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other->toObject());
        return BydaoValue::get( BydaoReal::create(m_value * otherInt->value()) );
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other->toObject());
        return BydaoValue::get( BydaoReal::create(m_value * otherReal->m_value) );
    }
    default:
        return BydaoValue::get( BydaoReal::create(m_value * other->toReal()) );
    }
}

BydaoValue* BydaoReal::div(const BydaoValue* other) {
    switch (other->typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other->toObject());
        if (otherInt->value() == 0) {
            return BydaoValue::get();  // null
        }
        return BydaoValue::get( BydaoReal::create( m_value / otherInt->value()) );
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other->toObject());
        if (otherReal->m_value == 0.0) {
            return BydaoValue::get();
        }
        return BydaoValue::get( BydaoReal::create(m_value / otherReal->m_value) );
    }
    }
    double d = other->toReal();
    if ( d == 0.0 ) {
        return BydaoValue::get();
    }
    return BydaoValue::get( BydaoReal::create(m_value / d ) );
}

BydaoValue* BydaoReal::neg() {
    return BydaoValue::get( BydaoReal::create(-m_value) );
}

BydaoValue* BydaoReal::eq(const BydaoValue* other) {
    switch (other->typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value == otherInt->value()) );
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value == otherReal->m_value) );
    }
    default:
        return BydaoValue::get( BydaoBool::create(m_value == other->toReal()) );
    }
}

BydaoValue* BydaoReal::neq(const BydaoValue* other) {
    switch (other->typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value != otherInt->value()) );
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value != otherReal->m_value) );
    }
    default:
        return BydaoValue::get( BydaoBool::create(m_value != other->toReal()) );
    }
}

BydaoValue* BydaoReal::lt(const BydaoValue* other) {
    switch (other->typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value < otherInt->value()) );
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value < otherReal->m_value) );
    }
    default:
        return BydaoValue::get( BydaoBool::create(m_value < other->toReal()) );
    }
}

BydaoValue* BydaoReal::le(const BydaoValue* other) {
    switch (other->typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value <= otherInt->value()) );
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value <= otherReal->m_value) );
    }
    default:
        return BydaoValue::get( BydaoBool::create(m_value <= other->toReal()) );
    }
}

BydaoValue* BydaoReal::gt(const BydaoValue* other) {
    switch (other->typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value > otherInt->value()) );
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value > otherReal->m_value) );
    }
    default:
        return BydaoValue::get( BydaoBool::create(m_value > other->toReal()) );
    }
}

BydaoValue* BydaoReal::ge(const BydaoValue* other) {
    switch (other->typeId()) {
    case TYPE_INT: {
        const auto* otherInt = (const BydaoInt*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value >= otherInt->value()) );
    }
    case TYPE_REAL: {
        const auto* otherReal = (const BydaoReal*)(other->toObject());
        return BydaoValue::get( BydaoBool::create(m_value >= otherReal->m_value) );
    }
    default:
        return BydaoValue::get( BydaoBool::create(m_value >= other->toReal()) );
    }
}

} // namespace BydaoScript
