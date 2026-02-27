#include "BydaoScript/BydaoTypeRegistry.h"
#include "BydaoScript/BydaoIntClass.h"
// TODO: добавить другие классы

namespace BydaoScript {

QHash<QString, BydaoValue> BydaoTypeRegistry::s_classes;

void BydaoTypeRegistry::ensureInitialized() {
    if (!s_classes.isEmpty()) return;
    
    // Инициализация встроенных классов
    auto* intClass = new BydaoIntClass();
    intClass->ref();
    s_classes["Int"] = BydaoValue(intClass);
    
    // TODO: добавить другие классы
    // s_classes["String"] = BydaoValue(new BydaoStringClass());
    // s_classes["Real"] = BydaoValue(new BydaoRealClass());
    // s_classes["Array"] = BydaoValue(new BydaoArrayClass());
}

BydaoValue BydaoTypeRegistry::getClass(const QString& name) {
    ensureInitialized();
    return s_classes.value(name);
}

} // namespace BydaoScript