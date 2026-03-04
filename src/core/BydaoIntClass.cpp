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
#include "BydaoScript/BydaoIntClass.h"
#include "BydaoScript/BydaoIntRange.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoString.h"
#include <QRandomGenerator>

namespace BydaoScript {

BydaoIntClass::BydaoIntClass(QObject* parent)
    : BydaoNative(parent)
{
    registerMethod("range", &BydaoIntClass::method_range);
    registerMethod("parse", &BydaoIntClass::method_parse);
    registerMethod("max", &BydaoIntClass::method_max);
    registerMethod("min", &BydaoIntClass::method_min);
    registerMethod("random", &BydaoIntClass::method_random);
}

void BydaoIntClass::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoIntClass::callMethod(const QString& name,
                               const QVector<BydaoValue>& args,
                               BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

// Int.range(start, end) -> возвращает итератор по числам от start до end-1
bool BydaoIntClass::method_range(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;
    
    qint64 start = args[0].toInt();
    qint64 end = args[1].toInt();
    
    if (start >= end) {
        result = BydaoValue(new BydaoIntRange(0, 0));  // пустой диапазон
        return true;
    }
    
    result = BydaoValue(new BydaoIntRange(start, end));
    return true;
}

// Int.parse(str) -> преобразует строку в число
bool BydaoIntClass::method_parse(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    
    bool ok;
    qint64 val = args[0].toString().toLongLong(&ok);
    if (!ok) return false;
    
    result = BydaoValue::fromInt(val);
    return true;
}

// Int.max(a, b) -> возвращает максимальное из двух чисел
bool BydaoIntClass::method_max(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;
    
    qint64 a = args[0].toInt();
    qint64 b = args[1].toInt();
    
    result = BydaoValue::fromInt(a > b ? a : b);
    return true;
}

// Int.min(a, b) -> возвращает минимальное из двух чисел
bool BydaoIntClass::method_min(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;
    
    qint64 a = args[0].toInt();
    qint64 b = args[1].toInt();
    
    result = BydaoValue::fromInt(a < b ? a : b);
    return true;
}

// Int.random(max) или Int.random(min, max) -> случайное число
bool BydaoIntClass::method_random(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() == 1) {
        // random(max)
        qint64 max = args[0].toInt();
        if (max <= 0) return false;
        result = BydaoValue::fromInt(QRandomGenerator::global()->bounded(max));
        return true;
    }
    else if (args.size() == 2) {
        // random(min, max)
        qint64 min = args[0].toInt();
        qint64 max = args[1].toInt();
        if (min >= max) return false;
        result = BydaoValue::fromInt(QRandomGenerator::global()->bounded(min, max));
        return true;
    }
    return false;
}

} // namespace BydaoScript