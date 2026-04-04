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
#include "BydaoScript/BydaoIterator.h"
#include "BydaoScript/BydaoBool.h"

namespace BydaoScript {

BydaoIterator::BydaoIterator()
    : BydaoObject()
{
    registerMethod("next", &BydaoIterator::method_next);
    registerMethod("isValid", &BydaoIterator::method_isValid);
    registerMethod("key", &BydaoIterator::method_key);
    registerMethod("value", &BydaoIterator::method_value);

    // Регистрируем свойства для доступа через точку
    // registerProperty("key",
    //                  [this]() { return this->getKey(); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));

    // registerProperty("value",
    //                  [this]() { return this->getValue(); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));
}

BydaoIterator::~BydaoIterator() {}

void BydaoIterator::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoIterator::callMethod(const QString& name,
                               const BydaoValueList& args,
                               BydaoValue* result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

bool BydaoIterator::method_next(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoBool::create(next()) );
    return true;
}

bool BydaoIterator::method_isValid(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoBool::create(isValid()) );
    return true;
}

bool BydaoIterator::method_key(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( key()->toObject() );
    return true;
}

bool BydaoIterator::method_value(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( value()->toObject() );
    return true;
}

} // namespace BydaoScript