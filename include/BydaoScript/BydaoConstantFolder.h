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
#pragma once

#include "BydaoValue.h"
#include "BydaoLexer.h"

namespace BydaoScript {

// Forward declaration
class BydaoParser;

// Вычислитель константных выражений
class BydaoConstantFolder {
public:
    explicit BydaoConstantFolder(BydaoParser* parser);

    // Проверяет, можно ли вычислить выражение как константу
    bool isConstantExpression();

    // Вычисляет значение константного выражения (начиная с текущей позиции)
    // Если выражение не является константным, возвращает null
    BydaoValue evaluate();

    // Возвращает позицию после вычисленного выражения (для восстановления)
    int getPos() const;

private:
    // Вспомогательные методы для сохранения/восстановления позиции
    void savePos();
    void restorePos();

    // Рекурсивные методы для каждого уровня грамматики
    bool isConstantLogicalOr();
    BydaoValue evaluateLogicalOr();

    bool isConstantLogicalAnd();
    BydaoValue evaluateLogicalAnd();

    bool isConstantEquality();
    BydaoValue evaluateEquality();

    bool isConstantComparison();
    BydaoValue evaluateComparison();

    bool isConstantAddition();
    BydaoValue evaluateAddition();

    bool isConstantTerm();
    BydaoValue evaluateTerm();

    bool isConstantUnary();
    BydaoValue evaluateUnary();

    bool isConstantPrimary();
    BydaoValue evaluatePrimary();

    BydaoParser* m_parser;
    int m_savedPos;
    BydaoToken m_savedToken;
};

} // namespace BydaoScript
