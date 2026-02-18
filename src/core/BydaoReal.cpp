#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"
#include <cmath>

namespace BydaoScript {

BydaoReal::BydaoReal(double value) : m_value(value) {
    registerMethod("toString", &BydaoReal::method_toString);
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
    result = BydaoValue(new BydaoString(QString::number(m_value)));
    return true;
}

bool BydaoReal::method_toInt(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoInt((int)m_value));
    return true;
}

bool BydaoReal::method_toBool(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(m_value != 0.0));
    return true;
}

bool BydaoReal::method_abs(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoReal(std::abs(m_value)));
    return true;
}

bool BydaoReal::method_negate(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoReal(-m_value));
    return true;
}

bool BydaoReal::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(false));
    return true;
}

bool BydaoReal::method_round(const QVector<BydaoValue>& args, BydaoValue& result) {
    int decimals = args.size() > 0 ? args[0].toInt() : 0;
    double multiplier = std::pow(10.0, decimals);
    double rounded = std::round(m_value * multiplier) / multiplier;
    result = BydaoValue(new BydaoReal(rounded));
    return true;
}

bool BydaoReal::method_floor(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoReal(std::floor(m_value)));
    return true;
}

bool BydaoReal::method_ceil(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoReal(std::ceil(m_value)));
    return true;
}

} // namespace BydaoScript
