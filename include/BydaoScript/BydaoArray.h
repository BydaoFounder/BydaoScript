#pragma once

#include "BydaoNative.h"
#include <QVector>

namespace BydaoScript {

class BydaoArray : public BydaoNative {
    Q_OBJECT

public:
    explicit BydaoArray(QObject* parent = nullptr);
    virtual ~BydaoArray() override = default;

    QString typeName() const override { return "Array"; }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

    int         size() const { return m_elements.size(); }
    BydaoValue  at(int index) const;
    void        set(int index, const BydaoValue& value);
    BydaoValue  get(const BydaoValue& index);
    void        append(const BydaoValue& value);
    void        insert(int index, const BydaoValue& value);
    void        removeAt(int index);
    void        clear();

private:

    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_length(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_get(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_set(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_push(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_pop(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_shift(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_unshift(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_slice(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_join(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoArray::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;

    QVector<BydaoValue> m_elements;
};

} // namespace BydaoScript
