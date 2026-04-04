// Copyright 2026 Oleh Horshkov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "BydaoScript/BydaoConstantFolder.h"
#include "BydaoScript/BydaoParser.h"
#include "BydaoScript/BydaoValue.h"
#include "BydaoScript/BydaoObject.h"
#include "BydaoScript/BydaoInt.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoNull.h"

namespace BydaoScript {

// ========== Конструктор ==========

BydaoConstantFolder::BydaoConstantFolder(BydaoParser* parser)
    : m_parser(parser)
    , m_savedPos(0)
{}

// ========== Вспомогательные методы ==========

void BydaoConstantFolder::savePos() {
    m_savedPos = m_parser->m_pos;
    m_savedToken = m_parser->m_current;
}

void BydaoConstantFolder::restorePos() {
    m_parser->m_pos = m_savedPos;
    m_parser->m_current = m_savedToken;
}

int BydaoConstantFolder::getPos() const {
    return m_parser->m_pos;
}

// ========== Основные методы ==========

bool BydaoConstantFolder::isConstantExpression() {
    savePos();
    bool result = isConstantLogicalOr();
    restorePos();
    return result;
}

BydaoValue* BydaoConstantFolder::evaluate() {
    savePos();
    BydaoValue* result = evaluateLogicalOr();

    // Если после выражения остались непрочитанные токены - это ошибка
    if (m_parser->match(BydaoTokenType::Plus) ||
        m_parser->match(BydaoTokenType::Minus) ||
        m_parser->match(BydaoTokenType::Mul) ||
        m_parser->match(BydaoTokenType::Div) ||
        m_parser->match(BydaoTokenType::Mod) ||
        m_parser->match(BydaoTokenType::LParen)) {
        restorePos();
        return BydaoValue::get();  // null
    }

    return result;
}

// ===== Logical OR (||) =====

bool BydaoConstantFolder::isConstantLogicalOr() {
//    qDebug() << "isConstantLogicalOr: start";
    if (!isConstantLogicalAnd()) return false;

    while (m_parser->match(BydaoTokenType::Or)) {
        m_parser->nextToken();
        if (!isConstantLogicalAnd()) return false;
    }
    return true;
}

BydaoValue* BydaoConstantFolder::evaluateLogicalOr() {
    BydaoValue* result = evaluateLogicalAnd();

    while (m_parser->match(BydaoTokenType::Or)) {
        m_parser->nextToken();
        BydaoValue* right = evaluateLogicalAnd();

        bool boolResult = result->toBool() || right->toBool();
        result->set( BydaoBool::create( boolResult ) );
    }
    return result;
}

// ===== Logical AND (&&) =====

bool BydaoConstantFolder::isConstantLogicalAnd() {
//    qDebug() << "isConstantLogicalAnd: start";
    if (!isConstantEquality()) return false;

    while (m_parser->match(BydaoTokenType::And)) {
        m_parser->nextToken();
        if (!isConstantEquality()) return false;
    }
    return true;
}

BydaoValue* BydaoConstantFolder::evaluateLogicalAnd() {
    BydaoValue* result = evaluateEquality();

    while (m_parser->match(BydaoTokenType::And)) {
        m_parser->nextToken();
        BydaoValue* right = evaluateEquality();

        bool boolResult = result->toBool() && right->toBool();
        result->set( BydaoBool::create( boolResult ) );
    }
    return result;
}

// ===== Equality (==, !=) =====

bool BydaoConstantFolder::isConstantEquality() {
//    qDebug() << "isConstantEquality: start";
    if (!isConstantComparison()) return false;

    while (m_parser->match(BydaoTokenType::Equal) || 
           m_parser->match(BydaoTokenType::NotEqual)) {
        m_parser->nextToken();
        if (!isConstantComparison()) return false;
    }
    return true;
}

BydaoValue* BydaoConstantFolder::evaluateEquality() {
    BydaoValue* result = evaluateComparison();

    while (true) {
        if (m_parser->match(BydaoTokenType::Equal)) {
            m_parser->nextToken();
            BydaoValue* right = evaluateComparison();

            bool eq = false;
            if (result->isObject() && right->isObject()) {
                eq = result->toObject()->eq(right)->toBool();
            } else {
                eq = (result->toString() == right->toString());
            }
            result->set( BydaoBool::create( eq ) );
        }
        else if (m_parser->match(BydaoTokenType::NotEqual)) {
            m_parser->nextToken();
            BydaoValue* right = evaluateComparison();

            bool neq = true;
            if (result->isObject() && right->isObject()) {
                neq = result->toObject()->neq(right)->toBool();
            } else {
                neq = (result->toString() != right->toString());
            }
            result->set( BydaoBool::create( neq ) );
        }
        else {
            break;
        }
    }
    return result;
}

// ===== Comparison (<, >, <=, >=) =====

bool BydaoConstantFolder::isConstantComparison() {
//    qDebug() << "isConstantComparison: start";
    if (!isConstantAddition()) return false;

    while (m_parser->match(BydaoTokenType::Less) || 
           m_parser->match(BydaoTokenType::Greater) ||
           m_parser->match(BydaoTokenType::LessEqual) || 
           m_parser->match(BydaoTokenType::GreaterEqual)) {
        m_parser->nextToken();
        if (!isConstantAddition()) return false;
    }
    return true;
}

BydaoValue* BydaoConstantFolder::evaluateComparison() {
    BydaoValue* result = evaluateAddition();

    while (true) {
        if (m_parser->match(BydaoTokenType::Less)) {
            m_parser->nextToken();
            BydaoValue* right = evaluateAddition();

            bool lt = false;
            if (result->isObject() && right->isObject()) {
                lt = result->toObject()->lt(right)->toBool();
            } else {
                lt = (result->toReal() < right->toReal());
            }
            result->set( BydaoBool::create(lt) );
        }
        else if (m_parser->match(BydaoTokenType::Greater)) {
            m_parser->nextToken();
            BydaoValue* right = evaluateAddition();

            bool gt = false;
            if (result->isObject() && right->isObject()) {
                gt = result->toObject()->gt(right)->toBool();
            } else {
                gt = (result->toReal() > right->toReal());
            }
            result->set( BydaoBool::create(gt) );
        }
        else if (m_parser->match(BydaoTokenType::LessEqual)) {
            m_parser->nextToken();
            BydaoValue* right = evaluateAddition();

            bool le = false;
            if (result->isObject() && right->isObject()) {
                le = result->toObject()->le(right)->toBool();
            } else {
                le = (result->toReal() <= right->toReal());
            }
            result->set( BydaoBool::create(le) );
        }
        else if (m_parser->match(BydaoTokenType::GreaterEqual)) {
            m_parser->nextToken();
            BydaoValue* right = evaluateAddition();

            bool ge = false;
            if (result->isObject() && right->isObject()) {
                ge = result->toObject()->ge(right)->toBool();
            } else {
                ge = (result->toReal() >= right->toReal());
            }
            result->set( BydaoBool::create(ge) );
        }
        else {
            break;
        }
    }
    return result;
}

// ===== Addition/Subtraction (+, -) =====

bool BydaoConstantFolder::isConstantAddition() {
//    qDebug() << "isConstantAddition: start";
    if (!isConstantTerm()) return false;

    while (m_parser->match(BydaoTokenType::Plus) || 
           m_parser->match(BydaoTokenType::Minus)) {
        m_parser->nextToken();
        if (!isConstantTerm()) return false;
    }
    return true;
}

BydaoValue* BydaoConstantFolder::evaluateAddition() {
    BydaoValue* result = evaluateTerm();

    while (true) {
        if (m_parser->match(BydaoTokenType::Plus)) {
            m_parser->nextToken();
            BydaoValue* right = evaluateTerm();

            if (result->isObject() && right->isObject()) {
                result = result->toObject()->add(right);
            } else {
                // Fallback для простых типов
                double val = result->toReal() + right->toReal();
                result->set( BydaoReal::create(val) );
            }
        }
        else if (m_parser->match(BydaoTokenType::Minus)) {
            m_parser->nextToken();
            BydaoValue* right = evaluateTerm();

            if (result->isObject() && right->isObject()) {
                result = result->toObject()->sub(right);
            } else {
                double val = result->toReal() - right->toReal();
                result->set( BydaoReal::create(val) );
            }
        }
        else {
            break;
        }
    }
    return result;
}

// ===== Term (*, /, %) =====

bool BydaoConstantFolder::isConstantTerm() {
//    qDebug() << ">> isConstantTerm: starting at pos" << m_parser->m_pos;
    if (!isConstantUnary()) {
//        qDebug() << ">> isConstantTerm: isConstantUnary failed";
        return false;
    }

    while (m_parser->match(BydaoTokenType::Mul) ||
           m_parser->match(BydaoTokenType::Div) ||
           m_parser->match(BydaoTokenType::Mod)) {

//        qDebug() << ">> isConstantTerm: found operator" << m_parser->m_current.text << "at pos" << m_parser->m_pos;
        m_parser->nextToken();

        if (!isConstantUnary()) {
//            qDebug() << ">> isConstantTerm: isConstantUnary after operator failed";
            return false;
        }
    }
//    qDebug() << ">> isConstantTerm: success";
    return true;
}

BydaoValue* BydaoConstantFolder::evaluateTerm() {
//    qDebug() << "<< evaluateTerm: starting at pos" << m_parser->m_pos;
    BydaoValue* result = evaluateUnary();
//    qDebug() << "<< evaluateTerm: left =" << result.toString() << "at pos" << m_parser->m_pos;

    while (true) {
        if (m_parser->match(BydaoTokenType::Mul)) {
//            qDebug() << "<< evaluateTerm: found * at pos" << m_parser->m_pos;
            m_parser->nextToken();
            BydaoValue* right = evaluateUnary();
//            qDebug() << "<< evaluateTerm: right =" << right.toString();

            if (result->isObject() && right->isObject()) {
                result = result->toObject()->mul(right);
            } else {
                double val = result->toReal() * right->toReal();
                result->set( BydaoReal::create(val) );
            }
        }
        else if (m_parser->match(BydaoTokenType::Div)) {
//            qDebug() << "<< evaluateTerm: found / at pos" << m_parser->m_pos;
            m_parser->nextToken();
            BydaoValue* right = evaluateUnary();
//            qDebug() << "<< evaluateTerm: right =" << right.toString();

            if (result->isObject() && right->isObject()) {
                result = result->toObject()->div(right);
            } else {
                double val = result->toReal() / right->toReal();
                result->set( BydaoReal::create(val) );
            }
        }
        else if (m_parser->match(BydaoTokenType::Mod)) {
//            qDebug() << "<< evaluateTerm: found % at pos" << m_parser->m_pos;
            m_parser->nextToken();
            BydaoValue* right = evaluateUnary();
//            qDebug() << "<< evaluateTerm: right =" << right.toString();

            if (result->isObject() && right->isObject()) {
                result = result->toObject()->mod(right);
            } else {
                qint64 val = result->toInt() % right->toInt();
                result->set( BydaoInt::create(val) );
            }
        }
        else {
            break;
        }
    }
//    qDebug() << "<< evaluateTerm: final result =" << result.toString();
    return result;
}

// ===== Unary (!, -) =====

bool BydaoConstantFolder::isConstantUnary() {
    if (m_parser->match(BydaoTokenType::Not) || 
        m_parser->match(BydaoTokenType::Minus)) {
        m_parser->nextToken();
        return isConstantUnary();
    }
    return isConstantPrimary();
}

BydaoValue* BydaoConstantFolder::evaluateUnary() {
    if (m_parser->match(BydaoTokenType::Not)) {
        m_parser->nextToken();
        BydaoValue* val = evaluateUnary();
        return BydaoValue::get( BydaoBool::create( ! val->toBool() ) );
    }
    if (m_parser->match(BydaoTokenType::Minus)) {
        m_parser->nextToken();
        BydaoValue* val = evaluateUnary();
        if (val->isObject()) {
            return val->toObject()->neg();
        }
        return BydaoValue::get( BydaoReal::create( -val->toReal() ) );
    }
    return evaluatePrimary();
}

// ===== Primary (literals, constants, parentheses) =====

bool BydaoConstantFolder::isConstantPrimary() {
    // Литералы
    if (m_parser->match(BydaoTokenType::Number) ||
        m_parser->match(BydaoTokenType::String) ||
        m_parser->match(BydaoTokenType::True) ||
        m_parser->match(BydaoTokenType::False) ||
        m_parser->match(BydaoTokenType::Null)) {
//        qDebug() << "isConstantPrimary: literal" << m_parser->m_current.text;  // ОТЛАДКА
        return true;
    }

    // Идентификаторы (другие константы)
    if (m_parser->match(BydaoTokenType::Identifier)) {
        QString name = m_parser->m_current.text;
//        qDebug() << "isConstantPrimary: identifier" << name;  // ОТЛАДКА
        if (m_parser->isVariableDeclared(name)) {
            VariableInfo info = m_parser->resolveVariable(name);
//            qDebug() << "  declared, isConstant =" << info.isConstant;  // ОТЛАДКА
            if (info.isConstant) {
                return true;
            }
        } else {
//            qDebug() << "  NOT declared!";  // ОТЛАДКА
        }
        return false;
    }

    // Выражения в скобках
    if (m_parser->match(BydaoTokenType::LParen)) {
        m_parser->nextToken();
        bool result = isConstantLogicalOr();
        if (!result) return false;
        return m_parser->match(BydaoTokenType::RParen);
    }

    return false;
}

BydaoValue* BydaoConstantFolder::evaluatePrimary() {
    // Числа
    if (m_parser->match(BydaoTokenType::Number)) {
        QString text = m_parser->m_current.text;
        m_parser->nextToken();

//        qDebug() << "evaluatePrimary: number" << text;  // ОТЛАДКА

        if (text.contains('.') || text.contains('e') || text.contains('E')) {
            return BydaoValue::get( BydaoReal::create( text.toDouble() ) );
        }
        else {
            return BydaoValue::get( BydaoInt::create(text.toLongLong()) );
        }
    }

    // Строки
    if (m_parser->match(BydaoTokenType::String)) {
        QString text = m_parser->m_current.text;
        m_parser->nextToken();
        return BydaoValue::get( BydaoString::create(text) );
    }

    // Булевы значения
    if (m_parser->match(BydaoTokenType::True)) {
        m_parser->nextToken();
        return BydaoValue::get( BydaoBool::create(true) );
    }
    if (m_parser->match(BydaoTokenType::False)) {
        m_parser->nextToken();
        return BydaoValue::get( BydaoBool::create(false) );
    }

    // Null
    if (m_parser->match(BydaoTokenType::Null)) {
        m_parser->nextToken();
        return BydaoValue::get();  // null
    }

    // Идентификаторы (другие константы)
    if (m_parser->match(BydaoTokenType::Identifier)) {
        QString name = m_parser->m_current.text;
        VariableInfo info = m_parser->resolveVariable(name);
        m_parser->nextToken();

//        qDebug() << "evaluatePrimary: identifier" << name << "value =" << info.constValue.toString();  // ОТЛАДКА

        if (info.isConstant) {
            return info.constValue;
        }
        // Если это не константа, возвращаем null
        return BydaoValue::get();
    }

    // Выражения в скобках
    if (m_parser->match(BydaoTokenType::LParen)) {
        m_parser->nextToken();
        BydaoValue* result = evaluateLogicalOr();
        m_parser->expect(BydaoTokenType::RParen);
        return result;
    }

    return BydaoValue::get();  // null
}

} // namespace BydaoScript
