#pragma once

#include "BydaoNative.h"

namespace BydaoScript {

class BydaoReal : public BydaoNative {
public:
    explicit BydaoReal(double value = 0.0);
    
    QString typeName() const override { return "Real"; }
    double value() const { return m_value; }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

private:

    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toInt(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toBool(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_abs(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_negate(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_round(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_floor(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_ceil(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoReal::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    double m_value;
};

} // namespace BydaoScript
