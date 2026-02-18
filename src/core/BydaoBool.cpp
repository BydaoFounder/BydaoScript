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
    registerMethod("toString", &BydaoBool::method_toString);
    registerMethod("toInt",  &BydaoBool::method_toInt);
    registerMethod("toReal", &BydaoBool::method_toReal);
    registerMethod("toBool", &BydaoBool::method_toBool);
    registerMethod("negate", &BydaoBool::method_negate);
    registerMethod("isNull", &BydaoBool::method_isNull);
}

void BydaoBool::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoBool::callMethod(const QString& name,
                          const QVector<BydaoValue>& args,
                          BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
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

bool BydaoBool::method_toBool(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( new BydaoBool(m_value) );
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
