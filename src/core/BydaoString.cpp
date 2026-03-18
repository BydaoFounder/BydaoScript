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

QVector<BydaoString*> BydaoString::s_cache;

BydaoString::BydaoString(const QString& value, QObject* parent)
    : BydaoNative(parent)
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
#ifdef QT_CRYPTOGRAPHICHASH_LIB
    registerMethod("md5", &BydaoString::method_md5);
#endif

    registerMethod("iter", &BydaoString::method_iter);

    // Свойства
    registerProperty("length",
                     [this]() { return BydaoValue::fromInt(m_value.length()); },
                     nullptr,
                     BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));
}

void BydaoString::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}


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
        ;
    }
    return metaData;
}

bool BydaoString::callMethod(const QString& name,
                          const QVector<BydaoValue>& args,
                          BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

BydaoValue BydaoString::iter() {
    return BydaoValue(new BydaoStringIterator(this));
}

BydaoValue BydaoString::add(const BydaoValue& other) {
    if ( other.typeId() == TYPE_STRING ) {
        const auto* otherStr = static_cast<const BydaoString*>(other.toObject());
        return BydaoValue::fromString( m_value + otherStr->m_value );
    }
    QString result = m_value + other.toString();
    return BydaoValue::fromString(result);
}

BydaoValue BydaoString::eq(const BydaoValue& other) {
    if ( other.typeId() == TYPE_STRING ) {
        const auto* otherStr = static_cast<const BydaoString*>(other.toObject());
        return BydaoValue::fromBool( m_value == otherStr->m_value );
    }
    return BydaoValue::fromBool(m_value == other.toString());
}

BydaoValue BydaoString::neq(const BydaoValue& other) {
    if ( other.typeId() == TYPE_STRING ) {
        const auto* otherStr = static_cast<const BydaoString*>(other.toObject());
        return BydaoValue::fromBool( m_value != otherStr->m_value );
    }
    return BydaoValue::fromBool(m_value != other.toString());
}

BydaoValue BydaoString::lt(const BydaoValue& other) {
    if ( other.typeId() == TYPE_STRING ) {
        const auto* otherStr = static_cast<const BydaoString*>(other.toObject());
        return BydaoValue::fromBool( m_value < otherStr->m_value );
    }
    return BydaoValue::fromBool(m_value < other.toString());
}

BydaoValue BydaoString::le(const BydaoValue& other) {
    if ( other.typeId() == TYPE_STRING ) {
        const auto* otherStr = static_cast<const BydaoString*>(other.toObject());
        return BydaoValue::fromBool( m_value <= otherStr->m_value );
    }
    return BydaoValue::fromBool(m_value <= other.toString());
}

BydaoValue BydaoString::gt(const BydaoValue& other) {
    if ( other.typeId() == TYPE_STRING ) {
        const auto* otherStr = static_cast<const BydaoString*>(other.toObject());
        return BydaoValue::fromBool( m_value > otherStr->m_value );
    }
    return BydaoValue::fromBool(m_value > other.toString());
}

BydaoValue BydaoString::ge(const BydaoValue& other) {
    if ( other.typeId() == TYPE_STRING ) {
        const auto* otherStr = static_cast<const BydaoString*>(other.toObject());
        return BydaoValue::fromBool( m_value >= otherStr->m_value );
    }
    return BydaoValue::fromBool(m_value >= other.toString());
}

// ========== Реализации методов ==========

bool BydaoString::method_iter(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoStringIterator(this));
    return true;
}

bool BydaoString::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoString::create(m_value));
    return true;
}

bool BydaoString::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoBool::create(m_value.isNull()));
    return true;
}

bool BydaoString::method_length(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoInt::create(m_value.length()));
    return true;
}

bool BydaoString::method_upper(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoString::create(m_value.toUpper()));
    return true;
}

bool BydaoString::method_lower(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoString::create(m_value.toLower()));
    return true;
}

bool BydaoString::method_trim(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoString::create(m_value.trimmed()));
    return true;
}

bool BydaoString::method_substring(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    qint64 start = args[0].toInt();
    qint64 len = m_value.length() - start;
    if ( args.size() == 2) {
        if ( ! args[1].isNull() ) {
            len = args[1].toInt();
        }
    }

    if (start < 0 || start > m_value.length() || len < 0 || start + len > m_value.length()) {
        return false;
    }

    result = BydaoValue( BydaoString::create( m_value.mid(start, len) ) );
    return true;
}

bool BydaoString::method_indexOf(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    QString sub = args[0].toString();
    qint64 from = 0;
    if ( args.size() == 2 ) {
        if ( ! args[1].isNull() ) {
            from = args[1].toInt();
        }
    }

    long idx = m_value.indexOf(sub, from);
    result = BydaoValue(BydaoInt::create(idx));
    return true;
}

bool BydaoString::method_contains(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sub = args[0].toString();
    result = BydaoValue( BydaoBool::create(m_value.contains(sub)) );
    return true;
}

bool BydaoString::method_startsWith(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sub = args[0].toString();
    result = BydaoValue( BydaoBool::create(m_value.startsWith(sub)));
    return true;
}

bool BydaoString::method_endsWith(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sub = args[0].toString();
    result = BydaoValue(BydaoBool::create(m_value.endsWith(sub)));
    return true;
}

bool BydaoString::method_split(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sep = args[0].toString();
    QStringList parts = m_value.split(sep);

    auto* array = new BydaoArray();
    for (const QString& part : parts) {
        array->append(BydaoValue(BydaoString::create(part)));
    }

    result = BydaoValue(array);
    return true;
}

bool BydaoString::method_replace(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;

    QString before = args[0].toString();
    QString after = args[1].toString();

    result = BydaoValue(BydaoString::create(m_value.replace(before, after)));
    return true;
}

bool BydaoString::method_toInt(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    bool ok = false;
    qint64 val = m_value.toLongLong(&ok);
    if (ok) {
        result = BydaoValue(BydaoInt::create(val));
        return true;
    }
    return false;
}

bool BydaoString::method_toReal(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    bool ok = false;
    double val = m_value.toDouble(&ok);
    if (ok) {
        result = BydaoValue::fromReal( val );
        return true;
    }
    return false;
}

#ifdef QT_CRYPTOGRAPHICHASH_LIB
bool BydaoString::method_md5(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    QByteArray hash = QCryptographicHash::hash(m_value.toUtf8(), QCryptographicHash::Md5);
    result = BydaoValue(BydaoString::create(QString::fromLatin1(hash.toHex())));
    return true;
}
#endif

} // namespace BydaoScript
