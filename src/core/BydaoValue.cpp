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
#include "BydaoScript/BydaoValue.h"
#include "BydaoScript/BydaoObject.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoArray.h"

namespace BydaoScript {

#define CACHE_MAX_SIZE      1024

/** Список используемых значений */
QList< BydaoValue* > BydaoValue::s_used;

/** Список свободных значений */
QList< BydaoValue* > BydaoValue::s_free;

BydaoValue* BydaoValue::get() {
    BydaoValue* val = ( s_free.size() > 0 ) ? s_free.takeLast() : new BydaoValue();
    return val;
}

BydaoValue*  BydaoValue::get( BydaoValue* other ) {
    BydaoValue* val = ( s_free.size() > 0 ) ? s_free.takeLast() : new BydaoValue();
    if ( other ) {
        val->m_typeId = other->m_typeId;
        val->m_obj = other->m_obj;
        if ( val->m_obj ) {
            val->m_obj->ref();
        }
    }
    return val;
}

BydaoValue*  BydaoValue::get( BydaoObject* obj ) {
    BydaoValue* val = ( s_free.size() > 0 ) ? s_free.takeLast() : new BydaoValue();
    val->set( obj );
    return val;
}

void         BydaoValue::free( BydaoValue* val ) {
    if ( val ) {
        if ( val->m_obj ) {
            val->m_obj->unref();
            val->m_obj = nullptr;
        }
        val->m_typeId = TYPE_UNKNOWN;

        if ( s_free.size() < CACHE_MAX_SIZE ) {
            s_free.append( val );
        }
        else {
            delete val;
        }
    }
}

void        BydaoValue::set( BydaoObject* obj ) {
    if ( m_obj ) {
        m_obj->unref();
    }
    m_obj = obj;
    if ( m_obj ) {
        m_obj->ref();
        QString typeName = m_obj->typeName();
        if (typeName == "Int")                m_typeId = TYPE_INT;
        else if (typeName == "Real")          m_typeId = TYPE_REAL;
        else if (typeName == "Bool")          m_typeId = TYPE_BOOL;
        else if (typeName == "String")        m_typeId = TYPE_STRING;
        else if (typeName == "Array")         m_typeId = TYPE_ARRAY;
        else if (typeName == "Null")          m_typeId = TYPE_NULL;
        else if (typeName.endsWith("Module")) m_typeId = TYPE_MODULE;
        else                                  m_typeId = TYPE_OBJECT;
    }
}

void        BydaoValue::shutdown() {
    s_free.clear();
    s_used.clear();
}

BydaoValue* BydaoValue::copy() {
    BydaoValue* newVal = BydaoValue::get();
    newVal->m_typeId = m_typeId;
    if ( m_obj ) {
        newVal->m_obj = m_obj->copy();
        newVal->m_obj->ref();
    }
    return newVal;
}

//==============================================================================

BydaoValue::BydaoValue() : m_obj(nullptr), m_typeId(TYPE_UNKNOWN) {}

BydaoValue::BydaoValue(BydaoObject* obj) : m_obj(obj) {
    if (m_obj) {
        m_obj->ref();
        QString typeName = m_obj->typeName();
        if (typeName == "Int") m_typeId = TYPE_INT;
        else if (typeName == "Real") m_typeId = TYPE_REAL;
        else if (typeName == "Bool") m_typeId = TYPE_BOOL;
        else if (typeName == "String") m_typeId = TYPE_STRING;
        else if (typeName == "Array") m_typeId = TYPE_ARRAY;
        else if (typeName == "Null") m_typeId = TYPE_NULL;
        else if (typeName.endsWith("Module")) m_typeId = TYPE_MODULE;
        else m_typeId = TYPE_OBJECT;
    }
    else {
        m_typeId = TYPE_NULL;
    }
}

BydaoValue::BydaoValue(const BydaoValue& other)
    : m_obj(other.m_obj), m_typeId(other.m_typeId) {
    if (m_obj) m_obj->ref();
}

BydaoValue::BydaoValue(BydaoValue&& other) noexcept
    : m_obj(other.m_obj), m_typeId(other.m_typeId) {
    other.m_obj = nullptr;
}

BydaoValue::~BydaoValue() {
    if (m_obj) m_obj->unref();
}

// BydaoValue& BydaoValue::operator=(const BydaoValue& other) {
//     if (this != &other) {
//         if (m_obj) {
//             if ( m_obj->getRef() == 1 ) {
//                 if ( other.m_obj && m_typeId == other.m_typeId ) {
//                     m_obj->assign( other.m_obj );
//                     return *this;
//                 }
//             }
//             m_obj->unref();
//         }
//         m_obj = other.m_obj;
//         m_typeId = other.m_typeId;
//         if (m_obj) m_obj->ref();
//     }
//     return *this;
// }

// BydaoValue& BydaoValue::operator=(BydaoValue&& other) noexcept {
//     if (this != &other) {
//         if (m_obj) {
//             if ( m_obj->getRef() == 1 ) {
//                 if ( other.m_obj && m_typeId == other.m_typeId ) {
//                     m_obj->assign( other.m_obj );
//                     return *this;
//                 }
//             }
//             m_obj->unref();
//         }
//         m_obj = other.m_obj;
//         m_typeId = other.m_typeId;
//         other.m_obj = nullptr;
//     }
//     return *this;
// }

// BydaoValue BydaoValue::fromArray(BydaoArray* array) {
//     return BydaoValue(array);
// }

QString BydaoValue::toString() const {
    if (!m_obj) return "null";

    // Быстрый путь для известных типов
    switch (m_typeId) {
        case TYPE_INT: {
            const auto* i = (const BydaoInt*)(m_obj);
            return QString::number(i->value());
        }
        case TYPE_REAL: {
            const auto* r = (const BydaoReal*)(m_obj);
            return QString::number(r->value());
        }
        case TYPE_BOOL: {
            const auto* b = (const BydaoBool*)(m_obj);
            return b->value() ? "true" : "false";
        }
        case TYPE_STRING: {
            const auto* s = (const BydaoString*)(m_obj);
            return s->value();
        }
        case TYPE_ARRAY: {
            const auto* a = (const BydaoArray*)(m_obj);
            // Можно сделать красивое представление
            QStringList parts;
            for (int i = 0; i < a->size(); i++) {
                parts << a->at(i)->toString();
            }
            return "[" + parts.join(", ") + "]";
        }
        case TYPE_NULL:
            return "null";
    }

    // Для неизвестных типов пробуем вызвать метод toString

    QString str = "???";
    BydaoValue* result = BydaoValue::get();
    if (m_obj->callMethod("toString", {}, result)) {
        if ( result ->isObject() ) {
            // Рекурсивно вызываем toString у результата
            str = result->toString();
        }
        else {
            qWarning() << "toString() returned non-object";
        }
    }
    else {
        qWarning() << "Object of type" << m_obj->typeName() << "has no toString() method";
    }
    BydaoValue::free( result );
    return str;
}

bool BydaoValue::toBool() const {
    if (!m_obj) return false;

    switch (m_typeId) {
        case TYPE_INT: {
            const auto* i = (const BydaoInt*)(m_obj);
            return i->value() != 0;
        }
        case TYPE_REAL: {
            const auto* r = (const BydaoReal*)(m_obj);
            return r->value() != 0.0;
        }
        case TYPE_BOOL: {
            const auto* b = (const BydaoBool*)(m_obj);
            return b->value();
        }
        case TYPE_STRING: {
            const auto* s = (const BydaoString*)(m_obj);
            return !s->value().isEmpty();
        }
        case TYPE_ARRAY: {
            const auto* a = (const BydaoArray*)(m_obj);
            return a->size() > 0;
        }
        case TYPE_NULL:
            return false;
    }

    bool res = true;    // по умолчанию объект считаем true

    BydaoValue* result = BydaoValue::get();
    if ( m_obj->callMethod("toBool", {}, result) ) {
        res = result->toBool();
    }
    BydaoValue::free( result );
    return res;
}

qint64 BydaoValue::toInt() const {
    if (!m_obj) return 0;

    // Прямое преобразование в зависимости от типа
    switch (m_typeId) {
        case TYPE_INT: {
            const auto* i = (const BydaoInt*)(m_obj);
            return i->value();
        }
        case TYPE_REAL: {
            const auto* r = (const BydaoReal*)(m_obj);
            return (qint64)r->value();
        }
        case TYPE_BOOL: {
            const auto* b = (const BydaoBool*)(m_obj);
            return b->value() ? 1 : 0;
        }
        case TYPE_STRING: {
            const auto* s = (const BydaoString*)(m_obj);
            return s->value().toLongLong();
        }
    }

    // Для неизвестных типов пробуем вызвать метод

    qint64 res = 0;
    BydaoValue* result = BydaoValue::get();
    if (m_obj->callMethod("toInt", {}, result)) {
        if ( result->isObject() ) {
            // Рекурсивно вызываем toInt у результата
            res = result->toInt();
        }
        else {
            qWarning() << "toInt() returned non-object";
        }
    }
    else {
        qWarning() << "Object of type" << m_obj->typeName() << "cannot convert to Int";
    }
    BydaoValue::free( result );
    return res;
}

double BydaoValue::toReal() const {
    if (!m_obj) return 0.0;

    switch (m_typeId) {
        case TYPE_INT: {
            const auto* i = (const BydaoInt*)(m_obj);
            return (double)i->value();
        }
        case TYPE_REAL: {
            const auto* r = (const BydaoReal*)(m_obj);
            return r->value();
        }
        case TYPE_BOOL: {
            const auto* b = (const BydaoBool*)(m_obj);
            return b->value() ? 1.0 : 0.0;
        }
        case TYPE_STRING: {
            const auto* s = (const BydaoString*)(m_obj);
            return s->value().toDouble();
        }
    }

    double res = 0.0;
    BydaoValue* result = BydaoValue::get();
    if ( m_obj->callMethod("toReal", {}, result) ) {
        res = result->toReal();
    }
    else {
        qWarning() << "Object of type" << m_obj->typeName() << "cannot convert to Real";
    }
    BydaoValue::free( result );
    return res;
}

bool BydaoValue::isNull() const {
    if (!m_obj) return true;

    bool res = false;
    BydaoValue* result = BydaoValue::get();
    if ( m_obj->callMethod("isNull", {}, result) ) {
        if ( result->isObject() ) {
            res = result->toBool();
        }
    }
    BydaoValue::free( result );
    return res;
}

} // namespace BydaoScript
