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

#include <QString>
#include <QVector>
#include <QHash>

namespace BydaoScript {

enum class BydaoTokenType {
    // Ключевые слова
    Var, Const, Pub, Drop, While, Next, If, Elsif, Else, Iter, Enum,
    As, Break, Use, False, True, Null,

    // Операторы
    Assign, PlusAssign, MinusAssign, MulAssign, DivAssign, ModAssign,
    Plus, Minus,
    Mul, Div, Mod,
    And, Or, Not,

    // Сравнение
    Equal, NotEqual, Less, Greater, LessEqual, GreaterEqual,

    // Разделители
    LParen, RParen,     // ()
    LBrace, RBrace,     // {}
    LBracket, RBracket, // []
    Comma, Dot, Colon, Semicolon,

    // Литералы
    Identifier, Number, String,

    // Спецсимволы
    Error, EndOfFile
};

struct BydaoToken {
    BydaoTokenType type;
    QString text;
    int line;
    int column;

    BydaoToken(BydaoTokenType t = BydaoTokenType::Error, QString txt = "", int l = 0, int c = 0)
        : type(t), text(txt), line(l), column(c) {}
};

class BydaoLexer {
public:
    explicit BydaoLexer(const QString& source);
    QVector<BydaoToken> tokenize();
    QString errorMessage() const { return m_error; }

private:
    void skipWhitespace();
    bool skipComment();
    BydaoToken readNumber();
    BydaoToken readIdentifier();
    BydaoToken readString(QChar quote);
    BydaoToken readOperator(QChar ch);

    QString m_source;
    int m_pos;
    int m_line;
    int m_column;
    QString m_error;

    static QHash<QString, BydaoTokenType> m_keywords;
};

} // namespace BydaoScript
