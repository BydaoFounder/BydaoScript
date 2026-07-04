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
#pragma once

#include "BydaoIterator.h"
#include "BydaoArray.h"

namespace BydaoScript {

class BydaoArrayIterator : public BydaoIterator {

public:
    explicit BydaoArrayIterator(BydaoArray* array);
    virtual ~BydaoArrayIterator();

    // Получить мета-данные
    static MetaData*   metaData();

    QString typeName() const override { return "ArrayIter"; }

    // Реализация методов итератора
    bool next() override;
    bool isValid() const override;
    BydaoValue key() const override;
    BydaoValue value() const override;

    bool    getVar( const QString& varName, BydaoValue& value ) override;

protected:

    static bool itNext(BydaoObject* self) {
        auto* iter = static_cast<BydaoArrayIterator*>(self);
        if ( iter->m_array && iter->m_index < iter->m_array->size() ) {
            return ++iter->m_index < iter->m_array->size();
        }
        return false;
    }

    static bool itIsValid(BydaoObject* self) {
        auto* iter = static_cast<BydaoArrayIterator*>(self);
        return iter->m_array && 0 <= iter->m_index && iter->m_index < iter->m_array->size();
    }

    static BydaoValue itValue(BydaoObject* self) {
        auto* iter = static_cast<BydaoArrayIterator*>(self);
        if ( iter->m_array && iter->m_index >= 0 && iter->m_index < iter->m_array->size() ) {
            return iter->m_array->at( iter->m_index );
        }
        return BydaoValue::fromNull();
    }

    static BydaoValue itKey(BydaoObject* self) {
        auto* iter = static_cast<BydaoArrayIterator*>(self);
        if ( iter->m_array && iter->m_index >= 0 && iter->m_index < iter->m_array->size() ) {
            return BydaoValue::fromInt( iter->m_index) ;
        }
        return BydaoValue::fromNull();
    }

    // Статические методы
    static bool nextImpl(BydaoObject* self, const QVector<BydaoValue>&, BydaoValue& result) {
        result = BydaoValue::fromBool( BydaoArrayIterator::itNext( static_cast<BydaoArrayIterator*>(self) ) );
        return true;
    }

    static bool isValidImpl(BydaoObject* self, const QVector<BydaoValue>&, BydaoValue& result) {
        result = BydaoValue::fromBool( BydaoArrayIterator::itIsValid( static_cast<BydaoArrayIterator*>(self) ) );
        return true;
    }

    static bool valueImpl(BydaoObject* self, const QVector<BydaoValue>&, BydaoValue& result) {
        result = BydaoArrayIterator::itValue( static_cast<BydaoArrayIterator*>(self) );
        return true;
    }

    static bool keyImpl(BydaoObject* self, const QVector<BydaoValue>&, BydaoValue& result) {
        result = BydaoArrayIterator::itKey( static_cast<BydaoArrayIterator*>(self) );
        return true;
    }

    using GetVarPtr = bool (BydaoArrayIterator::*)(BydaoValue&);
    using SetVarPtr = bool (BydaoArrayIterator::*)(const BydaoValue&);
    struct VarMethod {
        GetVarPtr   getter;
        SetVarPtr   setter;
    };
    QHash<QString,VarMethod> m_vars;

    void registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter = nullptr );

    bool getvar_key( BydaoValue& value ) {
        value = key();
        return true;
    };

    bool getvar_value( BydaoValue& value ) {
        value = this->value();
        return true;
    };

    BydaoArray* m_array;
    int m_index;
};

} // namespace BydaoScript