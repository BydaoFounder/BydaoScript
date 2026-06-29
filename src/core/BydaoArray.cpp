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
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoArray.h"
#include "BydaoScript/BydaoArrayIterator.h"

namespace BydaoScript {

// Получить мета-данные
MetaData*   BydaoArray::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData->name = "Array";
        metaData
            // переменные объекта
            ->appendObj( "type",        VarMetaData("String",VMD_CONST) )
            .appendObj( "length",       VarMetaData("Int", VMD_CONST) );
        metaData
            // методы объекта
            ->appendObj( "append",      FuncMetaData(0,"Void", FMD_ALTERABLE) << FuncArgMetaData("obj","Any",ARG_IN) )
            .appendObj( "sort",         FuncMetaData(1,"Void", FMD_ALTERABLE) << FuncArgMetaData("callback","Func",ARG_IN,"null") )
            .appendObj( "iter",         FuncMetaData("ArrayIter", FMD_IMMUTABLE) )
            .appendObj( "toString",     FuncMetaData("String", FMD_IMMUTABLE) )
            .appendObj( "get",          FuncMetaData("Int", FMD_IMMUTABLE) << FuncArgMetaData("pos","Int",ARG_IN) )
            .appendObj( "set",          FuncMetaData("Void", FMD_ALTERABLE) << FuncArgMetaData("pos","Int",ARG_IN) << FuncArgMetaData("obj","Any",ARG_IN) )
            .appendObj( "takeLast",     FuncMetaData("Any", FMD_ALTERABLE) )
            .appendObj( "takeFirst",    FuncMetaData("Any", FMD_ALTERABLE) )
            .appendObj( "prepend",      FuncMetaData("Void", FMD_ALTERABLE) << FuncArgMetaData("obj","Any",ARG_IN) )
            .appendObj( "merge",        FuncMetaData("Void", FMD_ALTERABLE) << FuncArgMetaData("arr","Array",ARG_IN) )
            .appendObj( "glue",         FuncMetaData("String", FMD_ALTERABLE) << FuncArgMetaData("symbol","String",ARG_IN,"\";\"") )
            .appendObj( "slice",        FuncMetaData("Array", FMD_ALTERABLE) << FuncArgMetaData("start","Int",ARG_IN) << FuncArgMetaData("count","Int",ARG_IN,"null") )
            .appendObj( "remove",       FuncMetaData("Void", FMD_ALTERABLE) << FuncArgMetaData("start","Int",ARG_IN) << FuncArgMetaData("count","Int",ARG_IN,"null") )
            .appendObj( "take",         FuncMetaData("Int", FMD_ALTERABLE) << FuncArgMetaData("pos","Int",ARG_IN) << FuncArgMetaData("count","Int",ARG_IN,"null") )
            .appendObj( "indexOf",      FuncMetaData("Int", FMD_ALTERABLE) << FuncArgMetaData("obj","Any",ARG_IN) )
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
    // Методы объекта
    registerMethod("iter",      &BydaoArray::method_iter);
    registerMethod("toString",  &BydaoArray::method_toString);
    registerMethod("get",       &BydaoArray::method_get);
    registerMethod("set",       &BydaoArray::method_set);
    registerMethod("append",    &BydaoArray::method_append);
    registerMethod("takeLast",  &BydaoArray::method_takeLast);
    registerMethod("takeFirst", &BydaoArray::method_takeFirst);
    registerMethod("prepend",   &BydaoArray::method_prepend);
    registerMethod("glue",      &BydaoArray::method_glue);
    registerMethod("merge",     &BydaoArray::method_merge);
    registerMethod("slice",     &BydaoArray::method_slice);
    registerMethod("take",      &BydaoArray::method_take);
    registerMethod("remove",    &BydaoArray::method_remove);
    registerMethod("indexOf",   &BydaoArray::method_indexOf);
    registerMethod("sort",      &BydaoArray::method_sort);

    // Свойства объекта
    registerVar( "length",  &BydaoArray::getvar_length );

