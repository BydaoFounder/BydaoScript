#pragma once

#include "BydaoNative.h"

namespace BydaoScript {

class BydaoBool : public BydaoNative {
    Q_OBJECT

    static QVector<BydaoBool*> s_cache;
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

    explicit BydaoBool(bool value = false, QObject* parent = nullptr);

public:

    static BydaoBool* create(bool value) {
        if (!s_cache.isEmpty()) {
            BydaoBool* obj = s_cache.takeLast();
            obj->m_value = value;
            return obj;
        }
        return new BydaoBool(value);
    }

    QString typeName() const override { return "Bool"; }
    bool value() const { return m_value; }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

private:

    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toInt(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toReal(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toBool(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_negate(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoBool::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    bool m_value;
};

} // namespace BydaoScript
