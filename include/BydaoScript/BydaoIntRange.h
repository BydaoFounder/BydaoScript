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
#include "BydaoMetaData.h"
#include "BydaoNull.h"
#include "BydaoInt.h"

namespace BydaoScript {

class BydaoIntRange : public BydaoObject {

public:

    // Получить мета-данные
    static MetaData*   metaData();

    BydaoIntRange(qint64 start, qint64 end);
    virtual ~BydaoIntRange() = default;

    QString typeName() const override { return "IntRange"; }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

    // Метод, возвращающий итератор
    BydaoValue iter() override;

    qint64 start() { return m_start; }
    qint64 end() { return m_end; }

private:

    bool method_iter(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoIntRange::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    qint64 m_start;
    qint64 m_end;
};

class BydaoIntRangeIterator : public BydaoIterator {

public:

    // Получить мета-данные
    static MetaData*   metaData();

    BydaoIntRangeIterator( BydaoIntRange* range );

    bool next() override;
    bool isValid() const override;
    BydaoValue key() const override;
    BydaoValue value() const override;

    bool    getVar( const QString& varName, BydaoValue& value ) override {
        if ( varName == "value" ) {
            value = this->value();
            return true;
        }
        if ( varName == "key" ) {
            value = this->key();
            return true;
        }
        return BydaoIterator::getVar( varName, value );
    };

private:

    static bool itNext(BydaoObject* self) {
        auto* iter = static_cast<BydaoIntRangeIterator*>(self);
        return ( ++iter->m_current < iter->m_end );
    }

    static bool itIsValid(BydaoObject* self) {
        auto* iter = static_cast<BydaoIntRangeIterator*>(self);
        return ( iter->m_current >= iter->m_start && iter->m_current < iter->m_end );
    }

    static BydaoValue itValue(BydaoObject* self) {
        auto* iter = static_cast<BydaoIntRangeIterator*>(self);
        if ( iter->m_current >= iter->m_start && iter->m_current < iter->m_end ) {
            return BydaoValue( BydaoInt::create( iter->m_current) );
        }
        return BydaoValue( BydaoNull::instance() );
    }

    static BydaoValue itKey(BydaoObject* self) {
        auto* iter = static_cast<BydaoIntRangeIterator*>(self);
        if ( iter->m_current >= iter->m_start && iter->m_current < iter->m_end ) {
            return BydaoValue( BydaoInt::create( iter->m_current - iter->m_start ) );
        }
        return BydaoValue( BydaoNull::instance() );
    }

    // Статические методы
    static bool nextImpl(BydaoObject* self,
                         const QVector<BydaoValue>&,
                         BydaoValue& result) {
        Q_UNUSED( result );
        auto* iter = static_cast<BydaoIntRangeIterator*>(self);
        result = BydaoValue::fromBool( ++iter->m_current < iter->m_end );
        return true;
    }

    static bool isValidImpl(BydaoObject* self,
                            const QVector<BydaoValue>&,
                            BydaoValue& result) {
        auto* iter = static_cast<BydaoIntRangeIterator*>(self);
        result = BydaoValue::fromBool( iter->m_current >= iter->m_start && iter->m_current < iter->m_end );
        return true;
    }

    static bool valueImpl(BydaoObject* self,
                          const QVector<BydaoValue>&,
                          BydaoValue& result) {
        auto* iter = static_cast<BydaoIntRangeIterator*>(self);

        if ( iter->m_current >= iter->m_start && iter->m_current < iter->m_end ) {
            result = BydaoValue( BydaoInt::create( iter->m_current) );
        }
        else {
            result = BydaoValue(BydaoNull::instance());
        }
        return true;
    }

    static bool keyImpl(BydaoObject* self,
                        const QVector<BydaoValue>&,
                        BydaoValue& result) {
        auto* iter = static_cast<BydaoIntRangeIterator*>(self);

        if ( iter->m_current >= iter->m_start && iter->m_current < iter->m_end ) {
            result = BydaoValue( BydaoInt::create( iter->m_current - iter->m_start ) );
        }
        else {
            result = BydaoValue(BydaoNull::instance());
        }
        return true;
    }

    qint64 m_start;
    qint64 m_end;
    qint64 m_current;
};

} // namespace BydaoScript
