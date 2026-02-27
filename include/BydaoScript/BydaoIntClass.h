#pragma once

#include "BydaoNative.h"
#include "BydaoIntRange.h"  // для range

namespace BydaoScript {

class BydaoIntClass : public BydaoNative {
    Q_OBJECT

public:
    explicit BydaoIntClass(QObject* parent = nullptr);
    virtual ~BydaoIntClass() = default;

    QString typeName() const override { return "IntClass"; }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

private:
    // Статические методы класса Int
    bool method_range(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_parse(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_max(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_min(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_random(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoIntClass::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;
};

} // namespace BydaoScript