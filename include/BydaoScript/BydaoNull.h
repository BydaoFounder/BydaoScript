#pragma once

#include "BydaoNative.h"

namespace BydaoScript {

class BydaoNull : public BydaoNative {
public:
    static BydaoNull* instance();
    
    QString typeName() const override { return "Null"; }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

    BydaoValue eq(const BydaoValue& other) override;
    BydaoValue neq(const BydaoValue& other) override;

private:

    BydaoNull();
    
    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toBool(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoNull::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;

    static BydaoNull* s_instance;
};

} // namespace BydaoScript
