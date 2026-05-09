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
#include "BydaoScript/BydaoIntRange.h"  // для range

namespace BydaoScript {

// Получить мета-данные
MetaData*   BydaoIntClass::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData

            // Методы класса
            ->append( "range",  FuncMetaData(0,"IntRange", true, true)
                                  << FuncArgMetaData("from","Int")
                                  << FuncArgMetaData("to","Int")
                     )
            .append( "max",     FuncMetaData(1,"Int", true, true)
                               << FuncArgMetaData("arg1","Int")
                               << FuncArgMetaData("arg2","Int")
                    )
            .append( "min",     FuncMetaData(2,"Int", true, true)
                               << FuncArgMetaData("arg1","Int")
                               << FuncArgMetaData("arg2","Int")
                    )
            .append( "parse",   FuncMetaData(3,"Int", true, true)
                                 << FuncArgMetaData("str","String")
                    )
            .append( "random",  FuncMetaData(4,"Int", true, true)
                                  << FuncArgMetaData("from","Int")
                                  << FuncArgMetaData("to","Int")
                    )
            ;
    }
    return metaData;
}

/**
 * Вернуть список используемых типов.
 */
UsedMetaDataList    BydaoIntClass::usedMetaData() {
    static UsedMetaDataList list;

    if ( list.isEmpty() ) {
        list << UsedMetaData( "Int",          BydaoInt::metaData(), true );
        list << UsedMetaData( "IntRange",     BydaoIntRange::metaData() );
        list << UsedMetaData( "IntRangeIter", BydaoIntRangeIterator::metaData() );
    }

    return list;
}

//==============================================================================

BydaoIntClass::BydaoIntClass()
    : BydaoObject()
{
    // Регистрация функций модуля, вызываемых по имени

    registerMethod("range",     &BydaoIntClass::method_range);
    registerMethod("max",       &BydaoIntClass::method_max);
    registerMethod("min",       &BydaoIntClass::method_min);
    registerMethod("parse",     &BydaoIntClass::method_parse);
    registerMethod("random",    &BydaoIntClass::method_random);

    // Регистрация функций для вызова по индексу

    m_stdMethodTable.resize(5);
    m_stdMethodTable[0] = &BydaoIntClass::rangeImpl;
    m_stdMethodTable[1] = &BydaoIntClass::maxImpl;
    m_stdMethodTable[2] = &BydaoIntClass::minImpl;
    m_stdMethodTable[3] = &BydaoIntClass::parseImpl;
    m_stdMethodTable[4] = &BydaoIntClass::randomImpl;
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

//==============================================================================

// Int.range(start, end) -> возвращает итератор по числам от start до end-1
bool BydaoIntClass::method_range(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) {
        setError("Invalid argument count");
        return false;
    }
    
    qint64 start = args[0].toInt();
    qint64 end = args[1].toInt();
    if (start >= end) {
        result = BydaoValue( new BydaoIntRange(0, 0), BydaoTypeId::TYPE_OBJECT );  // пустой диапазон
    }
    else {
        result = BydaoValue( new BydaoIntRange(start, end), BydaoTypeId::TYPE_OBJECT );
    }
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

// Int.parse(str) -> преобразует строку в число
bool BydaoIntClass::method_parse(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    
    bool ok;
    qint64 val = args[0].toString().toLongLong(&ok);
    if (!ok) return false;
    
    result = BydaoValue::fromInt(val);
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
