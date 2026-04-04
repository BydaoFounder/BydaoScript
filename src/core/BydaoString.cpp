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
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoArray.h"
#include "BydaoScript/BydaoNull.h"
#include "BydaoScript/BydaoStringIterator.h"
#ifdef QT_CRYPTOGRAPHICHASH_LIB
#include <QCryptographicHash>
#endif

namespace BydaoScript {

// Получить мета-данные
MetaData*   BydaoString::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData
            // переменные объекта
            ->append( "length",     VarMetaData("Int",true,false) );
        metaData
            // методы объекта
            ->append( "toString",   FuncMetaData("String", false, true) )
            .append( "isNull",      FuncMetaData("Bool", false, true) )
            .append( "length",      FuncMetaData("Int", false, true) )
            .append( "upper",       FuncMetaData("String", false, true) )
            .append( "lower",       FuncMetaData("String", false, true) )
            .append( "trim",        FuncMetaData("String", false, true) )
            .append( "substr",      FuncMetaData("String", false, true)
                                  << FuncArgMetaData("from","Int",false)
                                  << FuncArgMetaData("len","Int",false,"null")
                    )
            .append( "indexOf",     FuncMetaData("Int", false, true)
                                   << FuncArgMetaData("str","String",false)
                                   << FuncArgMetaData("pos","Int",false,"0")
                    )
            .append( "contains",    FuncMetaData("Bool", false, true)
                                    << FuncArgMetaData("str","String",false)
                    )
            .append( "startsWith",  FuncMetaData("Bool", false, true)
                                      << FuncArgMetaData("str","String",false)
                    )
            .append( "endsWith",    FuncMetaData("Bool", false, true)
                                    << FuncArgMetaData("str","String",false)
                    )
            .append( "toReal",      FuncMetaData("Real", false, true) )
            .append( "toInt",       FuncMetaData("Int", false, true) )
            .append( "toBool",      FuncMetaData("Bool", false, true) )
            .append( "isEmpty",     FuncMetaData("Bool", false, true) )
            ;
        metaData
            ->append( "eq",         OperMetaData("Any", "Bool" ) )
            .append( "neq",         OperMetaData("Any", "Bool" ) )
            .append( "lt",          OperMetaData("Any", "Bool" ) )
            .append( "le",          OperMetaData("Any", "Bool" ) )
            .append( "gt",          OperMetaData("Any", "Bool" ) )
            .append( "ge",          OperMetaData("Any", "Bool" ) )
            .append( "add",         OperMetaData("Any", "String" ) )
            .append( "addToValue",  OperMetaData("Any", "Void" ) )
            ;
    }
    return metaData;
}

/**
 * Вернуть список используемых типов.
 */
UsedMetaDataList    BydaoString::usedMetaData() {
    static UsedMetaDataList list;

    if ( list.isEmpty() ) {
        list << UsedMetaData( "StringArray",     BydaoStringArray::metaData() );
        list << UsedMetaData( "StringArrayIter", BydaoStringArrayIterator::metaData() );
    }

    return list;
}

QList<BydaoString*> BydaoString::s_cache;

BydaoString::BydaoString(const QString& value)
    : BydaoObject()
    , m_value(value)
{
    registerMethod("toString", &BydaoString::method_toString);
    registerMethod("isNull", &BydaoString::method_isNull);
    registerMethod("length", &BydaoString::method_length);
    registerMethod("upper", &BydaoString::method_upper);
    registerMethod("lower", &BydaoString::method_lower);
    registerMethod("trim", &BydaoString::method_trim);
    registerMethod("substr", &BydaoString::method_substring);
    registerMethod("indexOf", &BydaoString::method_indexOf);
    registerMethod("contains", &BydaoString::method_contains);
    registerMethod("startsWith", &BydaoString::method_startsWith);
    registerMethod("endsWith", &BydaoString::method_endsWith);
    registerMethod("split", &BydaoString::method_split);
    registerMethod("replace", &BydaoString::method_replace);
    registerMethod("toInt", &BydaoString::method_toInt);
    registerMethod("toReal", &BydaoString::method_toReal);
    registerMethod("toBool", &BydaoString::method_toBool);
    registerMethod("isEmpty", &BydaoString::method_toBool);
#ifdef QT_CRYPTOGRAPHICHASH_LIB
    registerMethod("md5", &BydaoString::method_md5);
#endif

    registerMethod("iter", &BydaoString::method_iter);

    // Свойства
    // registerProperty("length",
    //                  [this]() { return BydaoValue::fromInt(m_value.length()); },
    //                  nullptr,
    //                  BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));
}

