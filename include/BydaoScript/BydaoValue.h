#pragma once

#include <QString>

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
    BydaoValue();
    explicit BydaoValue(BydaoObject* obj);
    BydaoValue(const BydaoValue& other);
    ~BydaoValue();

    BydaoValue& operator=(const BydaoValue& other);

    bool isObject() const { return m_obj != nullptr; }
    BydaoObject* toObject() const { return m_obj; }

    // Быстрый доступ к типу
    int typeId() const { return m_typeId; }

    // Удобные методы
    QString toString() const;
    bool toBool() const;
    int toInt() const;
    double toReal() const;
    bool isNull() const;

    // Фабричные методы для быстрого создания без new
    static BydaoValue fromInt(int value);
    static BydaoValue fromReal(double value);
    static BydaoValue fromBool(bool value);
    static BydaoValue fromString(const QString& value);
//    static BydaoValue fromArray(class BydaoArray* array);
    static BydaoValue fromObject(BydaoObject* obj);

private:

    void updateTypeId();

    BydaoObject*    m_obj;
    int             m_typeId;  // кэш типа
};

} // namespace BydaoScript
