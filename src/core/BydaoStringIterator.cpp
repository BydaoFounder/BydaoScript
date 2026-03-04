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
#include "BydaoScript/BydaoStringIterator.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"

namespace BydaoScript {

BydaoStringIterator::BydaoStringIterator(BydaoString* str, QObject* parent)
    : BydaoIterator(parent)
    , m_string(str)
    , m_index(-1)
{
    if (m_string) {
        m_string->ref();
    }
}

BydaoStringIterator::~BydaoStringIterator() {
    if (m_string) {
        m_string->unref();
    }
}

bool BydaoStringIterator::next() {
    if (!m_string) return false;
    
    m_index++;
    return m_index < m_string->length();
}

bool BydaoStringIterator::isValid() const {
    return m_string && m_index >= 0 && m_index < m_string->length();
}

BydaoValue BydaoStringIterator::key() const {
    if (!isValid()) return BydaoValue(BydaoNull::instance());
    return BydaoValue::fromInt(m_index);
}

BydaoValue BydaoStringIterator::value() const {
    if (!isValid()) return BydaoValue(BydaoNull::instance());
    return BydaoValue::fromString(QString(m_string->value().at(m_index)));
}

} // namespace BydaoScript