void BydaoString::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoString::callMethod(const QString& name,
                          const BydaoValueList& args,
                          BydaoValue* result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

BydaoValue* BydaoString::iter() {
    return BydaoValue::get( new BydaoStringIterator(this) );
}

BydaoValue* BydaoString::add(const BydaoValue* other) {
    if ( other->typeId() == TYPE_STRING ) {
        const auto* otherStr = (const BydaoString*)(other->toObject());
        return BydaoValue::get( BydaoString::create( m_value + otherStr->m_value ) );
    }
    QString result = m_value + other->toString();
    return BydaoValue::get( BydaoString::create(result) );
}

void    BydaoString::addToValue(const BydaoValue* other) {
    if ( other->typeId() == TYPE_STRING ) {
        const auto* otherStr = (const BydaoString*)(other->toObject());
        m_value += otherStr->m_value;
    }
    else {
        m_value += other->toString();
    }
}

BydaoValue* BydaoString::eq(const BydaoValue* other) {
    if ( other->typeId() == TYPE_STRING ) {
        const auto* otherStr = (const BydaoString*)(other->toObject());
        return BydaoValue::get( BydaoBool::create( m_value == otherStr->m_value ) );
    }
    return BydaoValue::get( BydaoBool::create(m_value == other->toString()) );
}

BydaoValue* BydaoString::neq(const BydaoValue* other) {
    if ( other->typeId() == TYPE_STRING ) {
        const auto* otherStr = (const BydaoString*)(other->toObject());
        return BydaoValue::get( BydaoBool::create( m_value != otherStr->m_value ) );
    }
    return BydaoValue::get( BydaoBool::create(m_value != other->toString()) );
}

BydaoValue* BydaoString::lt(const BydaoValue* other) {
    if ( other->typeId() == TYPE_STRING ) {
        const auto* otherStr = (const BydaoString*)(other->toObject());
        return BydaoValue::get( BydaoBool::create( m_value < otherStr->m_value ) );
    }
    return BydaoValue::get( BydaoBool::create(m_value < other->toString()) );
}

BydaoValue* BydaoString::le(const BydaoValue* other) {
    if ( other->typeId() == TYPE_STRING ) {
        const auto* otherStr = (const BydaoString*)(other->toObject());
        return BydaoValue::get( BydaoBool::create( m_value <= otherStr->m_value ) );
    }
    return BydaoValue::get( BydaoBool::create(m_value <= other->toString()) );
}

BydaoValue* BydaoString::gt(const BydaoValue* other) {
    if ( other->typeId() == TYPE_STRING ) {
        const auto* otherStr = (const BydaoString*)(other->toObject());
        return BydaoValue::get( BydaoBool::create( m_value > otherStr->m_value ) );
    }
    return BydaoValue::get( BydaoBool::create(m_value > other->toString()) );
}

BydaoValue* BydaoString::ge(const BydaoValue* other) {
    if ( other->typeId() == TYPE_STRING ) {
        const auto* otherStr = (const BydaoString*)(other->toObject());
        return BydaoValue::get( BydaoBool::create( m_value >= otherStr->m_value ) );
    }
    return BydaoValue::get( BydaoBool::create(m_value >= other->toString()) );
}

// ========== Реализации методов ==========

bool BydaoString::method_iter(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( new BydaoStringIterator(this) );
    return true;
}

bool BydaoString::method_toString(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoString::create(m_value));
    return true;
}

bool BydaoString::method_isNull(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoBool::create( m_value.isNull() ));
    return true;
}

bool BydaoString::method_length(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoInt::create(m_value.length()));
    return true;
}

bool BydaoString::method_upper(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoString::create(m_value.toUpper()));
    return true;
}

bool BydaoString::method_lower(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoString::create(m_value.toLower()));
    return true;
}

bool BydaoString::method_trim(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoString::create(m_value.trimmed()));
    return true;
}

bool BydaoString::method_substring(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() < 1 || args.size() > 2) return false;

    qint64 start = args[0]->toInt();
    qint64 len = m_value.length() - start;
    if ( args.size() == 2) {
        if ( ! args[1]->isNull() ) {
            len = args[1]->toInt();
        }
    }

    if (start < 0 || start > m_value.length() || len < 0 || start + len > m_value.length()) {
        return false;
    }

    result->set( BydaoString::create( m_value.mid(start, len) ) );
    return true;
}

