#include "BydaoScript/BydaoValue.h"
#include "BydaoScript/BydaoObject.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"

namespace BydaoScript {

BydaoValue::BydaoValue() : m_obj(nullptr) {}

BydaoValue::BydaoValue(BydaoObject* obj) : m_obj(obj) {}

BydaoValue::BydaoValue(const BydaoValue& other) : m_obj(other.m_obj) {}

BydaoValue::~BydaoValue() {}

BydaoValue& BydaoValue::operator=(const BydaoValue& other) {
    if (this != &other) {
        m_obj = other.m_obj;
    }
    return *this;
}

QString BydaoValue::toString() const {
    if (!m_obj) return "null";

    BydaoValue result;
    if (m_obj->callMethod("toString", {}, result)) {
        if (result.isObject()) {
            if (auto* str = dynamic_cast<BydaoString*>(result.toObject())) {
                return str->value();
            }
        }
    }
    return "???";
}

bool BydaoValue::toBool() const {
    if (!m_obj) return false;

    BydaoValue result;
    if (m_obj->callMethod("toBool", {}, result)) {
        if (result.isObject()) {
            if (auto* b = dynamic_cast<BydaoBool*>(result.toObject())) {
                return b->value();
            }
        }
    }
    return false;
}

int BydaoValue::toInt() const {
    if (!m_obj) return 0;

    BydaoValue result;
    if (m_obj->callMethod("toInt", {}, result)) {
        if (result.isObject()) {
            if (auto* i = dynamic_cast<BydaoInt*>(result.toObject())) {
                return i->value();
            }
            if (auto* b = dynamic_cast<BydaoBool*>(result.toObject())) {
                return b->value() ? 1 : 0;
            }
            if (auto* r = dynamic_cast<BydaoReal*>(result.toObject())) {
                return (int)r->value();
            }
            if (auto* s = dynamic_cast<BydaoString*>(result.toObject())) {
                return s->value().toInt();
            }
        }
    }
    return 0;
}

double BydaoValue::toReal() const {
    if (!m_obj) return 0.0;

    BydaoValue result;
    if (m_obj->callMethod("toReal", {}, result)) {
        if (result.isObject()) {
            if (auto* r = dynamic_cast<BydaoReal*>(result.toObject())) {
                return r->value();
            }
            if (auto* i = dynamic_cast<BydaoInt*>(result.toObject())) {
                return (double)i->value();
            }
            if (auto* b = dynamic_cast<BydaoBool*>(result.toObject())) {
                return b->value() ? 1.0 : 0.0;
            }
            if (auto* s = dynamic_cast<BydaoString*>(result.toObject())) {
                return s->value().toDouble();
            }
        }
    }
    return 0.0;
}

bool BydaoValue::isNull() const {
    if (!m_obj) return true;

    BydaoValue result;
    if (m_obj->callMethod("isNull", {}, result)) {
        if (result.isObject()) {
            if (auto* b = dynamic_cast<BydaoBool*>(result.toObject())) {
                return b->value();
            }
        }
    }
    return false;
}

} // namespace BydaoScript
