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
#include "BydaoScript/BydaoNull.h"
#include "BydaoScript/BydaoArray.h"

namespace BydaoScript {

BydaoValue::BydaoValue() : m_obj(nullptr), m_typeId(TYPE_UNKNOWN) {}

BydaoValue::BydaoValue(BydaoObject* obj, BydaoTypeId typeId ) : m_obj(obj) {
    if (m_obj) {
        m_obj->ref();
        m_typeId = typeId;
    }
    else {
        m_typeId = TYPE_NULL;
    }
}

BydaoValue::BydaoValue(const BydaoValue& other)
    : m_obj(other.m_obj), m_typeId(other.m_typeId) {
    if (m_obj) m_obj->ref();
}

BydaoValue::~BydaoValue() {
    if (m_obj) m_obj->unref();
}

BydaoValue BydaoValue::copy() {
    BydaoValue newVal;
    newVal.m_typeId = m_typeId;
    if ( m_obj ) {
        newVal.m_obj = m_obj->copy();
        newVal.m_obj->ref();
    }
    else {
        newVal.m_obj = nullptr;
    }
    return newVal;
}

BydaoValue& BydaoValue::operator=(const BydaoValue& other) {
    if (this != &other) {
        if (m_obj) {
            if ( m_obj->getRef() == 1 ) {
                if ( other.m_obj && m_typeId == other.m_typeId ) {
                    m_obj->assign( other.m_obj );
                    return *this;
                }
            }
            m_obj->unref();
        }
        m_obj = other.m_obj;
        m_typeId = other.m_typeId;
        if (m_obj) m_obj->ref();
    }
    return *this;
}


// Конструктор перемещения
BydaoValue::BydaoValue(BydaoValue&& other) noexcept
    : m_obj(other.m_obj)        // забираем указатель
    , m_typeId(other.m_typeId)  // забираем typeId
{
    // Обнуляем источник, чтобы его деструктор не тронул объект
    other.m_obj = nullptr;
    other.m_typeId = TYPE_UNKNOWN;
}

// Оператор перемещения
BydaoValue& BydaoValue::operator=(BydaoValue&& other) noexcept {
    if (this != &other) {
        // Освобождаем свой текущий объект (если есть)
        if (m_obj) {
            m_obj->unref();
        }
        // Забираем чужой
        m_obj = other.m_obj;
        m_typeId = other.m_typeId;
        // Обнуляем источник
        other.m_obj = nullptr;
        other.m_typeId = TYPE_UNKNOWN;
    }
    return *this;
}

// Фабричные методы
BydaoValue BydaoValue::fromInt(qint64 value) {
    return BydaoValue( BydaoInt::create(value), BydaoTypeId::TYPE_INT );
}

BydaoValue BydaoValue::fromReal(double value) {
    return BydaoValue( BydaoReal::create(value), BydaoTypeId::TYPE_REAL);
}

BydaoValue BydaoValue::fromBool(bool value) {
    return BydaoValue( BydaoBool::create(value), BydaoTypeId::TYPE_BOOL );
}

BydaoValue BydaoValue::fromString(const QString& value) {
    return BydaoValue(BydaoString::create(value), BydaoTypeId::TYPE_STRING);
}

BydaoValue BydaoValue::fromNull() {
    return BydaoValue(BydaoNull::instance(), BydaoTypeId::TYPE_NULL );
}

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
                parts << a->at(i).toString();
            }
            return "[" + parts.join(", ") + "]";
        }
        case TYPE_NULL:
            return "null";
    }

    // Для неизвестных типов пробуем вызвать метод toString
    BydaoValue result;
    if (m_obj->callMethod("toString", {}, result)) {
        if (result.isObject()) {
            // Рекурсивно вызываем toString у результата
            return result.toString();
        }
        qWarning() << "toString() returned non-object";
    }
    else {
        qWarning() << "Object of type" << m_obj->typeName() << "has no toString() method";
    }
    return "???";
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

    BydaoValue result;
    if (m_obj->callMethod("toBool", {}, result)) {
        return result.toBool();
    }
    // По умолчанию объект считается true
    return true;
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
    BydaoValue result;
    if (m_obj->callMethod("toInt", {}, result)) {
        if (result.isObject()) {
            // Рекурсивно вызываем toInt у результата
            return result.toInt();
        }
        qWarning() << "toInt() returned non-object";
    }
    else {
        qWarning() << "Object of type" << m_obj->typeName() << "cannot convert to Int";
    }
    return 0;
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
    BydaoValue result;
    if (m_obj->callMethod("toReal", {}, result)) {
        return result.toReal();
    }
    qWarning() << "Object of type" << m_obj->typeName() << "cannot convert to Real";
    return 0.0;
}

bool BydaoValue::isNull() const {
    if (!m_obj) return true;

    BydaoValue result;
    if (m_obj->callMethod("isNull", {}, result)) {
        if (result.isObject()) {
            return result.toBool();
        }
    }
    return false;
}

} // namespace BydaoScript
