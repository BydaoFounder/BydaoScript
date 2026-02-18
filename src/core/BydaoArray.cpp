#include "BydaoScript/BydaoArray.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"

namespace BydaoScript {

BydaoArray::BydaoArray() {
    registerMethod("toString", [this](auto& args, auto& result) {
        return this->method_toString(args, result);
    });
    registerMethod("isNull", [this](auto& args, auto& result) {
        return this->method_isNull(args, result);
    });
    registerMethod("length", [this](auto& args, auto& result) {
        return this->method_length(args, result);
    });
    registerMethod("get", [this](auto& args, auto& result) {
        return this->method_get(args, result);
    });
    registerMethod("set", [this](auto& args, auto& result) {
        return this->method_set(args, result);
    });
    registerMethod("push", [this](auto& args, auto& result) {
        return this->method_push(args, result);
    });
    registerMethod("pop", [this](auto& args, auto& result) {
        return this->method_pop(args, result);
    });
    registerMethod("shift", [this](auto& args, auto& result) {
        return this->method_shift(args, result);
    });
    registerMethod("unshift", [this](auto& args, auto& result) {
        return this->method_unshift(args, result);
    });
    registerMethod("slice", [this](auto& args, auto& result) {
        return this->method_slice(args, result);
    });
    registerMethod("join", [this](auto& args, auto& result) {
        return this->method_join(args, result);
    });
    registerMethod("map", [this](auto& args, auto& result) {
        return this->method_map(args, result);
    });
    registerMethod("filter", [this](auto& args, auto& result) {
        return this->method_filter(args, result);
    });
}

int BydaoArray::length() const {
    return m_elements.size();
}

BydaoValue BydaoArray::at(int index) const {
    if (index >= 0 && index < m_elements.size()) {
        return m_elements[index];
    }
    return BydaoValue(BydaoNull::instance());
}

void BydaoArray::set(int index, const BydaoValue& value) {
    if (index >= 0) {
        if (index >= m_elements.size()) {
            m_elements.resize(index + 1);
        }
        m_elements[index] = value;
    }
}

void BydaoArray::append(const BydaoValue& value) {
    m_elements.append(value);
}

void BydaoArray::insert(int index, const BydaoValue& value) {
    if (index >= 0 && index <= m_elements.size()) {
        m_elements.insert(index, value);
    }
}

void BydaoArray::removeAt(int index) {
    if (index >= 0 && index < m_elements.size()) {
        m_elements.removeAt(index);
    }
}

void BydaoArray::clear() {
    m_elements.clear();
}

bool BydaoArray::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    QStringList parts;
    for (const auto& elem : m_elements) {
        parts << elem.toString();
    }
    result = BydaoValue(new BydaoString("[" + parts.join(", ") + "]"));
    return true;
}

bool BydaoArray::method_length(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoInt(m_elements.size()));
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
    int index = args[0].toInt();
    set(index, args[1]);
    result = BydaoValue(BydaoNull::instance());
    return true;
}

bool BydaoArray::method_push(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;
    append(args[0]);
    result = BydaoValue(new BydaoInt(m_elements.size()));
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
    result = BydaoValue(new BydaoInt(m_elements.size()));
    return true;
}

bool BydaoArray::method_slice(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    int start = args[0].toInt();
    int end = (args.size() == 2) ? args[1].toInt() : m_elements.size();

    if (start < 0) start = 0;
    if (end > m_elements.size()) end = m_elements.size();
    if (start >= end) {
        result = BydaoValue(new BydaoArray());
        return true;
    }

    auto* newArray = new BydaoArray();
    for (int i = start; i < end; i++) {
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
    result = BydaoValue(new BydaoString(parts.join(sep)));
    return true;
}

bool BydaoArray::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(false));
    return true;
}

bool BydaoArray::method_map(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    // TODO: реализовать map с функцией
    result = BydaoValue(new BydaoArray());
    return true;
}

bool BydaoArray::method_filter(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    // TODO: реализовать filter с функцией
    result = BydaoValue(new BydaoArray());
    return true;
}

} // namespace BydaoScript
