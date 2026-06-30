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
#include "BydaoScript/BydaoDict.h"

namespace BydaoScript {

// Получить мета-данные
MetaData*   BydaoDict::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData->name = "Dict";
        metaData
            // переменные объекта
            ->appendObj( "type",        VarMetaData("String",VMD_CONST) )
            .appendObj( "size",         VarMetaData("Int", VMD_CONST) );
        metaData
            // методы объекта
            ->appendObj( "toString",    FuncMetaData("String", FMD_IMMUTABLE) )
            .appendObj( "get",          FuncMetaData("Any", FMD_IMMUTABLE) << FuncArgMetaData("key","String",ARG_IN) )
            .appendObj( "set",          FuncMetaData("Void", FMD_ALTERABLE) << FuncArgMetaData("key","String",ARG_IN) << FuncArgMetaData("obj","Any",ARG_IN) )
            .appendObj( "sort",         FuncMetaData(0,"Void", FMD_ALTERABLE) << FuncArgMetaData("callback","Func",ARG_IN,"null") )
            .appendObj( "ksort",        FuncMetaData(1,"Void", FMD_ALTERABLE) << FuncArgMetaData("callback","Func",ARG_IN,"null") )
            .appendObj( "iter",         FuncMetaData("DictIter", FMD_IMMUTABLE) )
            ;
    }
    return metaData;
}

/**
 * Вернуть список используемых типов.
 */
UsedMetaDataList    BydaoDict::usedMetaData() {
    static UsedMetaDataList list;

    if ( list.isEmpty() ) {
        list << UsedMetaData( "DictIter", BydaoDictIterator::metaData() );
    }

    return list;
}

BydaoDict::BydaoDict()
    : BydaoObject()
{
    // Методы объекта
    registerMethod("toString",  &BydaoDict::method_toString);
    registerMethod("get",       &BydaoDict::method_get);
    registerMethod("set",       &BydaoDict::method_set);
    registerMethod("sort",      &BydaoDict::method_sort);
    registerMethod("ksort",     &BydaoDict::method_ksort);
    registerMethod("iter",      &BydaoDict::method_iter);

    // Свойства объекта
    registerVar( "size",        &BydaoDict::getvar_size );

    // Регистрация функций для вызова по индексу

    m_stdMethodTable.resize(2);
    m_stdMethodTable[0] = &BydaoDict::sortImpl;
    m_stdMethodTable[1] = &BydaoDict::ksortImpl;
}

void BydaoDict::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

void BydaoDict::registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter ) {
    m_vars[ name ] = { getter, setter };
}

bool    BydaoDict::getVar( const QString& varName, BydaoValue& value ) {
    auto it = m_vars.find( varName );
    if ( it == m_vars.end() ) {
        return BydaoObject::getVar( varName, value );
    }
    GetVarPtr getter = it.value().getter;
    return ( this->*( getter) )( value );
}

bool BydaoDict::callMethod(const QString& name,
                           const QVector<BydaoValue>& args,
                           BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

BydaoValue BydaoDict::iter() {
    return BydaoValue(new BydaoDictIterator(this), BydaoTypeId::TYPE_OBJECT);
}

// ========== Реализация методов массива ==========

void    BydaoDict::set( const BydaoValue& key, const BydaoValue& value) {
    auto iter = m_index.find( key );
    if ( iter == m_index.end() ) {  // не нашли такого ключа

        // Добавить новый ключ и значение

        int index = m_entries.size();
        m_index[ key ] = index;
        m_entries.append( { key, value } );
        return;
    }
    m_entries[ iter.value() ].value = value;
}

BydaoValue  BydaoDict::get( const BydaoValue& key ) {
    auto iter = m_index.find( key );
    if ( iter == m_index.end() ) {  // не нашли такого ключа
        return BydaoValue::fromNull();
    }
    return m_entries[ iter.value() ].value;
}

// ========== Методы ==========

bool BydaoDict::method_iter(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoDictIterator(this), BydaoTypeId::TYPE_OBJECT);
    return true;
}

