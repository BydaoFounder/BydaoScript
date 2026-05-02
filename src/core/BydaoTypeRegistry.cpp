// Copyright 2026 Oleh Horshkov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
    s_classes["Int"] = BydaoValue(intClass, BydaoTypeId::TYPE_OBJECT);
    
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