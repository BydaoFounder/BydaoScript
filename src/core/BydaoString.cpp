#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoArray.h"
#include "BydaoScript/BydaoNull.h"
#ifdef QT_CRYPTOGRAPHICHASH_LIB
#include <QCryptographicHash>
#endif

namespace BydaoScript {

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
    registerMethod("substring", &BydaoString::method_substring);
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

    // Свойства
    registerProperty("length",
                     [this]() { return BydaoValue::fromInt(m_value.length()); },
                     nullptr,
                     BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));
}

void BydaoString::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
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

BydaoValue BydaoString::add(const BydaoValue& other) {
    if ( other.typeId() == TYPE_STRING ) {
        const auto* otherStr = static_cast<const BydaoString*>(other.toObject());
        return BydaoValue::fromString( m_value.append(otherStr->m_value) );
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

BydaoValue BydaoString::ne(const BydaoValue& other) {
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

bool BydaoString::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(m_value));
    return true;
}

bool BydaoString::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(m_value.isNull()));
    return true;
}

bool BydaoString::method_length(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoInt(m_value.length()));
    return true;
}

bool BydaoString::method_upper(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(m_value.toUpper()));
    return true;
}

bool BydaoString::method_lower(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(m_value.toLower()));
    return true;
}

bool BydaoString::method_trim(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(m_value.trimmed()));
    return true;
}

bool BydaoString::method_substring(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    int start = args[0].toInt();
    int end = (args.size() == 2) ? args[1].toInt() : m_value.length();

    if (start < 0 || start > m_value.length() || end < start || end > m_value.length()) {
        return false;
    }

    result = BydaoValue(new BydaoString(m_value.mid(start, end - start)));
    return true;
}

bool BydaoString::method_indexOf(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    QString sub = args[0].toString();
    int from = (args.size() == 2) ? args[1].toInt() : 0;

    int idx = m_value.indexOf(sub, from);
    result = BydaoValue(new BydaoInt(idx));
    return true;
}

bool BydaoString::method_contains(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sub = args[0].toString();
    result = BydaoValue(new BydaoBool(m_value.contains(sub)));
    return true;
}

bool BydaoString::method_startsWith(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sub = args[0].toString();
    result = BydaoValue(new BydaoBool(m_value.startsWith(sub)));
    return true;
}

bool BydaoString::method_endsWith(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sub = args[0].toString();
    result = BydaoValue(new BydaoBool(m_value.endsWith(sub)));
    return true;
}

bool BydaoString::method_split(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sep = args[0].toString();
    QStringList parts = m_value.split(sep);

    auto* array = new BydaoArray();
    for (const QString& part : parts) {
        array->append(BydaoValue(new BydaoString(part)));
    }

    result = BydaoValue(array);
    return true;
}

bool BydaoString::method_replace(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;

    QString before = args[0].toString();
    QString after = args[1].toString();

    result = BydaoValue(new BydaoString(m_value.replace(before, after)));
    return true;
}

bool BydaoString::method_toInt(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    bool ok = false;
    int val = m_value.toInt(&ok);
    if (ok) {
        result = BydaoValue(new BydaoInt(val));
        return true;
    }
    return false;
}

bool BydaoString::method_toReal(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    bool ok = false;
    double val = m_value.toDouble(&ok);
    if (ok) {
        result = BydaoValue(new BydaoReal(val));
        return true;
    }
    return false;
}

#ifdef QT_CRYPTOGRAPHICHASH_LIB
bool BydaoString::method_md5(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    QByteArray hash = QCryptographicHash::hash(m_value.toUtf8(), QCryptographicHash::Md5);
    result = BydaoValue(new BydaoString(QString::fromLatin1(hash.toHex())));
    return true;
}
#endif

} // namespace BydaoScript
