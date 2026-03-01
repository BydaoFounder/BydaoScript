#pragma once

#include "BydaoObject.h"
#include "BydaoValue.h"
#include <QObject>
#include <QHash>
#include <QSet>
#include <functional>

namespace BydaoScript {

struct BydaoPropertyInfo {
    enum Access { ReadOnly, ReadWrite };
    enum Visibility { Public, Internal };

    Access access = ReadWrite;
    Visibility visibility = Public;
    QString doc;

    BydaoPropertyInfo() = default;
    BydaoPropertyInfo(Access a, Visibility v = Public)
        : access(a), visibility(v) {
    }
};

class BydaoNative : public QObject, public BydaoObject {
    Q_OBJECT

public:
    explicit BydaoNative(QObject* parent = nullptr);
    virtual ~BydaoNative();

    // ========== Методы ==========

    // Абстрактный метод — каждый класс реализует сам
    virtual bool callMethod(const QString& name,
                            const QVector<BydaoValue>& args,
                            BydaoValue& result) = 0;

    // Получить итератор
    virtual BydaoValue iter() override;

    // ========== Свойства ==========

    void registerProperty(const QString& name,
                          std::function<BydaoValue()> getter,
                          std::function<bool(const BydaoValue&)> setter = nullptr,
                          const BydaoPropertyInfo& info = BydaoPropertyInfo());

    virtual bool hasProperty(const QString& name) const;
    virtual bool canGetProperty(const QString& name) const;
    virtual bool canSetProperty(const QString& name) const;
    virtual BydaoValue getProperty(const QString& name);
    virtual bool setProperty(const QString& name, const BydaoValue& value);

protected:

    struct PropertyEntry {
        BydaoPropertyInfo info;
        std::function<BydaoValue()> getter;
        std::function<bool(const BydaoValue&)> setter;
    };

    QHash<QString, PropertyEntry> m_properties;
};

} // namespace BydaoScript
