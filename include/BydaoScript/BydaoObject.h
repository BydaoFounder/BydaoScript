#pragma once

#include <QString>
#include <QVector>
#include <QDebug>
#include "BydaoValue.h"

namespace BydaoScript {

class BydaoObject {

public:
    virtual ~BydaoObject() = default;

    void ref() { m_refCount++; }
    void unref() { if (--m_refCount == 0) release(); }

    // Единственный метод вызова
    virtual bool callMethod(const QString& name,
                            const QVector<BydaoValue>& args,
                            BydaoValue& result) = 0;

    // Информация о типе
    virtual QString typeName() const = 0;

    // ========== Операции (с реализацией по умолчанию) ==========

    virtual BydaoValue add(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "add not supported for" << typeName();
        return BydaoValue();  // null
    }

    virtual BydaoValue sub(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "sub not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue mul(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "mul not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue div(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "div not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue neg() {
        qWarning() << "neg not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue eq(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "eq not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue neq(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "neq not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue lt(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "lt not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue le(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "le not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue gt(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "gt not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue ge(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "ge not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue and_(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "and not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue or_(const BydaoValue& other) {
        Q_UNUSED( other );
        qWarning() << "or not supported for" << typeName();
        return BydaoValue();
    }

    virtual BydaoValue not_() {
        qWarning() << "not not supported for" << typeName();
        return BydaoValue();
    }

protected:

    std::atomic<int> m_refCount{0};

    virtual void release() {
        delete this;
    }
};

} // namespace BydaoScript
