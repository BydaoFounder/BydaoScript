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

#include <QVector>

#include "BydaoObject.h"
#include "BydaoRuntime.h"
#include "BydaoMetaData.h"
#include "BydaoIterator.h"

namespace BydaoScript {

class BydaoArray : public BydaoObject {

public:
    explicit BydaoArray();
    virtual ~BydaoArray() override = default;

    enum ArrayType {
        ARR_EMPTY     = 0,    // неизвестный
        ARR_INDEXED     = 1,    // индексный
        ARR_ASSOCIATIVE = 2     // ассоциативный
    };

    struct Entry {
        BydaoValue  key;
        BydaoValue  value;
    };

    // Получить мета-данные
    static MetaData*   metaData();

    // Получить список используемых мета-данных
    static UsedMetaDataList usedMetaData();

    QString     typeName() const override { return "Array"; }

    bool        getVar( const QString& varName, BydaoValue& value ) override;

    bool        callMethod(const QString& name, const QVector<BydaoValue>& args, BydaoValue& result) override;

    ArrayType   arrayType() const {
        if ( m_index.empty() ) {
            if ( ! m_entries.empty() ) {
                return ARR_INDEXED;
            }
            return ARR_EMPTY;
        }
        if ( ! m_entries.empty() ) {
            return ARR_ASSOCIATIVE;
        }
        return ARR_EMPTY;
    }

    int         size() const {
        return m_entries.size();
    }

    BydaoValue  at(qint64 index) const;

    bool        append(const BydaoValue& value);

    BydaoValue  get( const BydaoValue& key );

    bool        set( const BydaoValue& key, const BydaoValue& value);

    // Для rvalue (временные объекты) — перемещаем
    bool        set(BydaoValue&& key, BydaoValue&& value);

    int         size() {
        return m_entries.size();
    }

    BydaoValue  key( int index ) {
        if ( 0 <= index && index < m_entries.size() ) {
            return m_entries[ index ].key;
        }
        return BydaoValue();
    }

    BydaoValue  value( int index ) {
        if ( 0 <= index && index < m_entries.size() ) {
            return m_entries[ index ].value;
        }
        return BydaoValue();
    }

    BydaoValue  iter() override;  // создаёт итератор для словаря

    void        setRuntime(BydaoRuntime* runtime) {
        m_runtime = runtime;
    };
    BydaoRuntime*   runtime() { return m_runtime; };

    // Операции
    void    assign( BydaoObject* obj ) override {
        BydaoArray* dict = (BydaoArray*)obj;
        m_entries = dict->m_entries;
        m_index   = dict->m_index;
    }

private:

    bool method_iter(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_get(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_set(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_ksort(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_sort(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_keys(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_size(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_slice(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_indexOf(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_append(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_merge(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoArray::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;

    using GetVarPtr = bool (BydaoArray::*)(BydaoValue&);
    using SetVarPtr = bool (BydaoArray::*)(const BydaoValue&);
    struct VarMethod {
        GetVarPtr   getter;
        SetVarPtr   setter;
    };
    QHash<QString,VarMethod> m_vars;

    void registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter = nullptr );

    bool getvar_size( BydaoValue& value ) {
        value = BydaoValue::fromInt( m_entries.size() );
        return true;
    };

    static bool sortImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        return (static_cast<BydaoArray*>(self))->method_sort( args, result );
    }

    static bool ksortImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        return (static_cast<BydaoArray*>(self))->method_ksort( args, result );
    }

    static bool sizeImpl( BydaoObject* self, const QVector<BydaoValue>&, BydaoValue& result ) {
        result = BydaoValue::fromInt( (static_cast<BydaoArray*>(self))->m_entries.size() );
        return true;
    }

    QVector< Entry >            m_entries;  // пары ключ-значение в порядке
    QHash< BydaoValue, qint32>   m_index;   // ключ → позиция в m_entries

    BydaoRuntime*   m_runtime;
};

//==============================================================================


class BydaoArrayIterator : public BydaoIterator {

public:
    explicit BydaoArrayIterator(BydaoArray* arr);
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
        return static_cast<BydaoArrayIterator*>(self)->next();
    }

    static bool itIsValid(BydaoObject* self) {
        return static_cast<BydaoArrayIterator*>(self)->isValid();
    }

    static BydaoValue itValue(BydaoObject* self) {
        return static_cast<BydaoArrayIterator*>(self)->value();
    }

    static BydaoValue itKey(BydaoObject* self) {
        return static_cast<BydaoArrayIterator*>(self)->key();
    }

    // Статические методы
    static bool nextImpl(BydaoObject* self, const QVector<BydaoValue>&, BydaoValue& result) {
        result = BydaoValue::fromBool( static_cast<BydaoArrayIterator*>(self)->next() );
        return true;
    }

    static bool isValidImpl(BydaoObject* self, const QVector<BydaoValue>&, BydaoValue& result) {
        result = BydaoValue::fromBool( static_cast<BydaoArrayIterator*>(self)->isValid() );
        return true;
    }

    static bool valueImpl(BydaoObject* self, const QVector<BydaoValue>&, BydaoValue& result) {
        result = static_cast<BydaoArrayIterator*>(self)->value();
        return true;
    }

    static bool keyImpl(BydaoObject* self, const QVector<BydaoValue>&, BydaoValue& result) {
        result = static_cast<BydaoArrayIterator*>(self)->key();
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

    BydaoArray*  m_arr;
    int         m_index;
};

} // namespace BydaoScript
