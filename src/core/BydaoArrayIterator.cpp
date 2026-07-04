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
            ->appendObj( "next",    FuncMetaData( 0, "Bool", FMD_ALTERABLE) )
            .appendObj( "value",    FuncMetaData( 1, "Int",  FMD_IMMUTABLE) )
            .appendObj( "key",      FuncMetaData( 2, "Int",  FMD_IMMUTABLE) )
            .appendObj( "isValid",  FuncMetaData( 3, "Bool", FMD_IMMUTABLE) )
            ;
        metaData
            // переменные объекта
            ->appendObj( "key",     VarMetaData("Int", VMD_CONST) )
            .appendObj( "value",    VarMetaData("Any", VMD_CONST) )
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

    m_stdMethodTable.resize(4);
    m_stdMethodTable[0] = &BydaoArrayIterator::nextImpl;
    m_stdMethodTable[1] = &BydaoArrayIterator::valueImpl;
    m_stdMethodTable[2] = &BydaoArrayIterator::keyImpl;
    m_stdMethodTable[3] = &BydaoArrayIterator::isValidImpl;

    m_boolMethodTable.resize( 2 );
    m_boolMethodTable[0] = &BydaoArrayIterator::itNext;
    m_boolMethodTable[1] = &BydaoArrayIterator::itIsValid;

    m_valueMethodTable.resize( 2 );
    m_valueMethodTable[0] = &BydaoArrayIterator::itValue;
    m_valueMethodTable[1] = &BydaoArrayIterator::itKey;

    registerVar("key",   &BydaoArrayIterator::getvar_key );
    registerVar("value", &BydaoArrayIterator::getvar_value );
}

BydaoArrayIterator::~BydaoArrayIterator() {
    if (m_array) {
        m_array->unref();
    }
}

void BydaoArrayIterator::registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter ) {
    m_vars[ name ] = { getter, setter };
}

bool    BydaoArrayIterator::getVar( const QString& varName, BydaoValue& value ) {
    auto it = m_vars.find( varName );
    if ( it == m_vars.end() ) {
        return BydaoIterator::getVar( varName, value );
    }
    GetVarPtr getter = it.value().getter;
    return ( this->*( getter) )( value );
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
