#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"
#include <cmath>

namespace BydaoScript {

BydaoReal::BydaoReal(double value) : m_value(value) {
    registerMethod("toString", [this](auto& args, auto& result) {
        return this->method_toString(args, result);
    });
    registerMethod("toInt", [this](auto& args, auto& result) {
        return this->method_toInt(args, result);
    });
    registerMethod("toBool", [this](auto& args, auto& result) {
        return this->method_toBool(args, result);
    });
    registerMethod("abs", [this](auto& args, auto& result) {
        return this->method_abs(args, result);
    });
    registerMethod("negate", [this](auto& args, auto& result) {
        return this->method_negate(args, result);
    });
    registerMethod("isNull", [this](auto& args, auto& result) {
        return this->method_isNull(args, result);
    });
    registerMethod("round", [this](auto& args, auto& result) {
        return this->method_round(args, result);
    });
    registerMethod("floor", [this](auto& args, auto& result) {
        return this->method_floor(args, result);
    });
    registerMethod("ceil", [this](auto& args, auto& result) {
        return this->method_ceil(args, result);
    });
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
