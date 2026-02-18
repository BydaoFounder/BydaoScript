#pragma once

#include "BydaoNative.h"
#include <QString>

namespace BydaoScript {

class BydaoString : public BydaoNative {
public:
    explicit BydaoString(const QString& value = QString(), QObject* parent = nullptr);
    
    QString typeName() const override { return "String"; }
    const QString& value() const { return m_value; }
    int length() const { return m_value.length(); }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

    // Операции
    BydaoValue add(const BydaoValue& other) override;   // сложение (конкатенация) строк
    BydaoValue eq(const BydaoValue& other) override;   // сравнение на равенство
    BydaoValue ne(const BydaoValue& other) override;   // сравнение на равенство
    BydaoValue lt(const BydaoValue& other) override;   // лексикографическое сравнение
    BydaoValue le(const BydaoValue& other) override;   // лексикографическое сравнение
    BydaoValue gt(const BydaoValue& other) override;   // лексикографическое сравнение
    BydaoValue ge(const BydaoValue& other) override;   // лексикографическое сравнение

private:

    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_length(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_upper(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_lower(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_trim(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_substring(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_indexOf(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_contains(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_startsWith(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_endsWith(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_replace(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_split(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toInt(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toReal(const QVector<BydaoValue>& args, BydaoValue& result);
#ifdef QT_CRYPTOGRAPHICHASH_LIB
    bool method_md5(const QVector<BydaoValue>& args, BydaoValue& result);
#endif

    using MethodPtr = bool (BydaoString::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    QString m_value;
};

} // namespace BydaoScript
