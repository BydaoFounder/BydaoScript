#include "BydaoScript/BydaoNull.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"

namespace BydaoScript {

BydaoNull* BydaoNull::s_instance = nullptr;

BydaoNull::BydaoNull() {
    registerMethod("toString", [this](auto& args, auto& result) {
        return this->method_toString(args, result);
    });
    registerMethod("toBool", [this](auto& args, auto& result) {
        return this->method_toBool(args, result);
    });
    registerMethod("isNull", [this](auto& args, auto& result) {
        return this->method_isNull(args, result);
    });
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
