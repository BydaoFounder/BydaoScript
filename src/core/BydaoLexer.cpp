#include "BydaoScript/BydaoLexer.h"

namespace BydaoScript {

QHash<QString, BydaoTokenType> BydaoLexer::m_keywords = {
    {"var", BydaoTokenType::Var},
    {"while", BydaoTokenType::While},
    {"next", BydaoTokenType::Next},
    {"if", BydaoTokenType::If},
    {"else", BydaoTokenType::Else},
    {"iter", BydaoTokenType::Iter},
    {"enum", BydaoTokenType::Enum},
    {"as", BydaoTokenType::As},
    {"break", BydaoTokenType::Break},
    {"use", BydaoTokenType::Use}
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

BydaoToken BydaoLexer::readNumber() {
    int start = m_pos, startCol = m_column;
    bool hasDot = false;
    while (m_pos < m_source.length()) {
        QChar ch = m_source[m_pos];
        if (ch.isDigit()) { m_pos++; m_column++; }
        else if (ch == '.' && !hasDot) { hasDot = true; m_pos++; m_column++; }
        else break;
    }
    QString text = m_source.mid(start, m_pos - start);
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

BydaoToken BydaoLexer::readString(QChar quote) {
    int start = m_pos, startCol = m_column;
    m_pos++; m_column++;
    while (m_pos < m_source.length() && m_source[m_pos] != quote) {
        if (m_source[m_pos] == '\n') { m_line++; m_column = 1; }
        else m_column++;
        m_pos++;
    }
    if (m_pos < m_source.length()) { m_pos++; m_column++; }
    QString text = m_source.mid(start, m_pos - start);
    return BydaoToken(BydaoTokenType::String, text, m_line, startCol);
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
    }

    m_pos++; m_column++;
    switch (ch.toLatin1()) {
        case '=': return BydaoToken(BydaoTokenType::Assign, "=", m_line, startCol);
        case '+': return BydaoToken(BydaoTokenType::Plus, "+", m_line, startCol);
        case '-': return BydaoToken(BydaoTokenType::Minus, "-", m_line, startCol);
        case '*': return BydaoToken(BydaoTokenType::Mul, "*", m_line, startCol);
        case '/': return BydaoToken(BydaoTokenType::Div, "/", m_line, startCol);
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
    while (m_pos < m_source.length()) {
        skipWhitespace();
        if (m_pos >= m_source.length()) break;
        if (skipComment()) continue;

        QChar ch = m_source[m_pos];
        if (ch.isDigit()) tokens.append(readNumber());
        else if (ch.isLetter() || ch == '_') tokens.append(readIdentifier());
        else if (ch == '\'' || ch == '"') tokens.append(readString(ch));
        else tokens.append(readOperator(ch));
    }
    tokens.append(BydaoToken(BydaoTokenType::EndOfFile, "EOF", m_line, m_column));
    return tokens;
}

} // namespace BydaoScript