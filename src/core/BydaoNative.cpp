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
#include "BydaoScript/BydaoNative.h"
#include "BydaoScript/BydaoNull.h"

namespace BydaoScript {

BydaoNative::BydaoNative() {
}

BydaoNative::~BydaoNative() {
}

// ========== Свойства ==========

void BydaoNative::registerProperty(const QString& name,
                                   std::function<BydaoValue()> getter,
                                   std::function<bool(const BydaoValue&)> setter,
                                   const BydaoPropertyInfo& info) {
    PropertyEntry entry;
    entry.info = info;
    entry.getter = getter;
    entry.setter = setter;
    m_properties[name] = entry;
}

bool BydaoNative::hasProperty(const QString& name) const {
    return m_properties.contains(name);
}

bool BydaoNative::canGetProperty(const QString& name) const {
    auto it = m_properties.find(name);
    if (it == m_properties.end()) return false;
    return it.value().info.visibility != BydaoPropertyInfo::Internal;
}

bool BydaoNative::canSetProperty(const QString& name) const {
    auto it = m_properties.find(name);
    if (it == m_properties.end()) return false;
    if (it.value().info.visibility == BydaoPropertyInfo::Internal) return false;
    if (it.value().info.access == BydaoPropertyInfo::ReadOnly) return false;
    return it.value().setter != nullptr;
}

BydaoValue BydaoNative::getProperty(const QString& name) {
    auto it = m_properties.find(name);
    if (it == m_properties.end()) return BydaoValue();
    if (!canGetProperty(name)) return BydaoValue();
    if (it.value().getter) {
        return it.value().getter();
    }
    return BydaoValue();
}

bool BydaoNative::setProperty(const QString& name, const BydaoValue& value) {
    auto it = m_properties.find(name);
    if (it == m_properties.end()) return false;
    if (!canSetProperty(name)) return false;
    if (it.value().setter) {
        return it.value().setter(value);
    }
    return false;
}

} // namespace BydaoScript
