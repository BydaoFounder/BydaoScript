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
#include "BydaoScript/BydaoLexer.h"

namespace BydaoScript {

QHash<QString, BydaoTokenType> BydaoLexer::m_keywords = {
    {"use", BydaoTokenType::Use},
    {"var", BydaoTokenType::Var},
    {"pub", BydaoTokenType::Pub},
    {"const", BydaoTokenType::Const},
    {"drop", BydaoTokenType::Drop},
    {"while", BydaoTokenType::While},
    {"next", BydaoTokenType::Next},
    {"break", BydaoTokenType::Break},
    {"if", BydaoTokenType::If},
    {"elsif", BydaoTokenType::Elsif},
    {"else", BydaoTokenType::Else},
    {"iter", BydaoTokenType::Iter},
    {"enum", BydaoTokenType::Enum},
    {"as", BydaoTokenType::As},
    {"false", BydaoTokenType::False},
    {"true", BydaoTokenType::True},
    {"null", BydaoTokenType::Null},
};

BydaoLexer::BydaoLexer(const QString& source)
    : m_source(source), m_pos(0), m_line(1), m_column(1) {}

void BydaoLexer::skipWhitespace() {
    while (m_pos < m_source.length()) {
        QChar ch = m_source[m_pos];
        if (ch == ' ' || ch == '\t') { m_pos++; m_column++; }
        else if (ch == '\n') { m_pos++; m_line++; m_column = 1; }
        else if (ch == '\r') { m_pos++; }
        else break;
    }
}

bool BydaoLexer::skipComment() {
    if (m_source.mid(m_pos, 2) == "//") {
        m_pos += 2; m_column += 2;
        while (m_pos < m_source.length() && m_source[m_pos] != '\n') {
            m_pos++; m_column++;
        }
        return true;
    }
    if (m_source.mid(m_pos, 2) == "/*") {
        m_pos += 2; m_column += 2;
        while (m_pos < m_source.length() - 1) {
            if (m_source[m_pos] == '\n') { m_line++; m_column = 1; }
            if (m_source.mid(m_pos, 2) == "*/") {
                m_pos += 2; m_column += 2;
                break;
            }
            m_pos++; m_column++;
        }
        return true;
    }
    return false;
}

BydaoToken BydaoLexer::readDecNumber() {
    int startCol = m_column;
    QString text = extractDecNumber();
    return BydaoToken(BydaoTokenType::Number, text, m_line, startCol);
}

BydaoToken BydaoLexer::readHexNumber() {
    int startCol = m_column;
    QString text = extractHexNumber();
    return BydaoToken(BydaoTokenType::Number, text, m_line, startCol);
}

BydaoToken BydaoLexer::readBinNumber() {
    int startCol = m_column;
    QString text = extractBinNumber();
    return BydaoToken(BydaoTokenType::Number, text, m_line, startCol);
}

BydaoToken BydaoLexer::readIdentifier() {
    int start = m_pos, startCol = m_column;
    while (m_pos < m_source.length()) {
        QChar ch = m_source[m_pos];
        if (ch.isLetterOrNumber() || ch == '_' || ch == '-') {
            m_pos++; m_column++;
        } else break;
    }
    QString text = m_source.mid(start, m_pos - start);
    BydaoTokenType type = m_keywords.value(text, BydaoTokenType::Identifier);
    return BydaoToken(type, text, m_line, startCol);
}

BydaoToken BydaoLexer::readString(QChar quote)
{
    int startLine = m_line;
    int startCol = m_column;
    QString text;

    m_pos++; // пропускаем открывающую кавычку
    m_column++;

    bool escape = false;

    while (m_pos < m_source.length()) {
        QChar ch = m_source[m_pos];

        if (escape) {
            // Обработка escape-последовательностей
            switch (ch.toLatin1()) {
            case 'n': text += '\n'; break;
            case 't': text += '\t'; break;
            case 'r': text += '\r'; break;
            case '\\': text += '\\'; break;
            case '\'': text += '\''; break;
            case '"': text += '"'; break;
            default: text += ch; break;
            }
            escape = false;
            m_pos++;
            m_column++;
            continue;
        }

        if (ch == '\\') {
            escape = true;
            m_pos++;
            m_column++;
            continue;
        }

        if (ch == quote) {
            m_pos++; // пропускаем закрывающую кавычку
            m_column++;
            break;
        }

        if (ch == '\n') {
            m_error = "Unclosed string at line " + QString::number(m_line);
            break;
        }

        text += ch;
        m_pos++;
        m_column++;
    }

    return BydaoToken(BydaoTokenType::String, text, startLine, startCol);
}

BydaoToken BydaoLexer::readOperator(QChar ch) {
    int startCol = m_column;

    if (m_pos + 1 < m_source.length()) {
        QString two = QString(ch) + m_source[m_pos + 1];
        if (two == "==") { m_pos += 2; m_column += 2; return BydaoToken(BydaoTokenType::Equal, two, m_line, startCol); }
        if (two == "!=") { m_pos += 2; m_column += 2; return BydaoToken(BydaoTokenType::NotEqual, two, m_line, startCol); }
        if (two == "<=") { m_pos += 2; m_column += 2; return BydaoToken(BydaoTokenType::LessEqual, two, m_line, startCol); }
        if (two == ">=") { m_pos += 2; m_column += 2; return BydaoToken(BydaoTokenType::GreaterEqual, two, m_line, startCol); }
        if (two == "&&") { m_pos += 2; m_column += 2; return BydaoToken(BydaoTokenType::And, two, m_line, startCol); }
        if (two == "||") { m_pos += 2; m_column += 2; return BydaoToken(BydaoTokenType::Or, two, m_line, startCol); }
        if (two == "+=") { m_pos += 2; m_column += 2; return BydaoToken(BydaoTokenType::PlusAssign, two, m_line, startCol); }
        if (two == "-=") { m_pos += 2; m_column += 2; return BydaoToken(BydaoTokenType::MinusAssign, two, m_line, startCol); }
        if (two == "*=") { m_pos += 2; m_column += 2; return BydaoToken(BydaoTokenType::MulAssign, two, m_line, startCol); }
        if (two == "/=") { m_pos += 2; m_column += 2; return BydaoToken(BydaoTokenType::DivAssign, two, m_line, startCol); }
        if (two == "%=") { m_pos += 2; m_column += 2; return BydaoToken(BydaoTokenType::ModAssign, two, m_line, startCol); }
    }

    m_pos++; m_column++;
    switch (ch.toLatin1()) {
        case '=': return BydaoToken(BydaoTokenType::Assign, "=", m_line, startCol);
        case '+': return BydaoToken(BydaoTokenType::Plus, "+", m_line, startCol);
        case '-': return BydaoToken(BydaoTokenType::Minus, "-", m_line, startCol);
        case '*': return BydaoToken(BydaoTokenType::Mul, "*", m_line, startCol);
        case '/': return BydaoToken(BydaoTokenType::Div, "/", m_line, startCol);
        case '%': return BydaoToken(BydaoTokenType::Mod, "%", m_line, startCol);
        case '!': return BydaoToken(BydaoTokenType::Not, "!", m_line, startCol);
        case '<': return BydaoToken(BydaoTokenType::Less, "<", m_line, startCol);
        case '>': return BydaoToken(BydaoTokenType::Greater, ">", m_line, startCol);
        case '(': return BydaoToken(BydaoTokenType::LParen, "(", m_line, startCol);
        case ')': return BydaoToken(BydaoTokenType::RParen, ")", m_line, startCol);
        case '{': return BydaoToken(BydaoTokenType::LBrace, "{", m_line, startCol);
        case '}': return BydaoToken(BydaoTokenType::RBrace, "}", m_line, startCol);
        case '[': return BydaoToken(BydaoTokenType::LBracket, "[", m_line, startCol);
        case ']': return BydaoToken(BydaoTokenType::RBracket, "]", m_line, startCol);
        case ',': return BydaoToken(BydaoTokenType::Comma, ",", m_line, startCol);
        case '.': return BydaoToken(BydaoTokenType::Dot, ".", m_line, startCol);
        case ':': return BydaoToken(BydaoTokenType::Colon, ":", m_line, startCol);
    }
    return BydaoToken(BydaoTokenType::Error, QString(ch), m_line, startCol);
}

QVector<BydaoToken> BydaoLexer::tokenize() {
    QVector<BydaoToken> tokens;
    int length = m_source.length();
    while (m_pos < length) {
        skipWhitespace();
        if (m_pos >= length ) break;
        if ( skipComment() ) continue;

        QChar ch = m_source[m_pos];
        QChar nextCh = ( (m_pos + 1) < length ) ? m_source[m_pos + 1] : ch;
        if ( ch == '0' && (nextCh == 'x' || nextCh == 'X') ) {
            tokens.append( readHexNumber() );
        }
        else if ( ch == '0' && (nextCh == 'b' || nextCh == 'B') ) {
            tokens.append( readBinNumber() );
        }
        else if ( ch.isDigit() || ( ch == '-' && nextCh.isDigit() ) ) {
            tokens.append( readDecNumber() );
        }
        else if (ch.isLetter() || ch == '_') {
            tokens.append( readIdentifier() );
        }
        else if (ch == '\'' || ch == '"') {
            tokens.append( readString(ch) );
        }
        else {
            tokens.append( readOperator(ch) );
        }
    }
    tokens.append(BydaoToken(BydaoTokenType::EndOfFile, "EOF", m_line, m_column));
    return tokens;
}


QString BydaoLexer::extractDecNumber() {
    QString result;
    int length = m_source.length();

    // Пропускаем пробельные символы в начале
    while (m_pos < length && m_source[m_pos].isSpace()) {
        m_pos++;
    }

    if (m_pos >= length) {
        return result; // Достигнут конец строки
    }

    // Обработка знака числа
    if (m_source[m_pos] == '+' || m_source[m_pos] == '-') {
        result += m_source[m_pos];
        m_pos++;
    }

    bool hasDigits = false;
    bool hasDecimalPoint = false;
    bool hasExponent = false;

    // Сборка мантиссы (целая и дробная часть)
    while (m_pos < length) {
        QChar c = m_source[m_pos];

        if (c.isDigit()) {
            result += c;
            hasDigits = true;
            m_pos++;
        }
        else if (c == '.' && !hasDecimalPoint && !hasExponent) {
            // Десятичная точка может быть только одна и до экспоненты
            result += c;
            hasDecimalPoint = true;
            m_pos++;
        }
        else {
            break; // Выходим при обнаружении нецифрового символа
        }
    }

    // Если нет цифр после возможного знака, число не найдено
    if (!hasDigits) {
        // Откатываем позицию, если не нашли число
        while (m_pos > 0 && (m_source[m_pos-1] == '+' || m_source[m_pos-1] == '-')) {
            m_pos--;
            result.chop(1);
        }
        result.clear();
        return result;
    }

    // Проверка на экспоненциальную часть (научная нотация)
    if (m_pos < length && (m_source[m_pos] == 'e' || m_source[m_pos] == 'E')) {
        int expPos = m_pos;
        QString expPart;
        expPart += m_source[m_pos];
        m_pos++;

        // Знак экспоненты
        if (m_pos < length && (m_source[m_pos] == '+' || m_source[m_pos] == '-')) {
            expPart += m_source[m_pos];
            m_pos++;
        }

        // Цифры экспоненты
        bool hasExpDigits = false;
        while (m_pos < length && m_source[m_pos].isDigit()) {
            expPart += m_source[m_pos];
            hasExpDigits = true;
            m_pos++;
        }

        if (hasExpDigits) {
            result += expPart;
            hasExponent = true;
        }
        else {
            // Откатываем позицию, если после e/E нет допустимой экспоненты
            m_pos = expPos;
        }
    }

    return result;
}

// Вспомогательная функция для проверки HEX цифры
bool BydaoLexer::isHexDigit(const QChar& c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f');
}

QString BydaoLexer::extractHexNumber() {
    QString result;
    int length = m_source.length();

    // Пропускаем пробельные символы в начале
    while (m_pos < length && m_source[m_pos].isSpace()) {
        m_pos++;
    }

    if (m_pos >= length) {
        return result; // Достигнут конец строки
    }

    // Проверяем наличие префикса 0x или 0X
    bool hasPrefix = false;
    if (m_pos + 1 < length &&
        m_source[m_pos] == '0' &&
        (m_source[m_pos + 1] == 'x' || m_source[m_pos + 1] == 'X')) {

        result += m_source[m_pos];
        result += m_source[m_pos + 1];
        m_pos += 2;
        hasPrefix = true;
    }

    // Альтернативный префикс: просто x или X (иногда используется)
    else if (m_pos < length &&
             (m_source[m_pos] == 'x' || m_source[m_pos] == 'X') &&
             m_pos + 1 < length &&
             isHexDigit(m_source[m_pos + 1])) {

        result += m_source[m_pos];
        m_pos++;
    }

    // Собираем шестнадцатеричные цифры
    bool hasDigits = false;
    while (m_pos < length) {
        QChar c = m_source[m_pos];

        if (isHexDigit(c)) {
            result += c;
            hasDigits = true;
            m_pos++;
        }
        else {
            break;
        }
    }

    // Если не нашли цифр, откатываемся
    if (!hasDigits) {
        if (hasPrefix) {
            // Откатываем префикс 0x
            m_pos -= result.length();
        }
        result.clear();
    }

    return result;
}

QString BydaoLexer::extractBinNumber() {
    QString result;
    int length = m_source.length();

    if (m_pos >= length) {
        return result; // Достигнут конец строки
    }

    // Проверяем наличие префикса 0b или 0B
    bool hasPrefix = false;
    if (m_pos + 1 < length &&
        m_source[m_pos] == '0' &&
        (m_source[m_pos + 1] == 'b' || m_source[m_pos + 1] == 'B')) {

        result += m_source[m_pos];
        result += m_source[m_pos + 1];
        m_pos += 2;
        hasPrefix = true;
    }

    // Собираем двоичные цифры (0 и 1)
    bool hasDigits = false;
    while (m_pos < length) {
        QChar c = m_source[m_pos];

        if (c == '0' || c == '1') {
            result += c;
            hasDigits = true;
            m_pos++;
        }
        else {
            break;
        }
    }

    // Если не нашли цифр после префикса, откатываемся
    if (!hasDigits) {
        if (hasPrefix) {
            // Откатываем префикс 0b
            m_pos -= result.length();
        }
        result.clear();
    }

    return result;
}

} // namespace BydaoScript
