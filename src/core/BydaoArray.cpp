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

namespace BydaoScript {

// Получить мета-данные
MetaData*   BydaoArray::metaData() {
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
            ->appendObj( "sort",        FuncMetaData(0,"Void", FMD_ALTERABLE) << FuncArgMetaData("callback","Func",ARG_IN,"null") )
            .appendObj( "ksort",        FuncMetaData(1,"Void", FMD_ALTERABLE) << FuncArgMetaData("callback","Func",ARG_IN,"null") )
            .appendObj( "size",         FuncMetaData(2,"Int", FMD_ALTERABLE) )
            .appendObj( "toString",     FuncMetaData("String", FMD_IMMUTABLE) )
            .appendObj( "get",          FuncMetaData("Any", FMD_IMMUTABLE) << FuncArgMetaData("key","String",ARG_IN) )
            .appendObj( "set",          FuncMetaData("Void", FMD_ALTERABLE) << FuncArgMetaData("key","String",ARG_IN) << FuncArgMetaData("obj","Any",ARG_IN) )
            .appendObj( "iter",         FuncMetaData("DictIter", FMD_IMMUTABLE) )
            .appendObj( "keys",         FuncMetaData("Array", FMD_IMMUTABLE) )
            .appendObj( "slice",        FuncMetaData("Array", FMD_ALTERABLE) << FuncArgMetaData("start","Int",ARG_IN) << FuncArgMetaData("count","Int",ARG_IN,"null") )
            .appendObj( "indexOf",      FuncMetaData("Int", FMD_ALTERABLE) << FuncArgMetaData("obj","Any",ARG_IN) )
            .appendObj( "append",       FuncMetaData("Void", FMD_ALTERABLE) << FuncArgMetaData("obj","Any",ARG_IN) )
            .appendObj( "merge",        FuncMetaData("Void", FMD_ALTERABLE) << FuncArgMetaData("arr","Array",ARG_IN) )
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
        list << UsedMetaData( "DictIter", BydaoArrayIterator::metaData() );
    }

    return list;
}

BydaoArray::BydaoArray()
    : BydaoObject()
{
    // Методы объекта
    registerMethod("toString",  &BydaoArray::method_toString);
    registerMethod("get",       &BydaoArray::method_get);
    registerMethod("set",       &BydaoArray::method_set);
    registerMethod("sort",      &BydaoArray::method_sort);
    registerMethod("ksort",     &BydaoArray::method_ksort);
    registerMethod("iter",      &BydaoArray::method_iter);
    registerMethod("keys",      &BydaoArray::method_keys);
    registerMethod("size",      &BydaoArray::method_size);
    registerMethod("slice",     &BydaoArray::method_slice);
    registerMethod("indexOf",   &BydaoArray::method_indexOf);
    registerMethod("append",    &BydaoArray::method_append);
    registerMethod("merge",     &BydaoArray::method_merge);

    // Свойства объекта
    registerVar( "size",        &BydaoArray::getvar_size );

    // Регистрация функций для вызова по индексу

    m_stdMethodTable.resize(3);
    m_stdMethodTable[0] = &BydaoArray::sortImpl;
    m_stdMethodTable[1] = &BydaoArray::ksortImpl;
    m_stdMethodTable[2] = &BydaoArray::sizeImpl;
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
    ArrayType type = arrayType();
    if ( type == ARR_EMPTY || type == ARR_INDEXED ) {
        if ( 0 <= index && index < m_entries.size() ) {
            return m_entries[index].value;
        }
    }
    m_runtime->logError( "Invalid array type for 'at' operation" );
    return BydaoValue::fromNull();
}

bool    BydaoArray::append(const BydaoValue& value) {
    ArrayType type = arrayType();
    if ( type == ARR_EMPTY || type == ARR_INDEXED ) {
        m_entries.append( { BydaoValue(), value } );
        return true;
    }
    m_runtime->logError( "Invalid array type for 'append' operation" );
    return false;
}

