#include "BydaoScript/BydaoIterator.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoNull.h"

namespace BydaoScript {

BydaoIterator::BydaoIterator(QObject* parent)
    : BydaoNative(parent)
{
    registerMethod("next", &BydaoIterator::method_next);
    registerMethod("isValid", &BydaoIterator::method_isValid);
    registerMethod("key", &BydaoIterator::method_key);
    registerMethod("value", &BydaoIterator::method_value);

    // Регистрируем свойства для доступа через точку
    registerProperty("key",
                     [this]() { return this->getKey(); },
                     nullptr,
                     BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));

    registerProperty("value",
                     [this]() { return this->getValue(); },
                     nullptr,
                     BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));
}

BydaoIterator::~BydaoIterator() {}

void BydaoIterator::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoIterator::callMethod(const QString& name,
                               const QVector<BydaoValue>& args,
                               BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

bool BydaoIterator::method_next(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoBool::create(next()));
    return true;
}

bool BydaoIterator::method_isValid(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(BydaoBool::create(isValid()));
    return true;
}

bool BydaoIterator::method_key(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = key();
    return true;
}

bool BydaoIterator::method_value(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = value();
    return true;
}

} // namespace BydaoScript