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
#include "BydaoScript/BydaoNull.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"

namespace BydaoScript {

BydaoNull* BydaoNull::s_instance = nullptr;

BydaoNull* BydaoNull::instance() {
    if (!s_instance) {
        s_instance = new BydaoNull();
    }
    return s_instance;
}

// Получить мета-данные
MetaData*   BydaoNull::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData
            // методы объекта
            ->append( "toString",   FuncMetaData("String", false, true) )
            .append( "toBool",      FuncMetaData("Bool", false, true) )
            .append( "isNull",      FuncMetaData("Bool", false, true) )
            .append( "isEmpty",     FuncMetaData("Bool", false, true) )
            ;
        metaData
            ->append( "eq",     OperMetaData("Any", "Bool" ) )
            .append( "neq",     OperMetaData("Any", "Bool" ) )
            ;
    }
    return metaData;
}

//------------------------------------------------------------------------------

BydaoNull::BydaoNull() {
    // Увеличиваем счётчик, чтобы никогда не удалялся
    ref();  // теперь никогда не станет 0
    registerMethod("toString", &BydaoNull::method_toString);
    registerMethod("toBool", &BydaoNull::method_toBool);
    registerMethod("isNull", &BydaoNull::method_isNull);
    registerMethod("isEmpty", &BydaoNull::method_isNull);
}

void BydaoNull::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoNull::callMethod(const QString& name,
                           const QVector<BydaoValue>& args,
                           BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

bool BydaoNull::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoString::create("null"));
    return true;
}

bool BydaoNull::method_toBool(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoBool::create(false));
    return true;
}

bool BydaoNull::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoBool::create(true));
    return true;
}

BydaoValue BydaoNull::eq(const BydaoValue& other) {
    return BydaoValue::fromBool( other.typeId() == TYPE_NULL );
}

BydaoValue BydaoNull::neq(const BydaoValue& other) {
    return BydaoValue::fromBool( other.typeId() != TYPE_NULL );
}

} // namespace BydaoScript
