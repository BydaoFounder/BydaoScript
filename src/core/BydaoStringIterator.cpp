#include "BydaoScript/BydaoStringIterator.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"

namespace BydaoScript {

BydaoStringIterator::BydaoStringIterator(BydaoString* str, QObject* parent)
    : BydaoIterator(parent)
    , m_string(str)
    , m_index(-1)
{
    if (m_string) {
        m_string->ref();
    }
}

BydaoStringIterator::~BydaoStringIterator() {
    if (m_string) {
        m_string->unref();
    }
}

bool BydaoStringIterator::next() {
    if (!m_string) return false;
    
    m_index++;
    return m_index < m_string->length();
}

bool BydaoStringIterator::isValid() const {
    return m_string && m_index >= 0 && m_index < m_string->length();
}

BydaoValue BydaoStringIterator::key() const {
    if (!isValid()) return BydaoValue(BydaoNull::instance());
    return BydaoValue::fromInt(m_index);
}

BydaoValue BydaoStringIterator::value() const {
    if (!isValid()) return BydaoValue(BydaoNull::instance());
    return BydaoValue::fromString(QString(m_string->value().at(m_index)));
}

} // namespace BydaoScript