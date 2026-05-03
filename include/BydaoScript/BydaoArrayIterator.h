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

protected:

    static bool itNext(BydaoObject* self) {
        auto* iter = static_cast<BydaoArrayIterator*>(self);
        if ( ! iter->m_array ) return false;
        return ++iter->m_index < iter->m_array->size();
    }

    static bool itIsValid(BydaoObject* self) {
        auto* iter = static_cast<BydaoArrayIterator*>(self);
        return iter->m_array && iter->m_index >= 0 && iter->m_index < iter->m_array->size();
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

    BydaoArray* m_array;
    int m_index;
};

} // namespace BydaoScript