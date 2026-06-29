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

#include <QString>

#include "BydaoObject.h"
#include "BydaoArray.h"
#include "BydaoArrayIterator.h"
#include "BydaoMetaData.h"

namespace BydaoScript {

class BydaoStringClass : public BydaoObject {

public:

    explicit BydaoStringClass();
    virtual ~BydaoStringClass() = default;

    // Получить мета-данные
    static MetaData*   metaData();

    // Получить список используемых мета-данных
    static UsedMetaDataList usedMetaData();

    QString typeName() const override { return "String"; }

    bool    getVar( const QString& varName, BydaoValue& value ) override;

    bool    callMethod(const QString& name, const QVector<BydaoValue>& args, BydaoValue& result) override;

private:

    // Статические методы класса String

    using MethodPtr = bool (BydaoStringClass::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;

    bool method_random(const QVector<BydaoValue>& args, BydaoValue& result);

    using GetVarPtr = bool (BydaoStringClass::*)(BydaoValue&);
    using SetVarPtr = bool (BydaoStringClass::*)(const BydaoValue&);
    struct VarMethod {
        GetVarPtr   getter;
        SetVarPtr   setter;
    };
    QHash<QString,VarMethod> m_vars;

    void registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter = nullptr );

    const int RANDOM_LETTERS = 0x0001;
    const int RANDOM_DIGITS  = 0x0002;
    const int RANDOM_SYMBOLS = 0x0004;
    const int RANDOM_HEX     = 0x0008;

    bool getvar_letters( BydaoValue& value ) {
        value = BydaoValue::fromInt( RANDOM_LETTERS );
        return true;
    };

    bool getvar_digits( BydaoValue& value ) {
        value = BydaoValue::fromInt( RANDOM_DIGITS );
        return true;
    };

    bool getvar_symbols( BydaoValue& value ) {
        value = BydaoValue::fromInt( RANDOM_SYMBOLS );
        return true;
    };

    bool getvar_hex( BydaoValue& value ) {
        value = BydaoValue::fromInt( RANDOM_HEX );
        return true;
    };

};

//==============================================================================

class BydaoString : public BydaoObject {

    static QVector<BydaoString*> s_cache;
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
    explicit BydaoString(const QString& value = QString());

public:

    static BydaoString* create( const QString& value) {
        if ( ! s_cache.isEmpty() ) {
            BydaoString* obj = s_cache.takeLast();
            obj->m_value = value;
            return obj;
        }
        return new BydaoString(value);
    }

    // Получить мета-данные
    static MetaData*   metaData();

    QString typeName() const override { return "String"; }

    bool    getVar( const QString& varName, BydaoValue& value ) override;

    bool    callMethod(const QString& name, const QVector<BydaoValue>& args, BydaoValue& result) override;

    const QString& value() const { return m_value; }

    BydaoValue iter() override;  // создаёт итератор для строки

    BydaoObject* copy() override {
        return BydaoString::create( m_value );
    }

    // Операции

    void    assign( BydaoObject* obj ) override {
        m_value = ((BydaoString*)obj)->m_value;
    }

    BydaoValue add(const BydaoValue& other) override;   // сложение (конкатенация) строк
    void       addToValue(const BydaoValue& other) override;

    BydaoValue eq(const BydaoValue& other) override;    // сравнение на равенство
    BydaoValue neq(const BydaoValue& other) override;   // сравнение на неравенство
    BydaoValue lt(const BydaoValue& other) override;    // лексикографическое сравнение
    BydaoValue le(const BydaoValue& other) override;    // лексикографическое сравнение
    BydaoValue gt(const BydaoValue& other) override;    // лексикографическое сравнение
    BydaoValue ge(const BydaoValue& other) override;    // лексикографическое сравнение

    bool lessThan(const BydaoValue& other) override {
        return m_value < other.toString();
    }

private:

    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_length(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_upper(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_lower(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_trim(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_substring(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_indexOf(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_contains(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_startsWith(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_endsWith(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_replace(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_split(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toInt(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toReal(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toBool(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isEmpty(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_md5(const QVector<BydaoValue>& args, BydaoValue& result);

    bool method_iter(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoString::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    using GetVarPtr = bool (BydaoString::*)(BydaoValue&);
    using SetVarPtr = bool (BydaoString::*)(const BydaoValue&);
    struct VarMethod {
        GetVarPtr   getter;
        SetVarPtr   setter;
    };
    QHash<QString,VarMethod> m_vars;

    void registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter = nullptr );

    bool getvar_length( BydaoValue& value ) {
        value = BydaoValue::fromInt( m_value.length() );
        return true;
    };

    QString m_value;
};

class BydaoStringArray: public BydaoArray {
public:
    explicit BydaoStringArray();
    virtual ~BydaoStringArray() override = default;

    // Получить мета-данные
    static MetaData*   metaData();

    // Получить список используемых мета-данных
    static UsedMetaDataList usedMetaData();

    QString typeName() const override { return "StringArray"; }

    BydaoValue  iter() override;  // создаёт итератор для массива

};

class BydaoStringArrayIterator : public BydaoArrayIterator {

public:
    explicit BydaoStringArrayIterator(BydaoArray* array);
    virtual ~BydaoStringArrayIterator();

    // Получить мета-данные
    static MetaData*   metaData();

    QString typeName() const override { return "StringArrayIter"; }

    // Реализация методов итератора
    //    BydaoValue value() const override;
};

} // namespace BydaoScript
