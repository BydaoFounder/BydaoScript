#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"
//#include <cmath>

namespace BydaoScript {

BydaoInt::BydaoInt(qint64 value, QObject* parent)
    : BydaoNative(parent)
    , m_value(value)
{
    registerMethod("toString", &BydaoInt::method_toString);
    registerMethod("toReal",   &BydaoInt::method_toReal);
    registerMethod("toBool",   &BydaoInt::method_toBool);
    registerMethod("abs",      &BydaoInt::method_abs);
    registerMethod("negate",   &BydaoInt::method_negate);
    registerMethod("isNull",   &BydaoInt::method_isNull);
    registerMethod("toHex",    &BydaoInt::method_toHex);
}

void BydaoInt::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoInt::callMethod(const QString& name,
                          const QVector<BydaoValue>& args,
                          BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

bool BydaoInt::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(QString::number(m_value)));
    return true;
}

bool BydaoInt::method_toReal(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoReal((double)m_value));
    return true;
}

bool BydaoInt::method_toBool(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(m_value != 0));
    return true;
}

bool BydaoInt::method_abs(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoInt(m_value >= 0 ? m_value : -m_value));
    return true;
}

bool BydaoInt::method_negate(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoInt(-m_value));
    return true;
}

bool BydaoInt::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(false));
    return true;
}

bool BydaoInt::method_toHex(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(QString::number(m_value, 16)));
    return true;
}

// ========== Операции ==========

BydaoValue BydaoInt::add(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromInt(m_value + otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromReal(m_value + otherReal->value());
    }
    case TYPE_STRING: {
        bool ok;
        qint64 val = other.toString().toLongLong(&ok);
        if (ok) return BydaoValue::fromInt(m_value + val);
        return BydaoValue();
    }
    default:
        return BydaoValue::fromInt(m_value + other.toInt());
    }
}

BydaoValue BydaoInt::sub(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromInt(m_value - otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromReal(m_value - otherReal->value());
    }
    case TYPE_STRING: {
        bool ok;
        qint64 val = other.toString().toLongLong(&ok);
        if (ok) return BydaoValue::fromInt(m_value - val);
        return BydaoValue();
    }
    default:
        return BydaoValue::fromInt(m_value - other.toInt());
    }
}

BydaoValue BydaoInt::mul(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromInt(m_value * otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromReal(m_value * otherReal->value());
    }
    default:
        return BydaoValue::fromInt(m_value * other.toInt());
    }
}

BydaoValue BydaoInt::div(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        if (otherInt->m_value == 0) {
            return BydaoValue();  // null
        }
        return BydaoValue::fromReal(static_cast<double>(m_value) / otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        if (otherReal->value() == 0.0) {
            return BydaoValue();
        }
        return BydaoValue::fromReal(m_value / otherReal->value());
    }
    default:
        return BydaoValue::fromReal(m_value / other.toReal());
    }
}

BydaoValue BydaoInt::neg() {
    return BydaoValue::fromInt(-m_value);
}

BydaoValue BydaoInt::eq(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromBool(m_value == otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromBool(m_value == otherReal->value());
    }
    default:
        return BydaoValue::fromBool(m_value == other.toInt());
    }
}

BydaoValue BydaoInt::lt(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromBool(m_value < otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromBool(m_value < otherReal->value());
    }
    default:
        return BydaoValue::fromBool(m_value < other.toReal());
    }
}

BydaoValue BydaoInt::le(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromBool(m_value <= otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromBool(m_value <= otherReal->value());
    }
    default:
        return BydaoValue::fromBool(m_value <= other.toReal());
    }
}

BydaoValue BydaoInt::gt(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromBool(m_value > otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromBool(m_value > otherReal->value());
    }
    default:
        return BydaoValue::fromBool(m_value > other.toReal());
    }
}

BydaoValue BydaoInt::ge(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromBool(m_value >= otherInt->m_value);
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromBool(m_value >= otherReal->value());
    }
    default:
        return BydaoValue::fromBool(m_value >= other.toReal());
    }
}

} // namespace BydaoScript
