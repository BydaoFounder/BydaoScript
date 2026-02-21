#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"
#include <cmath>

namespace BydaoScript {

QVector<BydaoReal*> BydaoReal::s_cache;

BydaoReal::BydaoReal(double value, QObject* parent)
    : BydaoNative(parent)
    , m_value(value)
{
    registerMethod("toString", &BydaoReal::method_toString);
    registerMethod("toFixed", &BydaoReal::method_toFixed);
    registerMethod("toInt", &BydaoReal::method_toInt);
    registerMethod("toBool", &BydaoReal::method_toBool);
    registerMethod("abs", &BydaoReal::method_abs);
    registerMethod("negate", &BydaoReal::method_negate);
    registerMethod("isNull", &BydaoReal::method_isNull);
    registerMethod("round", &BydaoReal::method_round);
    registerMethod("floor", &BydaoReal::method_floor);
    registerMethod("ceil", &BydaoReal::method_ceil);
}

void BydaoReal::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoReal::callMethod(const QString& name,
                          const QVector<BydaoValue>& args,
                          BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

bool BydaoReal::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoString::create(QString::number(m_value)));
    return true;
}

bool BydaoReal::method_toFixed(const QVector<BydaoValue>& args, BydaoValue& result) {
    int decimals = args.size() > 0 ? (int)args[0].toInt() : 0;
    result = BydaoValue(BydaoString::create(QString::number(m_value,'f',decimals)));
    return true;
}

bool BydaoReal::method_toInt(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoInt::create((qint64)m_value));
    return true;
}

bool BydaoReal::method_toBool(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoBool::create(m_value != 0.0));
    return true;
}

bool BydaoReal::method_abs(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoReal::create(std::abs(m_value)));
    return true;
}

bool BydaoReal::method_negate(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoReal::create(-m_value));
    return true;
}

bool BydaoReal::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoBool::create(false));
    return true;
}

bool BydaoReal::method_round(const QVector<BydaoValue>& args, BydaoValue& result) {
    int decimals = args.size() > 0 ? (int)args[0].toInt() : 0;
    double multiplier = std::pow(10.0, decimals);
    double rounded = std::round(m_value * multiplier) / multiplier;
    result = BydaoValue( BydaoReal::create(rounded));
    return true;
}

bool BydaoReal::method_floor(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoReal::create(std::floor(m_value)));
    return true;
}

bool BydaoReal::method_ceil(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoReal::create(std::ceil(m_value)));
    return true;
}

// ========== Операции ==========

BydaoValue BydaoReal::add(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromReal(m_value + otherInt->value());
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromReal(m_value + otherReal->m_value);
    }
    case TYPE_STRING: {
        bool ok;
        double val = other.toString().toDouble(&ok);
        if (ok) return BydaoValue::fromInt(m_value + val);
        return BydaoValue();
    }
    default:
        return BydaoValue::fromInt(m_value + other.toInt());
    }
}

BydaoValue BydaoReal::sub(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromInt(m_value - otherInt->value());
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromReal(m_value - otherReal->m_value);
    }
    case TYPE_STRING: {
        bool ok;
        double val = other.toString().toDouble(&ok);
        if (ok) return BydaoValue::fromInt(m_value - val);
        return BydaoValue();
    }
    default:
        return BydaoValue::fromInt(m_value - other.toInt());
    }
}

BydaoValue BydaoReal::mul(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromInt(m_value * otherInt->value());
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromReal(m_value * otherReal->m_value);
    }
    case TYPE_STRING: {
        bool ok;
        double val = other.toString().toDouble(&ok);
        if (ok) return BydaoValue::fromInt(m_value * val);
        return BydaoValue();
    }
    default:
        return BydaoValue::fromInt(m_value * other.toInt());
    }
}

BydaoValue BydaoReal::div(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        if (otherInt->value() == 0) {
            return BydaoValue();  // null
        }
        return BydaoValue::fromReal(static_cast<double>(m_value) / otherInt->value());
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        if (otherReal->m_value == 0.0) {
            return BydaoValue();
        }
        return BydaoValue::fromReal(m_value / otherReal->m_value);
    }
    case TYPE_STRING: {
        bool ok;
        double val = other.toString().toDouble(&ok);
        if ( val == 0.0) {
            return BydaoValue();
        }
        return BydaoValue::fromReal(m_value / val);
    }
    default:
        return BydaoValue::fromReal(m_value / other.toReal());
    }
}

BydaoValue BydaoReal::neg() {
    return BydaoValue( BydaoReal::create(-m_value) );
}

BydaoValue BydaoReal::eq(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromBool(m_value == otherInt->value());
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromBool(m_value == otherReal->m_value);
    }
    default:
        return BydaoValue::fromBool(m_value == other.toInt());
    }
}

BydaoValue BydaoReal::lt(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromBool(m_value < otherInt->value());
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromBool(m_value < otherReal->m_value);
    }
    default:
        return BydaoValue::fromBool(m_value < other.toReal());
    }
}

BydaoValue BydaoReal::le(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromBool(m_value <= otherInt->value());
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromBool(m_value <= otherReal->m_value);
    }
    default:
        return BydaoValue::fromBool(m_value <= other.toReal());
    }
}

BydaoValue BydaoReal::gt(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromBool(m_value > otherInt->value());
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromBool(m_value > otherReal->m_value);
    }
    default:
        return BydaoValue::fromBool(m_value > other.toReal());
    }
}

BydaoValue BydaoReal::ge(const BydaoValue& other) {
    switch (other.typeId()) {
    case TYPE_INT: {
        const auto* otherInt = static_cast<const BydaoInt*>(other.toObject());
        return BydaoValue::fromBool(m_value >= otherInt->value());
    }
    case TYPE_REAL: {
        const auto* otherReal = static_cast<const BydaoReal*>(other.toObject());
        return BydaoValue::fromBool(m_value >= otherReal->m_value);
    }
    default:
        return BydaoValue::fromBool(m_value >= other.toReal());
    }
}

} // namespace BydaoScript
