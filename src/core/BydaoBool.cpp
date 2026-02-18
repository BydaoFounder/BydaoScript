#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"

namespace BydaoScript {

BydaoBool::BydaoBool(bool value, QObject* parent)
    : BydaoNative(parent)
    , m_value(value)
{
    registerMethod("toString", [this](auto& args, auto& result) {
        return this->method_toString(args, result);
    });
    registerMethod("toInt", [this](auto& args, auto& result) {
        return this->method_toInt(args, result);
    });
    registerMethod("toReal", [this](auto& args, auto& result) {
        return this->method_toReal(args, result);
    });
    registerMethod("negate", [this](auto& args, auto& result) {
        return this->method_negate(args, result);
    });
    registerMethod("isNull", [this](auto& args, auto& result) {
        return this->method_isNull(args, result);
    });
}

bool BydaoBool::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(m_value ? "true" : "false"));
    return true;
}

bool BydaoBool::method_toInt(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoInt(m_value ? 1 : 0));
    return true;
}

bool BydaoBool::method_toReal(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoReal(m_value ? 1.0 : 0.0));
    return true;
}

bool BydaoBool::method_negate(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(!m_value));
    return true;
}

bool BydaoBool::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(false));
    return true;
}

} // namespace BydaoScript
