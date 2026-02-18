#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoArray.h"
#include "BydaoScript/BydaoNull.h"
#ifdef QT_CRYPTOGRAPHICHASH_LIB
#include <QCryptographicHash>
#endif

namespace BydaoScript {

BydaoString::BydaoString(const QString& value, QObject* parent)
    : BydaoNative(parent)
    , m_value(value)
{
    registerMethod("toString", [this](auto& args, auto& result) {
        return this->method_toString(args, result);
    });
    registerMethod("isNull", [this](auto& args, auto& result) {
        return this->method_isNull(args, result);
    });
    registerMethod("length", [this](auto& args, auto& result) {
        return this->method_length(args, result);
    });
    registerMethod("upper", [this](auto& args, auto& result) {
        return this->method_upper(args, result);
    });
    registerMethod("lower", [this](auto& args, auto& result) {
        return this->method_lower(args, result);
    });
    registerMethod("trim", [this](auto& args, auto& result) {
        return this->method_trim(args, result);
    });
    registerMethod("substring", [this](auto& args, auto& result) {
        return this->method_substring(args, result);
    });
    registerMethod("indexOf", [this](auto& args, auto& result) {
        return this->method_indexOf(args, result);
    });
    registerMethod("contains", [this](auto& args, auto& result) {
        return this->method_contains(args, result);
    });
    registerMethod("startsWith", [this](auto& args, auto& result) {
        return this->method_startsWith(args, result);
    });
    registerMethod("endsWith", [this](auto& args, auto& result) {
        return this->method_endsWith(args, result);
    });
    registerMethod("split", [this](auto& args, auto& result) {
        return this->method_split(args, result);
    });
    registerMethod("replace", [this](auto& args, auto& result) {
        return this->method_replace(args, result);
    });
    registerMethod("toInt", [this](auto& args, auto& result) {
        return this->method_toInt(args, result);
    });
    registerMethod("toReal", [this](auto& args, auto& result) {
        return this->method_toReal(args, result);
    });
#ifdef QT_CRYPTOGRAPHICHASH_LIB
    registerMethod("md5", [this](auto& args, auto& result) {
        return this->method_md5(args, result);
    });
#endif

    registerProperty("length");
}

bool BydaoString::getProperty(const QString& name, BydaoValue& result) {
    if (name == "length") {
        result = BydaoValue(new BydaoInt(m_value.length()));
        return true;
    }
    return BydaoNative::getProperty(name, result);
}

// ========== Реализации методов ==========

bool BydaoString::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(m_value));
    return true;
}

bool BydaoString::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoBool(m_value.isNull()));
    return true;
}

bool BydaoString::method_length(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoInt(m_value.length()));
    return true;
}

bool BydaoString::method_upper(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(m_value.toUpper()));
    return true;
}

bool BydaoString::method_lower(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(m_value.toLower()));
    return true;
}

bool BydaoString::method_trim(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue(new BydaoString(m_value.trimmed()));
    return true;
}

bool BydaoString::method_substring(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    int start = args[0].toInt();
    int end = (args.size() == 2) ? args[1].toInt() : m_value.length();

    if (start < 0 || start > m_value.length() || end < start || end > m_value.length()) {
        return false;
    }

    result = BydaoValue(new BydaoString(m_value.mid(start, end - start)));
    return true;
}

bool BydaoString::method_indexOf(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() < 1 || args.size() > 2) return false;

    QString sub = args[0].toString();
    int from = (args.size() == 2) ? args[1].toInt() : 0;

    int idx = m_value.indexOf(sub, from);
    result = BydaoValue(new BydaoInt(idx));
    return true;
}

bool BydaoString::method_contains(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sub = args[0].toString();
    result = BydaoValue(new BydaoBool(m_value.contains(sub)));
    return true;
}

bool BydaoString::method_startsWith(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sub = args[0].toString();
    result = BydaoValue(new BydaoBool(m_value.startsWith(sub)));
    return true;
}

bool BydaoString::method_endsWith(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sub = args[0].toString();
    result = BydaoValue(new BydaoBool(m_value.endsWith(sub)));
    return true;
}

bool BydaoString::method_split(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 1) return false;

    QString sep = args[0].toString();
    QStringList parts = m_value.split(sep);

    auto* array = new BydaoArray();
    for (const QString& part : parts) {
        array->append(BydaoValue(new BydaoString(part)));
    }

    result = BydaoValue(array);
    return true;
}

bool BydaoString::method_replace(const QVector<BydaoValue>& args, BydaoValue& result) {
    if (args.size() != 2) return false;

    QString before = args[0].toString();
    QString after = args[1].toString();

    result = BydaoValue(new BydaoString(m_value.replace(before, after)));
    return true;
}

bool BydaoString::method_toInt(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    bool ok = false;
    int val = m_value.toInt(&ok);
    if (ok) {
        result = BydaoValue(new BydaoInt(val));
        return true;
    }
    return false;
}

bool BydaoString::method_toReal(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    bool ok = false;
    double val = m_value.toDouble(&ok);
    if (ok) {
        result = BydaoValue(new BydaoReal(val));
        return true;
    }
    return false;
}

#ifdef QT_CRYPTOGRAPHICHASH_LIB
bool BydaoString::method_md5(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    QByteArray hash = QCryptographicHash::hash(m_value.toUtf8(), QCryptographicHash::Md5);
    result = BydaoValue(new BydaoString(QString::fromLatin1(hash.toHex())));
    return true;
}
#endif

} // namespace BydaoScript
