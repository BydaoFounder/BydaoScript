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
#include "BydaoMetaData.h"

namespace BydaoScript {

class BydaoInt : public BydaoObject {

    static const int MAX_CACHE_SIZE = 1024;
    static QList<BydaoInt*> s_cache;
    static bool s_initCache;

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
        if ( s_initCache ) {
            s_cache.reserve( MAX_CACHE_SIZE );
            s_initCache = false;
        }
        return new BydaoInt(value);
    }

    virtual ~BydaoInt() override = default;

    // Получить мета-данные
    static MetaData*   metaData();

    BydaoObject* copy() override {
        return BydaoInt::create( m_value );
    }

    QString typeName() const override { return "Int"; }
    qint64 value() const { return m_value; }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

    // ========== Операции ==========

    void    assign( BydaoObject* obj ) override {
        m_value = ((BydaoInt*)obj)->m_value;
    }
    BydaoValue bitAnd(const BydaoValue& other) override;
    BydaoValue bitOr(const BydaoValue& other) override;
    BydaoValue bitXor(const BydaoValue& other) override;
    BydaoValue add(const BydaoValue& other) override;
    void       addToValue(const BydaoValue& other) override;
    BydaoValue sub(const BydaoValue& other) override;
    BydaoValue mul(const BydaoValue& other) override;
    BydaoValue div(const BydaoValue& other) override;
    BydaoValue mod(const BydaoValue& other) override;
    BydaoValue neg() override;

    BydaoValue eq(const BydaoValue& other) override;
    BydaoValue neq(const BydaoValue& other) override;
    BydaoValue lt(const BydaoValue& other) override;
    BydaoValue le(const BydaoValue& other) override;
    BydaoValue gt(const BydaoValue& other) override;
    BydaoValue ge(const BydaoValue& other) override;

private:

    // Методы
    bool method_abs(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_negate(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toHex(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toBin(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toReal(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toBool(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoInt::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    static bool absImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        auto* obj = static_cast<BydaoInt*>(self);
        return obj->method_abs( args, result );
    }

    static bool negImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        auto* obj = static_cast<BydaoInt*>(self);
        return obj->method_negate( args, result );
    }

    static bool toStringImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        auto* obj = static_cast<BydaoInt*>(self);
        return obj->method_toString( args, result );
    }

    static bool toHexImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        auto* obj = static_cast<BydaoInt*>(self);
        return obj->method_toHex( args, result );
    }

    static bool toBinImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        auto* obj = static_cast<BydaoInt*>(self);
        return obj->method_toBin( args, result );
    }

    static bool toRealImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        auto* obj = static_cast<BydaoInt*>(self);
        return obj->method_toReal( args, result );
    }

    static bool toBoolImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        auto* obj = static_cast<BydaoInt*>(self);
        return obj->method_toBool( args, result );
    }

    static bool isNullImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        auto* obj = static_cast<BydaoInt*>(self);
        return obj->method_isNull( args, result );
    }

    QHash<QString, MethodPtr> m_methods;  // таблица методов, вызываемых по имени

    qint64 m_value;
};

} // namespace BydaoScript