    // Регистрация функций для вызова по индексу

    m_stdMethodTable.resize(2);
    m_stdMethodTable[0] = &BydaoArray::appendImpl;
    m_stdMethodTable[1] = &BydaoArray::sortImpl;
}

void BydaoArray::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

void BydaoArray::registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter ) {
    m_vars[ name ] = { getter, setter };
}

bool    BydaoArray::getVar( const QString& varName, BydaoValue& value ) {
    auto it = m_vars.find( varName );
    if ( it == m_vars.end() ) {
        return BydaoObject::getVar( varName, value );
    }
    GetVarPtr getter = it.value().getter;
    return ( this->*( getter) )( value );
}

bool BydaoArray::callMethod(const QString& name,
                           const QVector<BydaoValue>& args,
                           BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

BydaoValue BydaoArray::iter() {
    return BydaoValue(new BydaoArrayIterator(this), BydaoTypeId::TYPE_OBJECT);
}

// ========== Реализация методов массива ==========

BydaoValue BydaoArray::at(qint64 index) const {
    if (index >= 0 && index < m_elements.size()) {
        return m_elements[index];
    }
    return BydaoValue::fromNull();
}

void BydaoArray::set(qint64 index, const BydaoValue& value) {
    if (index >= 0) {
        if (index >= m_elements.size()) {
            m_elements.resize(index + 1);
        }
        m_elements[index] = value;
    }
}

BydaoValue  BydaoArray::get(const BydaoValue& index) {
    int idx = index.toInt();
    if (0 <= idx && idx < m_elements.size()) {
        return m_elements[idx];
    }
    return BydaoValue::fromNull();
}

void BydaoArray::append(const BydaoValue& value) {
    m_elements.append(value);
}

void BydaoArray::insert(qint64 index, const BydaoValue& value) {
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

bool BydaoArray::method_iter(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoArrayIterator(this), BydaoTypeId::TYPE_OBJECT);
    return true;
}

bool BydaoArray::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    QStringList parts;
    foreach (const auto& elem, m_elements) {
        QString str = elem.toString();
        BydaoObject* obj = elem.toObject();
        if ( obj ) {
            if ( obj->typeName() == "String" ) {
                str = "\"" + str + "\"";
            }
        }
        parts << str;
    }
    result = BydaoValue::fromString("[" + parts.join(", ") + "]");
    return true;
}

bool BydaoArray::method_length(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue::fromInt(m_elements.size());
    return true;
}

bool BydaoArray::method_get(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    int index = args[0].toInt();
    result = at(index);
    return true;
}

bool BydaoArray::method_set(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() != 2) return false;
    set(args[0].toInt(), args[1]);
    return true;
}

bool BydaoArray::method_append(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() != 1) return false;
    append(args[0]);
    return true;
}

bool BydaoArray::method_takeLast(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = m_elements.isEmpty() ? BydaoValue::fromNull() : m_elements.takeLast();
    return true;
}

bool BydaoArray::method_takeFirst(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = m_elements.isEmpty() ? BydaoValue::fromNull() : m_elements.takeFirst();
    return true;
}

bool BydaoArray::method_prepend(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() != 1) return false;
    m_elements.prepend(args[0]);
    return true;
}

bool BydaoArray::method_glue(const QVector<BydaoValue>& args, BydaoValue& result) {
    QString sep = args.size() > 0 ? args[0].toString() : ",";

    QStringList parts;
    foreach (const auto& elem, m_elements) {
        parts << elem.toString();
    }
    result = BydaoValue::fromString(parts.join(sep));
    return true;
}

bool BydaoArray::method_merge(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() < 1) return false;
    if ( args[0].typeId() != TYPE_ARRAY ) return false;

    BydaoArray* arr = ((BydaoArray*)args[0].toObject() );
    foreach (const auto& elem, arr->m_elements) {
        m_elements.append( elem );
    }
    return true;
}

