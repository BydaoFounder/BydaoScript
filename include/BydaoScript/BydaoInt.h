#pragma once

#include "BydaoNative.h"

namespace BydaoScript {

class BydaoInt : public BydaoNative {
    Q_OBJECT
    // Q_PROPERTY(int value READ value)

public:
    explicit BydaoInt(qint64 value = 0, QObject* parent = nullptr);
    virtual ~BydaoInt() override = default;

    QString typeName() const override { return "Int"; }
    qint64 value() const { return m_value; }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

    // ========== Операции ==========
    BydaoValue add(const BydaoValue& other) override;
    BydaoValue sub(const BydaoValue& other) override;
    BydaoValue mul(const BydaoValue& other) override;
    BydaoValue div(const BydaoValue& other) override;
    BydaoValue neg() override;

    BydaoValue eq(const BydaoValue& other) override;
    BydaoValue lt(const BydaoValue& other) override;
    BydaoValue le(const BydaoValue& other) override;
    BydaoValue gt(const BydaoValue& other) override;
    BydaoValue ge(const BydaoValue& other) override;

private:

    // Методы
    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toReal(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toBool(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_abs(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_negate(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toHex(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoInt::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    qint64 m_value;
};

} // namespace BydaoScript
