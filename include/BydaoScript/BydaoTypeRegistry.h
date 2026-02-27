#pragma once

#include "BydaoValue.h"

namespace BydaoScript {

class BydaoTypeRegistry {
public:
    static BydaoValue getClass(const QString& name);

private:
    static QHash<QString, BydaoValue> s_classes;
    static void ensureInitialized();
};

} // namespace BydaoScript