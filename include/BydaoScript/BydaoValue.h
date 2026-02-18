#pragma once

#include <QString>

namespace BydaoScript {

class BydaoObject;
class BydaoString;
class BydaoBool;
class BydaoInt;

class BydaoValue {
public:
    BydaoValue();
    explicit BydaoValue(BydaoObject* obj);
    BydaoValue(const BydaoValue& other);
    ~BydaoValue();

    BydaoValue& operator=(const BydaoValue& other);

    bool isObject() const { return m_obj != nullptr; }
    BydaoObject* toObject() const { return m_obj; }

    // Удобные методы
    QString toString() const;
    bool toBool() const;
    int toInt() const;        // +++ ДОБАВИТЬ +++
    double toReal() const;    // +++ ДОБАВИТЬ +++
    bool isNull() const;

private:
    BydaoObject* m_obj;
};

} // namespace BydaoScript