BydaoValue  BydaoArray::get( const BydaoValue& key ) {
    ArrayType type = arrayType();
    if ( type == ARR_ASSOCIATIVE ) {
        auto iter = m_index.find( key );
        if ( iter == m_index.end() ) {  // не нашли такого ключа
            return BydaoValue::fromNull();
        }
        return m_entries[ iter.value() ].value;
    }
    else if ( type == ARR_INDEXED ) {
        int idx = key.toInt();
        if (0 <= idx && idx < m_entries.size()) {
            return m_entries[idx].value;
        }
    }
    return BydaoValue::fromNull();
}

bool        BydaoArray::set( const BydaoValue& key, const BydaoValue& value) {
    ArrayType type = arrayType();
    if ( type == ARR_EMPTY ) {
        type = key.isInt() ? ARR_INDEXED : ARR_ASSOCIATIVE;
    }
    if ( type == ARR_ASSOCIATIVE ) {
        if ( ! key.isString() ) {
            m_runtime->logError( "Invalid key type" );
            return false;
        }
        auto iter = m_index.find( key );
        if ( iter == m_index.end() ) {  // не нашли такого ключа

            // Добавить новый ключ и значение

            int index = m_entries.size();
            m_index[ key ] = index;
            m_entries.append( { key, value } );
        }
        else {
            m_entries[ iter.value() ].value = value;
        }
    }
    else {  // type == ARR_INDEXED
        if ( ! key.isInt() ) {
            m_runtime->logError( "Invalid index type" );
            return false;
        }
        int index = key.toInt();
        if (index >= 0) {
            if ( index >= m_entries.size() ) {
                m_entries.resize( index + 1 );
            }
            m_entries[index].value = value;
        }
    }
    return true;
}

// Для rvalue (временные объекты) — перемещаем
bool        BydaoArray::set(BydaoValue&& key, BydaoValue&& value) {
    ArrayType type = arrayType();
    if ( type == ARR_EMPTY ) {
        type = key.isInt() ? ARR_INDEXED : ARR_ASSOCIATIVE;
    }
    if ( type == ARR_ASSOCIATIVE ) {
        BydaoValue k = std::move(key);
        if ( ! key.isString() ) {
            m_runtime->logError( "Invalid key type" );
            return false;
        }
        auto iter = m_index.find( k );
        if ( iter == m_index.end() ) {  // не нашли такого ключа

            // Добавить новый ключ и значение

            int index = m_entries.size();
            m_index[ k ] = index;
            m_entries.append({ k, std::move(value)});
        }
        else {
            m_entries[ iter.value() ].value = std::move(value);
        }
    }
    else {  // type == ARR_INDEXED
        if ( ! key.isInt() ) {
            m_runtime->logError( "Invalid index type" );
            return false;
        }
        int index = key.toInt();
        if (index >= 0) {
            if ( index >= m_entries.size() ) {
                m_entries.resize( index + 1 );
            }
            m_entries[index].value = std::move(value);
        }
    }
    return true;
}

// ========== Методы ==========

bool    BydaoArray::method_iter(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoArrayIterator(this), BydaoTypeId::TYPE_OBJECT);
    return true;
}

bool BydaoArray::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    ArrayType type = arrayType();
    if ( type == ARR_EMPTY ) {
        result = BydaoValue::fromString("[]");
    }
    else if ( type == ARR_ASSOCIATIVE ) {
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
        result = BydaoValue::fromString("[" + parts.join(", ") + "]");
    }
    else {  // type == ARR_INDEXED
        QStringList parts;
        foreach ( const auto& elem, m_entries ) {
            if ( elem.value.isNull() ) {
                parts << "null";
            }
            else if ( elem.value.isString() ) {
                parts << QString("\"") + elem.value.toString() + "\"";
            }
            else {
                parts << elem.value.toString();
            }
        }
        result = BydaoValue::fromString("[" + parts.join(", ") + "]");
    }
    return true;
}

bool BydaoArray::method_keys(const QVector<BydaoValue>&, BydaoValue& result) {
    BydaoArray* arr = new BydaoArray();
    foreach ( const auto& elem, m_entries ) {
        arr->append( elem.key );
    }
    result = BydaoValue( arr, BydaoTypeId::TYPE_ARRAY );
    return true;
}

