#pragma once

#include "BydaoNative.h"
#include <QVector>

namespace BydaoScript {

class BydaoArray : public BydaoNative {
public:
    BydaoArray();
    
    QString typeName() const override { return "Array"; }
    
    int length() const;
    BydaoValue at(int index) const;
    void set(int index, const BydaoValue& value);
    void append(const BydaoValue& value);
    void insert(int index, const BydaoValue& value);
    void removeAt(int index);
    void clear();

private:
    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_length(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_get(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_set(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_push(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_pop(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_shift(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_unshift(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_slice(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_join(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_map(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_filter(const QVector<BydaoValue>& args, BydaoValue& result);
    
    QVector<BydaoValue> m_elements;
};

} // namespace BydaoScript
