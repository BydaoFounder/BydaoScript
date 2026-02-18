#pragma once

#include "BydaoObject.h"
#include "BydaoValue.h"
#include <QObject>
#include <QHash>
#include <QSet>
#include <functional>

namespace BydaoScript {

class BydaoNative : public QObject, public BydaoObject {
    Q_OBJECT

public:
    explicit BydaoNative(QObject* parent = nullptr);
    virtual ~BydaoNative();

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) final;

    virtual bool getProperty(const QString& name, BydaoValue& result);
    virtual bool setProperty(const QString& name, const BydaoValue& value);

protected:
    using NativeMethod = std::function<bool(const QVector<BydaoValue>&, BydaoValue&)>;

    void registerMethod(const QString& name, NativeMethod method);
    void registerProperty(const QString& name);

    QHash<QString, NativeMethod> m_methods;
    QSet<QString> m_properties;
};

} // namespace BydaoScript