bool BydaoArray::method_slice(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    qint64 start = args[0].toInt();
    if ( start < 0 ) {
        start = 0;
    }
    qint64 count = ( args.size() == 2 && ! args[1].isNull() ) ? args[1].toInt() : m_elements.size();
    if ( count <= 0 ) {
        result = BydaoValue(new BydaoArray(), BydaoTypeId::TYPE_ARRAY);
        return true;
    }
    if ( start + count > m_elements.size() ) {
        count = m_elements.size() - start;
    }

    auto* newArray = new BydaoArray();
    for (qint64 i = 0; i < count; ++i) {
        newArray->append( m_elements[ start + i ].copy() );
    }
    result = BydaoValue( newArray, BydaoTypeId::TYPE_ARRAY );
    return true;
}

bool BydaoArray::method_take(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    qint64 start = args[0].toInt();
    if ( start < 0 ) {
        start = 0;
    }
    qint64 count = ( args.size() == 2 && ! args[1].isNull() ) ? args[1].toInt() : m_elements.size();
    if ( count <= 0 ) {
        result = BydaoValue(new BydaoArray(), BydaoTypeId::TYPE_ARRAY);
        return true;
    }
    if ( start + count > m_elements.size() ) {
        count = m_elements.size() - start;
    }

    auto* newArray = new BydaoArray();
    for (qint64 i = 0; i < count; i++) {
        newArray->append( m_elements.takeAt( start ) );
    }
    result = BydaoValue( newArray, BydaoTypeId::TYPE_ARRAY );
    return true;
}

bool BydaoArray::method_remove(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() < 1 || args.size() > 2) return false;

    qint64 start = args[0].toInt();
    qint64 count = (args.size() == 2 && ! args[1].isNull() ) ? args[1].toInt() : m_elements.size();
    if ( start + count > m_elements.size() ) {
        count = m_elements.size() - start;
    }
    if ( count > 0 ) {
        m_elements.remove( start, count );
    }
    return true;
}

bool BydaoArray::method_indexOf(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    const BydaoValue& val = args[0];

    int count = m_elements.size();
    for( int i = 0; i < count; ++i ) {
        BydaoObject* myObj = m_elements[ i ].toObject();
        if ( myObj && myObj->eq( val ).toBool() ) {
            result = BydaoValue::fromInt( i );
            return true;
        }
    }
    result = BydaoValue::fromInt( -1 );
    return true;
}

bool BydaoArray::method_sort(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() != 1) return false;
    const BydaoValue& callback = args[0];
    if ( callback.isNull() ) {  // сортировка по умолчанию

        std::sort(m_elements.begin(), m_elements.end(), [](const BydaoValue& a, const BydaoValue& b) {
            if (a.typeId() == BydaoTypeId::TYPE_INT && b.typeId() == BydaoTypeId::TYPE_INT) {
                return ((BydaoInt*)a.toObject())->value() < ((BydaoInt*)b.toObject())->value();
            }
            if (a.typeId() == BydaoTypeId::TYPE_STRING && b.typeId() == BydaoTypeId::TYPE_STRING) {
                return ((BydaoString*)a.toObject())->value() < ((BydaoString*)b.toObject())->value();
            }
            if (a.typeId() == BydaoTypeId::TYPE_REAL && b.typeId() == BydaoTypeId::TYPE_REAL) {
                return ((BydaoReal*)a.toObject())->value() < ((BydaoReal*)b.toObject())->value();
            }
            if ( a.isNull() ) {
                return ! b.isNull();
            }
            if ( b.isNull() ) {
                return false;
            }
            return a.toObject()->lessThan( b );
        });
    }
    else {                      // сортировка с использованием колбек-фукнции

        QVector<BydaoValue> callArgs(2);
        BydaoValue cmpResult;
        std::sort(m_elements.begin(), m_elements.end(), [this,callback,&callArgs,&cmpResult](BydaoValue& a, BydaoValue& b) {
            callArgs[0] = a;
            callArgs[1] = b;
            m_runtime->callFunction(callback, callArgs, cmpResult);
            return cmpResult.toBool();
        });
    }
    return true;
}

} // namespace BydaoScript
