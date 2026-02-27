#include "BydaoScript/BydaoIntRange.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoNull.h"

namespace BydaoScript {

BydaoIntRange::BydaoIntRange(qint64 start, qint64 end, QObject* parent)
    : BydaoNative(parent)
    , m_start(start)
    , m_end(end)
{
    registerMethod("iter", &BydaoIntRange::method_iter);
}

void BydaoIntRange::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoIntRange::callMethod(const QString& name,
                             const QVector<BydaoValue>& args,
                             BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

BydaoValue BydaoIntRange::iter() {
    return BydaoValue(new BydaoIntRangeIterator(this));
}

// BydaoIntRange::method_iter()
bool BydaoIntRange::method_iter(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = iter();
    return true;
}

//=============================================================================


BydaoIntRangeIterator::BydaoIntRangeIterator( BydaoIntRange* range ) {
    m_start = range->start();
    m_end = range->end();
    m_current = m_start - 1;
}

bool BydaoIntRangeIterator::next() {
    m_current++;
    return m_current < m_end;
}

bool BydaoIntRangeIterator::isValid() const {
    return m_current >= m_start && m_current < m_end;
}

BydaoValue BydaoIntRangeIterator::key() const {
    if (!isValid()) return BydaoValue(BydaoNull::instance());
    return BydaoValue::fromInt(m_current - m_start);  // ключ = индекс от 0
}

BydaoValue BydaoIntRangeIterator::value() const {
    if (!isValid()) return BydaoValue(BydaoNull::instance());
    return BydaoValue::fromInt(m_current);
}

} // namespace BydaoScript
