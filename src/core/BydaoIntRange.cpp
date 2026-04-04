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
#include "BydaoScript/BydaoIntRange.h"
#include "BydaoScript/BydaoInt.h"

namespace BydaoScript {

// Получить мета-данные
MetaData*   BydaoIntRange::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData
            // методы объекта
            ->append( "iter",   FuncMetaData("IntRangeIter", false, true) )
            ;
    }
    return metaData;
}

BydaoIntRange::BydaoIntRange(qint64 start, qint64 end)
    : BydaoObject()
    , m_start(start)
    , m_end(end)
{
    registerMethod("iter", &BydaoIntRange::method_iter);
}

void BydaoIntRange::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoIntRange::callMethod(const QString& name,
                             const BydaoValueList& args,
                             BydaoValue* result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

BydaoValue* BydaoIntRange::iter() {
    return BydaoValue::get( new BydaoIntRangeIterator(this) );
}

// BydaoIntRange::method_iter()
bool BydaoIntRange::method_iter(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( new BydaoIntRangeIterator(this) );
    return true;
}

//=============================================================================

// Получить мета-данные
MetaData*   BydaoIntRangeIterator::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData->extend = "Iter";
        metaData
            // методы объекта
            ->append( "next",    FuncMetaData("Bool", false, false) )
            .append( "isValid",  FuncMetaData("Bool", false, false) )
            ;
        metaData
            // переменные объекта
            ->append( "key",     VarMetaData("Int",true,false) )
            .append( "value",    VarMetaData("Int",true,false) )
            ;
    }
    return metaData;
}

BydaoIntRangeIterator::BydaoIntRangeIterator( BydaoIntRange* range ) {
    m_start = range->start();
    m_end = range->end();
    m_current = m_start - 1;
}

bool    BydaoIntRangeIterator::getVar( const QString& varName, BydaoValue* value ) {
    if ( varName == "value" ) {
        value->set( isValid() ? BydaoInt::create(m_current) : nullptr );
        return true;
    }
    if ( varName == "key" ) {
        value->set( isValid() ? BydaoInt::create(m_current - m_start) : nullptr );
        return true;
    }
    return BydaoIterator::getVar( varName, value );
}

bool    BydaoIntRangeIterator::next() {
    return ++m_current < m_end;
}

bool BydaoIntRangeIterator::isValid() const {
    return m_current >= m_start && m_current < m_end;
}

BydaoValue* BydaoIntRangeIterator::key() const {
    if (!isValid()) return BydaoValue::get();
    return BydaoValue::get( BydaoInt::create(m_current - m_start) );
}

BydaoValue* BydaoIntRangeIterator::value() const {
    if (!isValid()) return BydaoValue::get();
    return BydaoValue::get( BydaoInt::create(m_current) );
}

} // namespace BydaoScript
