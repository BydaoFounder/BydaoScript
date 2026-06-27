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

namespace BydaoScript {

class BydaoArray : public BydaoObject {

public:
    explicit BydaoArray();
    virtual ~BydaoArray() override = default;

    // Получить мета-данные
    static MetaData*   metaData();

    // Получить список используемых мета-данных
    static UsedMetaDataList usedMetaData();

    QString typeName() const override { return "Array"; }

    bool    getVar( const QString& varName, BydaoValue& value ) override;

    bool callMethod(const QString& name, const QVector<BydaoValue>& args, BydaoValue& result) override;

    virtual int         size() const { return m_elements.size(); }
    virtual BydaoValue  at(qint64 index) const;
    virtual void        set(qint64 index, const BydaoValue& value);
    virtual BydaoValue  get(const BydaoValue& index);
    virtual void        append(const BydaoValue& value);
    virtual void        insert(qint64 index, const BydaoValue& value);
    virtual void        removeAt(qint64 index);
    virtual void        clear();

    BydaoValue  iter() override;  // создаёт итератор для массива

    void            setRuntime(BydaoRuntime* runtime) {
        m_runtime = runtime;
    };
    BydaoRuntime*   runtime() { return m_runtime; };

private:

    bool method_iter(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_length(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_get(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_set(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_append(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_takeLast(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_take(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_takeFirst(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_prepend(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_slice(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_glue(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_merge(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_remove(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_indexOf(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_sort(const QVector<BydaoValue>& args, BydaoValue& result);

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

    bool getvar_length( BydaoValue& value ) {
        value = BydaoValue::fromInt( m_elements.size() );
        return true;
    };

    static bool appendImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        auto* obj = static_cast<BydaoArray*>(self);
        return obj->method_append( args, result );
    }

    static bool sortImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        auto* obj = static_cast<BydaoArray*>(self);
        return obj->method_sort( args, result );
    }

    QVector<BydaoValue> m_elements;

    BydaoRuntime*   m_runtime;
};

} // namespace BydaoScript
