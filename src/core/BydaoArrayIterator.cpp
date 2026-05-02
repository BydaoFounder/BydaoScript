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
#include "BydaoScript/BydaoArrayIterator.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoNull.h"

namespace BydaoScript {

// Получить мета-данные
MetaData*   BydaoArrayIterator::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData
            // методы объекта
            ->append( "next",    FuncMetaData("Bool", false, false) )
            .append( "isValid",  FuncMetaData("Bool", false, false) )
            ;
        metaData
            // переменные объекта
            ->append( "key",     VarMetaData("Int",true,false) )
            .append( "value",    VarMetaData("Any",true,false) )
            ;
    }
    return metaData;
}

BydaoArrayIterator::BydaoArrayIterator(BydaoArray* array)
    : BydaoIterator()
    , m_array(array)
    , m_index(-1)
{
    if (m_array) {
        m_array->ref();
    }

    m_boolMethodTable.resize( 2 );
    m_boolMethodTable[0] = &BydaoArrayIterator::itNext;
    m_boolMethodTable[1] = &BydaoArrayIterator::itIsValid;

    m_valueMethodTable.resize( 2 );
    m_valueMethodTable[0] = &BydaoArrayIterator::itValue;
    m_valueMethodTable[1] = &BydaoArrayIterator::itKey;
}

BydaoArrayIterator::~BydaoArrayIterator() {
    if (m_array) {
        m_array->unref();
    }
}

bool BydaoArrayIterator::next() {
    if (!m_array) return false;
    
    m_index++;
    return m_index < m_array->size();
}

bool BydaoArrayIterator::isValid() const {
    return m_array && m_index >= 0 && m_index < m_array->size();
}

BydaoValue BydaoArrayIterator::key() const {
    if (!isValid()) return BydaoValue::fromNull();
    return BydaoValue::fromInt(m_index);
}

BydaoValue BydaoArrayIterator::value() const {
    if ( ! isValid() ) {
        return BydaoValue::fromNull();
    }
    return m_array->at(m_index);
}

} // namespace BydaoScript
