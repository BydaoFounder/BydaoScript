#pragma once

#include "BydaoNative.h"

namespace BydaoScript {

class BydaoReal : public BydaoNative {
    Q_OBJECT

    static QVector<BydaoReal*> s_cache;
    static const int MAX_CACHE_SIZE = 1024;

protected:

    void release() override {
        if (s_cache.size() < MAX_CACHE_SIZE) {
            m_refCount = 0;  // сбрасываем для следующего использования
            s_cache.append(this);
        } else {
            delete this;
        }
    }
    explicit BydaoReal(double value = 0.0, QObject* parent = nullptr);

public:

    static BydaoReal* create(qint64 value) {
        if (!s_cache.isEmpty()) {
            BydaoReal* obj = s_cache.takeLast();
            obj->m_value = value;
            return obj;
        }
        return new BydaoReal(value);
    }

    QString typeName() const override { return "Real"; }
    double value() const { return m_value; }

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

    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toFixed(const QVector<BydaoValue>& args, BydaoValue& result);
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