bool BydaoDict::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    QStringList parts;
    foreach ( const auto& elem, m_entries ) {
        QString str = "\"" + elem.key.toString() + "\": ";
        if ( elem.value.isNull() ) {
            str += "null";
        }
        else if ( elem.value.typeId() == TYPE_STRING ) {
            str += "\"" + elem.value.toString() + "\"";
        }
        else {
            str += elem.value.toString();
        }
        parts << str;
    }
    result = BydaoValue::fromString("#[" + parts.join(", ") + "]");
    return true;
}

bool BydaoDict::method_get(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    result = get( args[0] );
    return true;
}

bool BydaoDict::method_set(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() != 2) return false;
    set( args[0], args[1]);
    return true;
}

bool BydaoDict::method_ksort(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() != 1) return false;
    const BydaoValue& callback = args[0];
    if ( callback.isNull() ) {  // сортировка по умолчанию

        std::sort( m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b) {
            const BydaoValue& keyA = a.key;
            const BydaoValue& keyB = b.key;
            if ( keyA.isString() && keyB.isString() ) {
                return ( (BydaoString*) keyA.toObject() )->value() < ( (BydaoString*) keyB.toObject() )->value();
            }
            return keyA.toString() < keyB.toString();
        });
    }
    else {                      // сортировка с использованием колбек-фукнции

        QVector<BydaoValue> callArgs(2);
        BydaoValue cmpResult;
        std::sort( m_entries.begin(), m_entries.end(), [this,callback,&callArgs,&cmpResult](const Entry& a, const Entry& b) {
            callArgs[0] = a.key;
            callArgs[1] = b.key;
            m_runtime->callFunction(callback, callArgs, cmpResult);
            return cmpResult.toBool();
        });
    }
    return true;
}

bool BydaoDict::method_sort(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() != 1) return false;
    const BydaoValue& callback = args[0];
    if ( callback.isNull() ) {  // сортировка по умолчанию

        std::sort( m_entries.begin(), m_entries.end(), [](const Entry& itemA, const Entry& itemB) {

            const BydaoValue& a = itemA.value;
            const BydaoValue& b = itemB.value;

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
        std::sort( m_entries.begin(), m_entries.end(), [this,callback,&callArgs,&cmpResult](const Entry& itemA, const Entry& itemB) {
            callArgs[0] = itemA.value;
            callArgs[1] = itemB.value;
            m_runtime->callFunction(callback, callArgs, cmpResult);
            return cmpResult.toBool();
        });
    }
    return true;
}

/*==============================================================================
 *  Итератор по словарю
 */

// Получить мета-данные
MetaData*   BydaoDictIterator::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData
            // методы объекта
            ->appendObj( "next",    FuncMetaData("Bool", FMD_ALTERABLE) )
            .appendObj( "isValid",  FuncMetaData("Bool", FMD_IMMUTABLE) )
            ;
        metaData
            // переменные объекта
            ->appendObj( "key",     VarMetaData("Int", VMD_CONST) )
            .appendObj( "value",    VarMetaData("Any", VMD_CONST) )
            ;
    }
    return metaData;
}

BydaoDictIterator::BydaoDictIterator(BydaoDict* dict)
    : BydaoIterator()
    , m_dict(dict)
    , m_index(-1)
{
    if (m_dict) {
        m_dict->ref();
    }

    m_boolMethodTable.resize( 2 );
    m_boolMethodTable[0] = &BydaoDictIterator::itNext;
    m_boolMethodTable[1] = &BydaoDictIterator::itIsValid;

    m_valueMethodTable.resize( 2 );
    m_valueMethodTable[0] = &BydaoDictIterator::itValue;
    m_valueMethodTable[1] = &BydaoDictIterator::itKey;
}

BydaoDictIterator::~BydaoDictIterator() {
    if (m_dict) {
        m_dict->unref();
    }
}

bool BydaoDictIterator::next() {
    if ( m_dict && 0 <= m_index && m_index < m_dict->size() ) {
        return ( ++m_index < m_dict->size() );
    }
    return false;
}

bool BydaoDictIterator::isValid() const {
    return m_dict && 0 <= m_index && m_index < m_dict->size();
}

BydaoValue BydaoDictIterator::key() const {
    if ( ! isValid() ) {
        return BydaoValue::fromNull();
    }
    return m_dict->key( m_index );
}

BydaoValue BydaoDictIterator::value() const {
    if ( ! isValid() ) {
        return BydaoValue::fromNull();
    }
    return m_dict->value( m_index );
}

} // namespace BydaoScript
