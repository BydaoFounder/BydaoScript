#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"
#include <cmath>

namespace BydaoScript {

BydaoInt::BydaoInt(int value) : m_value(value) {
    registerMethod("toString", [this](auto& args, auto& result) {
        return this->method_toString(args, result);
    });
    registerMethod("toReal", [this](auto& args, auto& result) {
        return this->method_toReal(args, result);
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
    registerMethod("toHex", [this](auto& args, auto& result) {
        return this->method_toHex(args, result);
    });
    registerMethod("toBin", [this](auto& args, auto& result) {
        return this->method_toBin(args, result);
    });
    registerMethod("isEven", [this](auto& args, auto& result) {
        return this->method_isEven(args, result);
    });
    registerMethod("isOdd", [this](auto& args, auto& result) {
        return this->method_isOdd(args, result);
    });
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
    result = BydaoValue(new BydaoInt(std::abs(m_value)));
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

bool BydaoInt::method_toBin(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(QString::number(m_value, 2)));
    return true;
}

bool BydaoInt::method_isEven(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(m_value % 2 == 0));
    return true;
}

bool BydaoInt::method_isOdd(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(m_value % 2 != 0));
    return true;
}

} // namespace BydaoScript
