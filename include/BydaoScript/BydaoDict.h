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

class BydaoDict : public BydaoObject {

public:
    explicit BydaoDict();
    virtual ~BydaoDict() override = default;

    struct Entry {
        BydaoValue  key;
        BydaoValue  value;
    };

    // Получить мета-данные
    static MetaData*   metaData();

    // Получить список используемых мета-данных
    static UsedMetaDataList usedMetaData();

    QString     typeName() const override { return "Dict"; }

    bool        getVar( const QString& varName, BydaoValue& value ) override;

    bool        callMethod(const QString& name, const QVector<BydaoValue>& args, BydaoValue& result) override;

    BydaoValue  get( const BydaoValue& key );
    void        set( const BydaoValue& key, const BydaoValue& value);
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
        BydaoDict* dict = (BydaoDict*)obj;
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

    using MethodPtr = bool (BydaoDict::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;

    using GetVarPtr = bool (BydaoDict::*)(BydaoValue&);
    using SetVarPtr = bool (BydaoDict::*)(const BydaoValue&);
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
        return (static_cast<BydaoDict*>(self))->method_sort( args, result );
    }

    static bool ksortImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        return (static_cast<BydaoDict*>(self))->method_ksort( args, result );
    }

    QVector< Entry >            m_entries;  // пары ключ-значение в порядке
    QHash< BydaoValue, qint32>   m_index;   // ключ → позиция в m_entries

    BydaoRuntime*   m_runtime;
};


class BydaoDictIterator : public BydaoIterator {

public:
    explicit BydaoDictIterator(BydaoDict* dict);
    virtual ~BydaoDictIterator();

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
        auto* iter = static_cast<BydaoDictIterator*>(self);
        return iter->next();
    }

    static bool itIsValid(BydaoObject* self) {
        auto* iter = static_cast<BydaoDictIterator*>(self);
        return iter->isValid();
    }

    static BydaoValue itValue(BydaoObject* self) {
        auto* iter = static_cast<BydaoDictIterator*>(self);
        return iter->value();
    }

    static BydaoValue itKey(BydaoObject* self) {
        auto* iter = static_cast<BydaoDictIterator*>(self);
        return iter->key();
    }

    BydaoDict*  m_dict;
    int         m_index;
};

} // namespace BydaoScript
