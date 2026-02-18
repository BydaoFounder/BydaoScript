#pragma once

#include "BydaoNative.h"

namespace BydaoScript {

class BydaoInt : public BydaoNative {
public:
    explicit BydaoInt(int value = 0);
    
    QString typeName() const override { return "Int"; }
    int value() const { return m_value; }

private:

    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toReal(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toBool(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_abs(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_negate(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toHex(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toBin(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isEven(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isOdd(const QVector<BydaoValue>& args, BydaoValue& result);
    
    int m_value;
};

} // namespace BydaoScript
