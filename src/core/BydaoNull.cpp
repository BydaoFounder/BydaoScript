#include "BydaoScript/BydaoNull.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"

namespace BydaoScript {

BydaoNull* BydaoNull::s_instance = nullptr;

BydaoNull::BydaoNull() {
    registerMethod("toString", &BydaoNull::method_toString);
    registerMethod("toBool", &BydaoNull::method_toBool);
    registerMethod("isNull", &BydaoNull::method_isNull);
}

void BydaoNull::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoNull::callMethod(const QString& name,
                           const QVector<BydaoValue>& args,
                           BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

BydaoNull* BydaoNull::instance() {
    if (!s_instance) {
        s_instance = new BydaoNull();
    }
    return s_instance;
}

bool BydaoNull::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString("null"));
    return true;
}

bool BydaoNull::method_toBool(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(false));
    return true;
}

bool BydaoNull::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(true));
    return true;
}

} // namespace BydaoScript
