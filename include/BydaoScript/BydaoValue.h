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

#include <QString>
#include <QList>

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
    TYPE_OBJECT = 8
};

class BydaoValue {
public:

    static BydaoValue*  get();
    static BydaoValue*  get( BydaoValue* other );
    static BydaoValue*  get( BydaoObject* obj );

    static void         free( BydaoValue* val );

    static void         shutdown();

    BydaoValue*         copy();
    void                set( BydaoObject* obj );

    inline bool         isObject() const { return m_obj != nullptr; }
    inline BydaoObject* toObject() const { return m_obj; }

    // Быстрый доступ к типу
    inline int          typeId() const { return m_typeId; }

    // Удобные методы
    QString     toString() const;
    bool        toBool() const;
    qint64      toInt() const;
    double      toReal() const;
    bool        isNull() const;

private:

    BydaoValue();
    explicit BydaoValue(BydaoObject* obj);
    BydaoValue(const BydaoValue& other);
    BydaoValue(BydaoValue&& other) noexcept;
    ~BydaoValue();

    // BydaoValue& operator=(const BydaoValue& other);
    // BydaoValue& operator=(BydaoValue&& other) noexcept;

    BydaoObject*    m_obj;
    int             m_typeId;  // кэш типа

    static QList< BydaoValue* > s_free;
    static QList< BydaoValue* > s_used;
};

typedef QList< BydaoValue* >    BydaoValueList;

} // namespace BydaoScript

#endif
