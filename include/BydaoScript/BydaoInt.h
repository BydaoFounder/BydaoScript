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

#include "BydaoObject.h"

namespace BydaoScript {

class BydaoInt : public BydaoObject {

    static QList<BydaoInt*> s_cache;
    static const int MAX_CACHE_SIZE = 1024;

protected:

    void release() override {
        if (s_cache.size() < MAX_CACHE_SIZE) {
            m_refCount = 0;  // сбрасываем для следующего использования
            s_cache.append(this);
        } else {
            delete this;
        }
    }
    explicit BydaoInt(qint64 value = 0);

public:

    static BydaoInt* create(qint64 value) {
        if (!s_cache.isEmpty()) {
            BydaoInt* obj = s_cache.takeLast();
            obj->m_value = value;
            return obj;
        }
        return new BydaoInt(value);
    }

    virtual ~BydaoInt() override = default;

    BydaoObject* copy() override {
        return BydaoInt::create( m_value );
    }

    QString typeName() const override { return "Int"; }
    qint64 value() const { return m_value; }

    bool callMethod(const QString& name,
                    const BydaoValueList& args,
                    BydaoValue* result) override;

    // ========== Операции ==========

    void    assign( BydaoObject* obj ) override {
        m_value = ((BydaoInt*)obj)->m_value;
    }
    BydaoValue* add(const BydaoValue* other) override;
    void        addToValue(const BydaoValue* other) override;
    BydaoValue* sub(const BydaoValue* other) override;
    BydaoValue* mul(const BydaoValue* other) override;
    BydaoValue* div(const BydaoValue* other) override;
    BydaoValue* mod(const BydaoValue* other) override;
    BydaoValue* neg() override;

    BydaoValue* eq(const BydaoValue* other) override;
    BydaoValue* neq(const BydaoValue* other) override;
    BydaoValue* lt(const BydaoValue* other) override;
    BydaoValue* le(const BydaoValue* other) override;
    BydaoValue* gt(const BydaoValue* other) override;
    BydaoValue* ge(const BydaoValue* other) override;

private:

    // Методы
    bool method_toString(const BydaoValueList& args, BydaoValue* result);
    bool method_toReal(const BydaoValueList& args, BydaoValue* result);
    bool method_toBool(const BydaoValueList& args, BydaoValue* result);
    bool method_abs(const BydaoValueList& args, BydaoValue* result);
    bool method_negate(const BydaoValueList& args, BydaoValue* result);
    bool method_isNull(const BydaoValueList& args, BydaoValue* result);
    bool method_toHex(const BydaoValueList& args, BydaoValue* result);
    bool method_toBin(const BydaoValueList& args, BydaoValue* result);

    using MethodPtr = bool (BydaoInt::*)(const BydaoValueList&, BydaoValue*);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    qint64 m_value;
};

} // namespace BydaoScript
