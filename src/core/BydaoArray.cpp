#include "BydaoScript/BydaoArray.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"

namespace BydaoScript {

BydaoArray::BydaoArray(QObject* parent)
    : BydaoNative(parent)
{
    // Регистрация методов (без макросов)
    registerMethod("toString", &BydaoArray::method_toString);
    registerMethod("length", &BydaoArray::method_length);
    registerMethod("get", &BydaoArray::method_get);
    registerMethod("set", &BydaoArray::method_set);
    registerMethod("push", &BydaoArray::method_push);
    registerMethod("pop", &BydaoArray::method_pop);
    registerMethod("shift", &BydaoArray::method_shift);
    registerMethod("unshift", &BydaoArray::method_unshift);
    registerMethod("slice", &BydaoArray::method_slice);
    registerMethod("join", &BydaoArray::method_join);

    // Регистрация свойства length (ReadOnly)
    // Свойства
    registerProperty("length",
                     [this]() { return BydaoValue::fromInt(m_elements.size()); },
                     nullptr,
                     BydaoPropertyInfo(BydaoPropertyInfo::ReadOnly));
}

void BydaoArray::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoArray::callMethod(const QString& name,
                           const QVector<BydaoValue>& args,
                           BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

// ========== Реализация методов массива ==========

BydaoValue BydaoArray::at(qint64 index) const {
    if (index >= 0 && index < m_elements.size()) {
        return m_elements[index];
    }
    return BydaoValue(BydaoNull::instance());
}

void BydaoArray::set(qint64 index, const BydaoValue& value) {
    if (index >= 0) {
        if (index >= m_elements.size()) {
            m_elements.resize(index + 1);
        }
        m_elements[index] = value;
    }
}

BydaoValue  BydaoArray::get(const BydaoValue& index) {
    int idx = index.toInt();
    if (0 <= idx && idx < m_elements.size()) {
        return m_elements[idx];
    }
    return BydaoValue(BydaoNull::instance());
}

void BydaoArray::append(const BydaoValue& value) {
    m_elements.append(value);
}

void BydaoArray::insert(qint64 index, const BydaoValue& value) {
    if (index >= 0 && index <= m_elements.size()) {
        m_elements.insert(index, value);
    }
}

void BydaoArray::removeAt(qint64 index) {
    if (index >= 0 && index < m_elements.size()) {
        m_elements.removeAt(index);
    }
}

void BydaoArray::clear() {
    m_elements.clear();
}

// ========== Методы ==========

bool BydaoArray::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    QStringList parts;
    for (const auto& elem : m_elements) {
        parts << elem.toString();
    }
    result = BydaoValue::fromString("[" + parts.join(", ") + "]");
    return true;
}

bool BydaoArray::method_length(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue::fromInt(m_elements.size());
    return true;
}

bool BydaoArray::method_get(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    int index = args[0].toInt();
    result = at(index);
    return true;
}

bool BydaoArray::method_set(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;
    set(args[0].toInt(), args[1]);
    result = BydaoValue(BydaoNull::instance());
    return true;
}

bool BydaoArray::method_push(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    append(args[0]);
    result = BydaoValue::fromInt(m_elements.size());
    return true;
}

bool BydaoArray::method_pop(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    if (m_elements.isEmpty()) {
        result = BydaoValue(BydaoNull::instance());
    } else {
        result = m_elements.takeLast();
    }
    return true;
}

bool BydaoArray::method_shift(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    if (m_elements.isEmpty()) {
        result = BydaoValue(BydaoNull::instance());
    } else {
        result = m_elements.takeFirst();
    }
    return true;
}

bool BydaoArray::method_unshift(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    m_elements.prepend(args[0]);
    result = BydaoValue::fromInt(m_elements.size());
    return true;
}

bool BydaoArray::method_slice(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    qint64 start = args[0].toInt();
    qint64 end = (args.size() == 2) ? args[1].toInt() : m_elements.size();

    if (start < 0) start = 0;
    if (end > m_elements.size()) end = m_elements.size();
    if (start >= end) {
        result = BydaoValue(new BydaoArray());
        return true;
    }

    auto* newArray = new BydaoArray();
    for (qint64 i = start; i < end; i++) {
        newArray->append(m_elements[i]);
    }
    result = BydaoValue(newArray);
    return true;
}

bool BydaoArray::method_join(const QVector<BydaoValue>& args, BydaoValue& result) {
    QString sep = args.size() > 0 ? args[0].toString() : ",";

    QStringList parts;
    for (const auto& elem : m_elements) {
        parts << elem.toString();
    }
    result = BydaoValue::fromString(parts.join(sep));
    return true;
}

} // namespace BydaoScript