bool BydaoArray::method_size(const QVector<BydaoValue>&, BydaoValue& result) {
    result = BydaoValue::fromInt( m_entries.size() );
    return true;
}

bool BydaoArray::method_append(const QVector<BydaoValue>& args, BydaoValue&) {
    if (args.size() != 1) return false;

    ArrayType type = arrayType();
    if ( type == ARR_EMPTY || type == ARR_INDEXED ) {
        m_entries.append( { BydaoValue(), args[0] } );
        return true;
    }
    m_runtime->logError( "Invalid array type for 'append' operation" );
    return false;
}

bool BydaoArray::method_get(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    result = get( args[0] );
    return true;
}

bool BydaoArray::method_set(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() != 2) return false;
    set( args[0], args[1]);
    return true;
}

bool BydaoArray::method_merge(const QVector<BydaoValue>& args, BydaoValue& ) {
    if (args.size() < 1) return false;
    if ( args[0].typeId() != TYPE_ARRAY ) {
        m_runtime->logError( "Invalid argument type for operation 'merge'" );
        return false;
    }

    BydaoArray* arr = ((BydaoArray*)args[0].toObject() );
    ArrayType type = arr->arrayType();
    if ( type == ARR_ASSOCIATIVE ) {
        m_runtime->logError( "Associative array not support operation 'merge'" );
        return false;
    }
    foreach (const auto& elem, arr->m_entries ) {
        m_entries.append( elem );
    }
    return true;
}

bool BydaoArray::method_ksort(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() != 1) return false;
    if ( arrayType() == ARR_INDEXED ) {
        m_runtime->logError( "Indexed array not support operation 'merge'" );
        return false;
    }
    if ( m_entries.size() > 0 ) {
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
        for ( int i = 0; i < m_entries.size(); ++i ) {
            m_index[ m_entries[i].key ] = i;
        }
    }
    return true;
}

bool BydaoArray::method_sort(const QVector<BydaoValue>& args, BydaoValue& result) {
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
    else {                      // сортировка с использованием колбек-функции

        QVector<BydaoValue> callArgs(2);
        BydaoValue cmpResult;
        std::sort( m_entries.begin(), m_entries.end(), [this,callback,&callArgs,&cmpResult](const Entry& itemA, const Entry& itemB) {
            callArgs[0] = itemA.value;
            callArgs[1] = itemB.value;
            m_runtime->callFunction(callback, callArgs, cmpResult);
            return cmpResult.toBool();
        });
    }
    if ( arrayType() == ARR_ASSOCIATIVE ) {
        for ( int i = 0; i < m_entries.size(); ++i ) {
            m_index[ m_entries[i].key ] = i;
        }
    }
    return true;
}

bool BydaoArray::method_slice(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    ArrayType type = arrayType();
    if ( type == ARR_EMPTY ) {
        result = BydaoValue(new BydaoArray(), BydaoTypeId::TYPE_ARRAY);
        return true;
    }
    if ( type == ARR_ASSOCIATIVE ) {
        m_runtime->logError( "Invalid array type for operation 'slice'" );
        return false;
    }

    qint64 start = args[0].toInt();
    if ( start < 0 ) {
        start = 0;
    }
    qint64 count = ( args.size() == 2 && ! args[1].isNull() ) ? args[1].toInt() : m_entries.size();
    if ( count <= 0 ) {
        result = BydaoValue(new BydaoArray(), BydaoTypeId::TYPE_ARRAY);
        return true;
    }
    if ( start + count > m_entries.size() ) {
        count = m_entries.size() - start;
    }

    auto* newArray = new BydaoArray();
    for (qint64 i = 0; i < count; ++i) {
        newArray->append( m_entries[ start + i ].value.copy() );
    }
    result = BydaoValue( newArray, BydaoTypeId::TYPE_ARRAY );
    return true;
}

