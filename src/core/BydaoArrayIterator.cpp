#include "BydaoScript/BydaoArrayIterator.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoNull.h"

namespace BydaoScript {

BydaoArrayIterator::BydaoArrayIterator(BydaoArray* array, QObject* parent)
    : BydaoIterator(parent)
    , m_array(array)
    , m_index(-1)
{
    if (m_array) {
        m_array->ref();
    }
}

BydaoArrayIterator::~BydaoArrayIterator() {
    if (m_array) {
        m_array->unref();
    }
}

bool BydaoArrayIterator::next() {
    if (!m_array) return false;
    
    m_index++;
    return m_index < m_array->size();
}

bool BydaoArrayIterator::isValid() const {
    return m_array && m_index >= 0 && m_index < m_array->size();
}

BydaoValue BydaoArrayIterator::key() const {
    if (!isValid()) return BydaoValue(BydaoNull::instance());
    return BydaoValue::fromInt(m_index);
}

BydaoValue BydaoArrayIterator::value() const {
    if (!isValid()) {
//        qDebug() << "Iterator not valid, index:" << m_index;
        return BydaoValue(BydaoNull::instance());
    }
    BydaoValue val = m_array->at(m_index);
//    qDebug() << "Iterator value at index" << m_index << ":" << val.toString() << "type:" << val.typeId();
    return val;
}

} // namespace BydaoScript
