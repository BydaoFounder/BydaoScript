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
#ifndef _BYDAOVALUE_H_
#define _BYDAOVALUE_H_

#include <stddef.h>
#include <QtCore>
#include <qhash.h>

namespace BydaoScript {

class BydaoObject;
class BydaoString;
class BydaoBool;
class BydaoInt;

// Быстрые идентификаторы типов
enum BydaoTypeId {
    TYPE_UNKNOWN = 0,
    TYPE_INT = 1,
    TYPE_REAL = 2,
    TYPE_BOOL = 3,
    TYPE_STRING = 4,
    TYPE_ARRAY = 5,
    TYPE_NULL = 6,
    TYPE_MODULE = 7,
    TYPE_OBJECT = 8,
    TYPE_FUNC = 9
};

class BydaoValue {
public:
    BydaoValue();
    BydaoValue(BydaoObject* obj, BydaoTypeId typeId );
    BydaoValue(const BydaoValue& other);
    ~BydaoValue();

    BydaoValue& operator=(const BydaoValue& other);

    // Конструктор перемещения
    BydaoValue(BydaoValue&& other) noexcept;

    // Оператор перемещения
    BydaoValue& operator=(BydaoValue&& other) noexcept;

    BydaoValue copy();

    inline bool isObject() const { return m_obj != nullptr; }
    inline BydaoObject* toObject() const { return m_obj; }

    void    set(BydaoObject* obj, BydaoTypeId typeId ) {
        m_obj = obj;
        m_typeId = typeId;
    }

    // Быстрый доступ к типу
    inline int typeId() const { return m_typeId; }

    bool        isInt() const { return m_typeId == TYPE_INT; }
    bool        isReal() const { return m_typeId == TYPE_REAL; }
    bool        isBool() const { return m_typeId == TYPE_BOOL; }
    bool        isString() const { return m_typeId == TYPE_STRING; }
    bool        isArray() const { return m_typeId == TYPE_ARRAY; }
    bool        isFunc() const { return m_typeId == TYPE_FUNC; }

    // Удобные методы
    QString toString() const;
    bool toBool() const;
    qint64 toInt() const;
    double toReal() const;
    bool isNull() const;

    // Фабричные методы для быстрого создания без new
    static BydaoValue fromInt(qint64 value);
    static BydaoValue fromReal(double value);
    static BydaoValue fromBool(bool value);
    static BydaoValue fromString(const QString& value);
    static BydaoValue fromNull();

private:

    BydaoObject*    m_obj;
    int             m_typeId;  // кэш типа
};

inline uint qHash(const BydaoValue& v, uint seed = 0) {
    if (v.isInt()) {
        return ::qHash( v.toInt(), seed);
    }
    if (v.isString()) {
        return ::qHash(v.toString(), seed);
    }
    if (v.isNull()) {
        return ::qHash(nullptr, seed);
    }
    // fallback: хэш по указателю на объект
    return ::qHash( reinterpret_cast<quintptr>(v.toObject()), seed);
}

inline bool operator==(const BydaoValue& a, const BydaoValue& b) {
    if (a.typeId() != b.typeId()) return false;
    if (a.isInt()) return a.toInt() == b.toInt();
    if (a.isString()) return a.toString() == b.toString();
    if (a.isNull()) return true;
    // fallback: сравнение по указателю или через equals()
    return a.toObject() == b.toObject();
}

} // namespace BydaoScript

#endif
