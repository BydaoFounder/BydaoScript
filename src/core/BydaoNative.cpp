#include "BydaoScript/BydaoNative.h"

namespace BydaoScript {

BydaoNative::BydaoNative(QObject* parent)
    : QObject(parent)
{}

BydaoNative::~BydaoNative() {}

bool BydaoNative::callMethod(const QString& name,
                            const QVector<BydaoValue>& args,
                            BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (it.value())(args, result);
    }
    return false;
}

bool BydaoNative::getProperty(const QString& name, BydaoValue& result) {
    Q_UNUSED(result)
    if (m_properties.contains(name)) {
        // Дочерние классы должны переопределить
        return false;
    }
    return false;
}

bool BydaoNative::setProperty(const QString& name, const BydaoValue& value) {
    Q_UNUSED(value)
    if (m_properties.contains(name)) {
        // Дочерние классы должны переопределить
        return false;
    }
    return false;
}

void BydaoNative::registerMethod(const QString& name, NativeMethod method) {
    m_methods[name] = method;
}

void BydaoNative::registerProperty(const QString& name) {
    m_properties.insert(name);
}

} // namespace BydaoScript