bool BydaoArray::method_indexOf(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    ArrayType type = arrayType();
    if ( type == ARR_EMPTY ) {
        result = BydaoValue::fromInt( -1 );
        return true;
    }
    if ( type == ARR_ASSOCIATIVE ) {
        m_runtime->logError( "Invalid array type for operation 'indexOf'" );
        return false;
    }
    const BydaoValue& val = args[0];

    int count = m_entries.size();
    for( int i = 0; i < count; ++i ) {
        BydaoObject* obj = m_entries[ i ].value.toObject();
        if ( obj && obj->eq( val ).toBool() ) {
            result = BydaoValue::fromInt( i );
            return true;
        }
    }
    result = BydaoValue::fromInt( -1 );
    return true;
}

/*==============================================================================
 *  Итератор по словарю
 */

// Получить мета-данные
MetaData*   BydaoArrayIterator::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData
            // методы объекта
            ->appendObj( "next",    FuncMetaData( 0, "Bool", FMD_ALTERABLE) )
            .appendObj( "value",    FuncMetaData( 1, "Int",  FMD_IMMUTABLE) )
            .appendObj( "key",      FuncMetaData( 2, "Int",  FMD_IMMUTABLE) )
            .appendObj( "isValid",  FuncMetaData( 3, "Bool", FMD_IMMUTABLE) )
            ;
        metaData
            // переменные объекта
            ->appendObj( "key",     VarMetaData("Int", VMD_CONST) )
            .appendObj( "value",    VarMetaData("Any", VMD_CONST) )
            ;
    }
    return metaData;
}

BydaoArrayIterator::BydaoArrayIterator(BydaoArray* dict)
    : BydaoIterator()
    , m_arr(dict)
    , m_index(-1)
{
    if (m_arr) {
        m_arr->ref();
    }

    m_stdMethodTable.resize(4);
    m_stdMethodTable[0] = &BydaoArrayIterator::nextImpl;
    m_stdMethodTable[1] = &BydaoArrayIterator::valueImpl;
    m_stdMethodTable[2] = &BydaoArrayIterator::keyImpl;
    m_stdMethodTable[3] = &BydaoArrayIterator::isValidImpl;

    m_boolMethodTable.resize( 2 );
    m_boolMethodTable[0] = &BydaoArrayIterator::itNext;
    m_boolMethodTable[1] = &BydaoArrayIterator::itIsValid;

    m_valueMethodTable.resize( 2 );
    m_valueMethodTable[0] = &BydaoArrayIterator::itValue;
    m_valueMethodTable[1] = &BydaoArrayIterator::itKey;

    registerVar("key",      &BydaoArrayIterator::getvar_key );
    registerVar("value",    &BydaoArrayIterator::getvar_value );
}

BydaoArrayIterator::~BydaoArrayIterator() {
    if (m_arr) {
        m_arr->unref();
    }
}

void BydaoArrayIterator::registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter ) {
    m_vars[ name ] = { getter, setter };
}

bool    BydaoArrayIterator::getVar( const QString& varName, BydaoValue& value ) {
    auto it = m_vars.find( varName );
    if ( it == m_vars.end() ) {
        return BydaoIterator::getVar( varName, value );
    }
    GetVarPtr getter = it.value().getter;
    return ( this->*( getter) )( value );
}

bool BydaoArrayIterator::next() {
    if ( m_arr ) {
        if ( m_index < m_arr->size() ) {
            return ( ++m_index < m_arr->size() );
        }
    }
    return false;
}

bool BydaoArrayIterator::isValid() const {
    if ( m_arr ) {
        return ( m_index < m_arr->size() );
    }
    return false;
}

BydaoValue BydaoArrayIterator::key() const {
    if ( m_arr && m_index < m_arr->size() ) {
        if ( m_arr->arrayType() == BydaoArray::ARR_INDEXED ) {
            return BydaoValue::fromInt( m_index );
        }
        else if ( m_arr->arrayType() == BydaoArray::ARR_ASSOCIATIVE ) {
            return m_arr->key( m_index );
        }
    }
    return BydaoValue::fromNull();
}

BydaoValue BydaoArrayIterator::value() const {
    if ( m_arr && m_index < m_arr->size() ) {
        return m_arr->value( m_index );
    }
    return BydaoValue::fromNull();
}

} // namespace BydaoScript
