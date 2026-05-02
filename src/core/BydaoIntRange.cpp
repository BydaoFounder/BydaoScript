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
                             const QVector<BydaoValue>& args,
                             BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

BydaoValue BydaoIntRange::iter() {
    return BydaoValue( new BydaoIntRangeIterator(this), BydaoTypeId::TYPE_OBJECT );
}

// BydaoIntRange::method_iter()
bool BydaoIntRange::method_iter(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = iter();
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
            // стандартные методы объекта
            ->append( "next",    FuncMetaData( 0, "Bool", false, false) )
            .append( "value",    FuncMetaData( 1, "Int", false, false) )
            .append( "key",      FuncMetaData( 2, "Int", false, false) )
            .append( "isValid",  FuncMetaData( 3, "Bool", false, false) )
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

    m_stdMethodTable.resize(4);
    m_stdMethodTable[0] = &BydaoIntRangeIterator::nextImpl;
    m_stdMethodTable[1] = &BydaoIntRangeIterator::valueImpl;
    m_stdMethodTable[2] = &BydaoIntRangeIterator::keyImpl;
    m_stdMethodTable[3] = &BydaoIntRangeIterator::isValidImpl;

    m_boolMethodTable.resize( 2 );
    m_boolMethodTable[0] = &BydaoIntRangeIterator::itNext;
    m_boolMethodTable[1] = &BydaoIntRangeIterator::itIsValid;

    m_valueMethodTable.resize( 2 );
    m_valueMethodTable[0] = &BydaoIntRangeIterator::itValue;
    m_valueMethodTable[1] = &BydaoIntRangeIterator::itKey;
}

bool BydaoIntRangeIterator::next() {
    return ++m_current < m_end;
}

bool BydaoIntRangeIterator::isValid() const {
    return m_current >= m_start && m_current < m_end;
}

BydaoValue BydaoIntRangeIterator::key() const {
    if (!isValid()) return BydaoValue::fromNull();
    return BydaoValue( BydaoInt::create(m_current - m_start), BydaoTypeId::TYPE_INT );
}

BydaoValue BydaoIntRangeIterator::value() const {
    if (!isValid()) return BydaoValue::fromNull();
    return BydaoValue( BydaoInt::create(m_current), BydaoTypeId::TYPE_INT );
}

} // namespace BydaoScript
