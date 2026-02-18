#include "BydaoScript/BydaoValue.h"
#include "BydaoScript/BydaoObject.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoArray.h"

namespace BydaoScript {

BydaoValue::BydaoValue() : m_obj(nullptr), m_typeId(TYPE_UNKNOWN) {}

BydaoValue::BydaoValue(BydaoObject* obj) : m_obj(obj), m_typeId(TYPE_UNKNOWN) {
    updateTypeId();
}

BydaoValue::BydaoValue(const BydaoValue& other)
    : m_obj(other.m_obj), m_typeId(other.m_typeId) {
}

BydaoValue::~BydaoValue() {}

BydaoValue& BydaoValue::operator=(const BydaoValue& other) {
    if (this != &other) {
        m_obj = other.m_obj;
        m_typeId = other.m_typeId;
    }
    return *this;
}

void BydaoValue::updateTypeId() {
    if (!m_obj) {
        m_typeId = TYPE_NULL;
        return;
    }

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

// Фабричные методы
BydaoValue BydaoValue::fromInt(int value) {
    return BydaoValue(new BydaoInt(value));
}

BydaoValue BydaoValue::fromReal(double value) {
    return BydaoValue(new BydaoReal(value));
}

BydaoValue BydaoValue::fromBool(bool value) {
    return BydaoValue(new BydaoBool(value));
}

BydaoValue BydaoValue::fromString(const QString& value) {
    return BydaoValue(new BydaoString(value));
}

// BydaoValue BydaoValue::fromArray(BydaoArray* array) {
//     return BydaoValue(array);
// }

BydaoValue BydaoValue::fromObject(BydaoObject* obj) {
    return BydaoValue(obj);
}

QString BydaoValue::toString() const {
    if (!m_obj) return "null";

    // Быстрый путь для известных типов
    switch (m_typeId) {
        case TYPE_INT: {
            const auto* i = static_cast<const BydaoInt*>(m_obj);
            return QString::number(i->value());
        }
        case TYPE_REAL: {
            const auto* r = static_cast<const BydaoReal*>(m_obj);
            return QString::number(r->value());
        }
        case TYPE_BOOL: {
            const auto* b = static_cast<const BydaoBool*>(m_obj);
            return b->value() ? "true" : "false";
        }
        case TYPE_STRING: {
            const auto* s = static_cast<const BydaoString*>(m_obj);
            return s->value();
        }
        case TYPE_ARRAY: {
            const auto* a = static_cast<const BydaoArray*>(m_obj);
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
            const auto* i = static_cast<const BydaoInt*>(m_obj);
            return i->value() != 0;
        }
        case TYPE_REAL: {
            const auto* r = static_cast<const BydaoReal*>(m_obj);
            return r->value() != 0.0;
        }
        case TYPE_BOOL: {
            const auto* b = static_cast<const BydaoBool*>(m_obj);
            return b->value();
        }
        case TYPE_STRING: {
            const auto* s = static_cast<const BydaoString*>(m_obj);
            return !s->value().isEmpty();
        }
        case TYPE_ARRAY: {
            const auto* a = static_cast<const BydaoArray*>(m_obj);
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

int BydaoValue::toInt() const {
    if (!m_obj) return 0;

    // Прямое преобразование в зависимости от типа
    switch (m_typeId) {
        case TYPE_INT: {
            const auto* i = static_cast<const BydaoInt*>(m_obj);
            return i->value();
        }
        case TYPE_REAL: {
            const auto* r = static_cast<const BydaoReal*>(m_obj);
            return (int)r->value();
        }
        case TYPE_BOOL: {
            const auto* b = static_cast<const BydaoBool*>(m_obj);
            return b->value() ? 1 : 0;
        }
        case TYPE_STRING: {
            const auto* s = static_cast<const BydaoString*>(m_obj);
            return s->value().toInt();
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
            const auto* i = static_cast<const BydaoInt*>(m_obj);
            return (double)i->value();
        }
        case TYPE_REAL: {
            const auto* r = static_cast<const BydaoReal*>(m_obj);
            return r->value();
        }
        case TYPE_BOOL: {
            const auto* b = static_cast<const BydaoBool*>(m_obj);
            return b->value() ? 1.0 : 0.0;
        }
        case TYPE_STRING: {
            const auto* s = static_cast<const BydaoString*>(m_obj);
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
            if (auto* b = dynamic_cast<BydaoBool*>(result.toObject())) {
                return b->value();
            }
        }
    }
    return false;
}

} // namespace BydaoScript
