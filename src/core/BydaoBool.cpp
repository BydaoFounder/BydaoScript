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
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoString.h"

namespace BydaoScript {

// Получить мета-данные
MetaData*   BydaoBool::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData
            // методы объекта
            ->append( "toString",   FuncMetaData("String", false, true) )
            .append( "toInt",       FuncMetaData("Int", false, true) )
            .append( "toReal",      FuncMetaData("Real", false, true) )
            .append( "toBool",      FuncMetaData("Bool", false, true) )
            .append( "isNull",      FuncMetaData("Bool", false, true) )
            ;
        metaData
            ->append( "eq",     OperMetaData("Any", "Bool" ) )
            .append( "neq",     OperMetaData("Any", "Bool" ) )
            ;
    }
    return metaData;
}

QList<BydaoBool*> BydaoBool::s_cache;

BydaoBool::BydaoBool(bool value)
    : BydaoObject()
    , m_value(value)
{
    registerMethod("toString", &BydaoBool::method_toString);
    registerMethod("toInt",    &BydaoBool::method_toInt);
    registerMethod("toReal",   &BydaoBool::method_toReal);
    registerMethod("toBool",   &BydaoBool::method_toBool);
    registerMethod("isNull",   &BydaoBool::method_isNull);
}

void BydaoBool::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoBool::callMethod(const QString& name,
                          const BydaoValueList& args,
                          BydaoValue* result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

bool BydaoBool::method_toString(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoString::create(m_value ? "true" : "false") );
    return true;
}

bool BydaoBool::method_toInt(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoInt::create(m_value ? 1 : 0) );
    return true;
}

bool BydaoBool::method_toReal(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoReal::create(m_value ? 1.0 : 0.0) );
    return true;
}

bool BydaoBool::method_toBool(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoBool::create(m_value) );
    return true;
}

bool BydaoBool::method_isNull(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoBool::create(false) );
    return true;
}

BydaoValue* BydaoBool::eq(const BydaoValue* other) {
    return BydaoValue::get( BydaoBool::create( m_value == other->toBool() ) );
}

BydaoValue* BydaoBool::neq(const BydaoValue* other) {
    return BydaoValue::get( BydaoBool::create( m_value != other->toBool() ) );
}

} // namespace BydaoScript
