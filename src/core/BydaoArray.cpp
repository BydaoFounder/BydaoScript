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
#include "BydaoScript/BydaoArray.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"
#include "BydaoScript/BydaoArrayIterator.h"

namespace BydaoScript {

// Получить мета-данные
MetaData*   BydaoArray::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData
            // переменные объекта
            ->append( "length",     VarMetaData("Int",true,false) );
        metaData
            // методы объекта
            ->append( "iter",       FuncMetaData("ArrayIter", false, true) )
            .append( "toString",    FuncMetaData("String", false, true) )
            .append( "length",      FuncMetaData("Int", false, true) )
            .append( "get",         FuncMetaData("Int", false, true) << FuncArgMetaData("pos","Int",false) )
            .append( "set",         FuncMetaData("Void", false, true)
                                    << FuncArgMetaData("pos","Int",false)
                                    << FuncArgMetaData("obj","Any",false) )
            ;
    }
    return metaData;
}

/**
 * Вернуть список используемых типов.
 */
UsedMetaDataList    BydaoArray::usedMetaData() {
    static UsedMetaDataList list;

    if ( list.isEmpty() ) {
        list << UsedMetaData( "ArrayIter", BydaoArrayIterator::metaData() );
    }

    return list;
}

BydaoArray::BydaoArray()
    : BydaoObject()
{
    // Регистрация методов (без макросов)
    registerMethod("toString",  &BydaoArray::method_toString);
    registerMethod("length",    &BydaoArray::method_length);
    registerMethod("get",       &BydaoArray::method_get);
    registerMethod("set",       &BydaoArray::method_set);
    registerMethod("push",      &BydaoArray::method_push);
    registerMethod("pop",       &BydaoArray::method_pop);
    registerMethod("shift",     &BydaoArray::method_shift);
    registerMethod("unshift",   &BydaoArray::method_unshift);
    registerMethod("slice",     &BydaoArray::method_slice);
    registerMethod("join",      &BydaoArray::method_join);

    registerMethod("iter",      &BydaoArray::method_iter);  // ← добавить
}

void BydaoArray::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoArray::callMethod(const QString& name,
                           const BydaoValueList& args,
                           BydaoValue* result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

BydaoValue* BydaoArray::iter() {
    return BydaoValue::get( new BydaoArrayIterator(this) );
}

// ========== Реализация методов массива ==========

BydaoValue* BydaoArray::at(qint64 index) const {
    if (index >= 0 && index < m_elements.size()) {
        return m_elements[index];
    }
    return BydaoValue::get();
}

void BydaoArray::set(qint64 index, BydaoValue* value) {
    if (index >= 0) {
        if (index >= m_elements.size()) {
            m_elements.resize(index + 1);
        }
        m_elements[index] = value;
    }
}

BydaoValue* BydaoArray::get(const BydaoValue* index) {
    int idx = index->toInt();
    if (0 <= idx && idx < m_elements.size()) {
        return m_elements[idx];
    }
    return BydaoValue::get();
}

void BydaoArray::append(BydaoValue* value) {
    m_elements.append(value);
}

void BydaoArray::insert(qint64 index, BydaoValue* value) {
    if (index >= 0 && index <= m_elements.size()) {
        m_elements.insert(index, value);
    }
}

void BydaoArray::removeAt(qint64 index) {
    if (index >= 0 && index < m_elements.size()) {
        m_elements.removeAt(index);
    }
}

void BydaoArray::clear() {
    m_elements.clear();
}

// ========== Методы ==========

bool BydaoArray::method_iter(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set(new BydaoArrayIterator(this));
    return true;
}

bool BydaoArray::method_toString(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    QStringList parts;
    foreach (const auto& elem, m_elements) {
        parts << elem->toString();
    }
    result->set( BydaoString::create( "[" + parts.join(", ") + "]") );
    return true;
}

bool BydaoArray::method_length(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoInt::create( m_elements.size() ) );
    return true;
}

bool BydaoArray::method_get(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) return false;
    int index = args[0]->toInt();
    if ( 0 <= index && index < m_elements.size() ) {
        result->set( m_elements[ index ]->toObject() );
        return true;
    }
    qWarning() << "Invalid array index";
    return false;
}

bool BydaoArray::method_set(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED( result );
    if (args.size() != 2) return false;
    set( args[0]->toInt(), args[1] );
    return true;
}

bool BydaoArray::method_push(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED( result );
    if (args.size() != 1) return false;
    append(args[0]);
    return true;
}

bool BydaoArray::method_pop(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    if (m_elements.isEmpty()) {
        result->set( BydaoNull::instance() );
    } else {
        result->set( m_elements.takeLast()->toObject() );
    }
    return true;
}

bool BydaoArray::method_shift(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    if (m_elements.isEmpty()) {
        result->set( BydaoNull::instance() );
    } else {
        result->set( m_elements.takeFirst()->toObject() );
    }
    return true;
}

bool BydaoArray::method_unshift(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) return false;
    m_elements.prepend(args[0]);
    result->set( BydaoInt::create( m_elements.size() ) );
    return true;
}

bool BydaoArray::method_slice(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() < 1 || args.size() > 2) return false;

    qint64 start = args[0]->toInt();
    qint64 end = (args.size() == 2) ? args[1]->toInt() : m_elements.size();

    if (start < 0) start = 0;
    if (end > m_elements.size()) end = m_elements.size();
    if (start >= end) {
        result->set( new BydaoArray() );
        return true;
    }

    auto* newArray = new BydaoArray();
    for (qint64 i = start; i < end; i++) {
        newArray->append(m_elements[i]);
    }
    result->set( newArray );
    return true;
}

bool BydaoArray::method_join(const BydaoValueList& args, BydaoValue* result) {
    QString sep = args.size() > 0 ? args[0]->toString() : ",";

    QStringList parts;
    for (const auto& elem : m_elements) {
        parts << elem->toString();
    }
    result->set( BydaoString::create( parts.join(sep) ) );
    return true;
}

} // namespace BydaoScript
