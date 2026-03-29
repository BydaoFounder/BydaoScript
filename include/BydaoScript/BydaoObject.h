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
#ifndef _BYDAOOBJECT_H_
#define _BYDAOOBJECT_H_

#include <QString>
#include <QVector>
#include <QDebug>

#include "BydaoValue.h"

namespace BydaoScript {

class BydaoObject {

public:
    virtual ~BydaoObject() = default;

    inline void ref() { m_refCount++; }
    inline void unref() { if (--m_refCount == 0) release(); }
    inline int getRef() { return m_refCount; }

    // Единственный метод вызова
    virtual bool callMethod(const QString& name,
                            const QVector<BydaoValue>& args,
                            BydaoValue& result) = 0;

    virtual bool    getVar( const QString& varName, BydaoValue& value ) {
        Q_UNUSED( varName );
        Q_UNUSED( value );
        qWarning() << "'getVar' not implemented for" << typeName();
        return false;
    };

    virtual bool    setVar( const QString& varName, const BydaoValue& value ) {
        Q_UNUSED( varName );
        Q_UNUSED( value );
        qWarning() << "'setVar' not implemented for" << typeName();
        return false;
    };

    // Информация о типе
    virtual QString typeName() const = 0;

    // Получить итератор
    virtual BydaoValue iter(){
        return BydaoValue();
    };

    virtual BydaoObject* copy() {
        qWarning() << "copy not implemented for" << typeName();
        return nullptr;
    }

    // ========== Операции (с реализацией по умолчанию) ==========

    virtual void    assign( BydaoObject* obj ) {
        Q_UNUSED( obj );
        qWarning() << "assign not implemented for" << typeName();
    }

    virtual BydaoValue add(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "add not supported for" << typeName();
        return BydaoValue();  // null
    }

    virtual void  addToValue(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "addToValue not supported for" << typeName();
    }

    virtual BydaoValue sub(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "sub not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue mul(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "mul not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue div(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "div not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue mod(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "mod not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue neg() {
        qWarning() << "neg not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue eq(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "eq not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue neq(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "neq not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue lt(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "lt not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue le(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "le not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue gt(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "gt not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue ge(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "ge not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue and_(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "and not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue or_(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "or not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue not_() {
        qWarning() << "not not supported for" << typeName();
        return BydaoValue();
    }

protected:

    std::atomic<int> m_refCount{0};

    virtual void release() {
        delete this;
    }
};

} // namespace BydaoScript

#endif
