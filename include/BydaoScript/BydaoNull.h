#pragma once

#include "BydaoNative.h"

namespace BydaoScript {

class BydaoNull : public BydaoNative {
public:
    static BydaoNull* instance();
    
    QString typeName() const override { return "Null"; }

private:
    BydaoNull();
    
    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toBool(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);
    
    static BydaoNull* s_instance;
};

} // namespace BydaoScript