bool BydaoString::method_indexOf(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() < 1 || args.size() > 2) return false;

    QString sub = args[0]->toString();
    qint64 from = 0;
    if ( args.size() == 2 ) {
        if ( ! args[1]->isNull() ) {
            from = args[1]->toInt();
        }
    }

    long idx = m_value.indexOf(sub, from);
    result->set( BydaoInt::create(idx) );
    return true;
}

bool BydaoString::method_contains(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) return false;

    QString sub = args[0]->toString();
    result->set( BydaoBool::create(m_value.contains(sub)) );
    return true;
}

bool BydaoString::method_startsWith(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) return false;

    QString sub = args[0]->toString();
    result->set( BydaoBool::create(m_value.startsWith(sub)));
    return true;
}

bool BydaoString::method_endsWith(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) return false;

    QString sub = args[0]->toString();
    result->set( BydaoBool::create(m_value.endsWith(sub)));
    return true;
}

bool BydaoString::method_split(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 1) return false;

    QString sep = args[0]->toString();
    QStringList parts = m_value.split(sep);

    auto* array = new BydaoArray();
    for (const QString& part : parts) {
        array->append( BydaoValue::get( BydaoString::create(part) ) );
    }
    result->set( array );
    return true;
}

bool BydaoString::method_replace(const BydaoValueList& args, BydaoValue* result) {
    if (args.size() != 2) return false;

    QString before = args[0]->toString();
    QString after = args[1]->toString();

    result->set( BydaoString::create(m_value.replace(before, after)));
    return true;
}

bool BydaoString::method_toInt(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    bool ok = false;
    qint64 val = m_value.toLongLong(&ok);
    if (ok) {
        result->set( BydaoInt::create(val) );
        return true;
    }
    return false;
}

bool BydaoString::method_toReal(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    bool ok = false;
    double val = m_value.toDouble(&ok);
    if (ok) {
        result->set( BydaoReal::create( val ) );
        return true;
    }
    return false;
}

bool BydaoString::method_toBool(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoBool::create( ! m_value.isEmpty() ) );
    return true;
}

bool BydaoString::method_isEmpty(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    result->set( BydaoBool::create( m_value.isEmpty() ) );
    return true;
}

#ifdef QT_CRYPTOGRAPHICHASH_LIB
bool BydaoString::method_md5(const BydaoValueList& args, BydaoValue* result) {
    Q_UNUSED(args);
    QByteArray hash = QCryptographicHash::hash(m_value.toUtf8(), QCryptographicHash::Md5);
    result->set( (BydaoString::create(QString::fromLatin1(hash.toHex())));
    return true;
}
#endif

//==============================================================================

// Получить мета-данные
MetaData*   BydaoStringArray::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData
            // переменные объекта
            ->append( "length",     VarMetaData("Int",true,false) );
        metaData
            // методы объекта
            ->append( "iter",      FuncMetaData("StringArrayIter", false, true) )
            .append( "toString",   FuncMetaData("String", false, true) )
            .append( "length",     FuncMetaData("Int", false, true) )
            .append( "get",        FuncMetaData("String", false, true)
                                    << FuncArgMetaData("pos","Int",false) )
            .append( "set",        FuncMetaData("Void", false, true)
                                    << FuncArgMetaData("pos","Int",false)
                                    << FuncArgMetaData("val","String",false) )
            ;
    }
    return metaData;
}

BydaoStringArray::BydaoStringArray()
    : BydaoArray() {
}

BydaoValue* BydaoStringArray::iter() {
    return BydaoValue::get( new BydaoStringArrayIterator(this) );
}

//==============================================================================

// Получить мета-данные
MetaData*   BydaoStringArrayIterator::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData
            // методы объекта
            ->append( "next",    FuncMetaData("Bool", false, false) )
            .append( "isValid",  FuncMetaData("Bool", false, false) )
            ;
        metaData
            // переменные объекта
            ->append( "key",     VarMetaData("Int",true,false) )
            .append( "value",    VarMetaData("String",true,false) )
            ;
    }
    return metaData;
}

BydaoStringArrayIterator::BydaoStringArrayIterator(BydaoArray* array)
    : BydaoArrayIterator( array )
{
}

BydaoStringArrayIterator::~BydaoStringArrayIterator() {
}

} // namespace BydaoScript
