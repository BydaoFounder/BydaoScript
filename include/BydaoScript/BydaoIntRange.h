#pragma once

#include "BydaoIterator.h"

namespace BydaoScript {

class BydaoIntRange : public BydaoNative {
    Q_OBJECT

public:
    BydaoIntRange(qint64 start, qint64 end, QObject* parent = nullptr);
    virtual ~BydaoIntRange() = default;

    QString typeName() const override { return "IntRange"; }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

    // Метод, возвращающий итератор
    BydaoValue iter();

    qint64 start() { return m_start; }
    qint64 end() { return m_end; }

private:

    bool method_iter(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoIntRange::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    qint64 m_start;
    qint64 m_end;
};

class BydaoIntRangeIterator : public BydaoIterator {
    Q_OBJECT
public:
    BydaoIntRangeIterator( BydaoIntRange* range );

    bool next() override;
    bool isValid() const override;
    BydaoValue key() const override;
    BydaoValue value() const override;

private:

    qint64 m_start;
    qint64 m_end;
    qint64 m_current;
};

} // namespace BydaoScript
