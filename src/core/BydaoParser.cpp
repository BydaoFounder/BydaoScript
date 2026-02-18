#include "BydaoScript/BydaoParser.h"
#include <QCoreApplication>
#include <QRegularExpression>
#include <QtCore/QtCore>

namespace BydaoScript {

// ============================================================
// КОНСТРУКТОР / ДЕСТРУКТОР
// ============================================================

BydaoParser::BydaoParser(const QVector<BydaoToken>& tokens)
    : m_tokens(tokens)
    , m_pos(0)
    , m_labelCounter(0)
    , m_inLoop(false)
{
    if (!m_tokens.isEmpty())
        m_current = m_tokens[0];
    
    // Добавляем пути поиска модулей по умолчанию
    m_modulePaths << ".";
    m_modulePaths << "./modules";
    m_modulePaths << QCoreApplication::applicationDirPath();
    m_modulePaths << QCoreApplication::applicationDirPath() + "/modules";
}

BydaoParser::~BydaoParser() {
    clearModuleCache();
}

// ============================================================
// БАЗОВЫЕ МЕТОДЫ РАБОТЫ С ТОКЕНАМИ
// ============================================================

void BydaoParser::nextToken() {
    m_pos++;
    if (m_pos < m_tokens.size())
        m_current = m_tokens[m_pos];
}

bool BydaoParser::match(BydaoTokenType type) {
    return m_current.type == type;
}

bool BydaoParser::expect(BydaoTokenType type) {
    if (match(type)) {
        nextToken();
        return true;
    }
    error(QString("Expected %1, got %2")
          .arg(static_cast<int>(type))
          .arg(m_current.text));
    return false;
}

bool BydaoParser::peek(BydaoTokenType type, int offset) const {
    int idx = m_pos + offset;
    if (idx >= 0 && idx < m_tokens.size()) {
        return m_tokens[idx].type == type;
    }
    return false;
}

void BydaoParser::error(const QString& msg) {
    m_errors.append(QString("%1 at %2:%3")
                   .arg(msg)
                   .arg(m_current.line)
                   .arg(m_current.column));
}

// ============================================================
// ГЕНЕРАЦИЯ КОДА
// ============================================================

int BydaoParser::emitCode(BydaoOpCode op, QString arg, const BydaoToken& token) {
    m_bytecode.append(BydaoInstruction(
        op, arg,
        token.line ? token.line : m_current.line,
        token.column ? token.column : m_current.column
    ));
    return m_bytecode.size() - 1;
}

int BydaoParser::newLabel() {
    return ++m_labelCounter;
}

void BydaoParser::emitLabel(int label) {
    // Ничего не генерируем, просто запоминаем позицию
    m_labels[label] = m_bytecode.size();
}

void BydaoParser::patchJump(int instrIndex) {
    if (instrIndex >= 0 && instrIndex < m_bytecode.size()) {
        int target = m_labels[m_bytecode[instrIndex].arg.toInt()];
        m_bytecode[instrIndex].arg = QString::number(target);
    }
}

// ============================================================
// СЕМАНТИЧЕСКИЙ АНАЛИЗ - ОБЛАСТИ ВИДИМОСТИ
// ============================================================

void BydaoParser::enterScope(bool isLoop) {
    m_scopes.push(BydaoScope(QSet<QString>(), isLoop));
    emitCode(BydaoOpCode::ScopeBegin);
}

void BydaoParser::exitScope() {
    m_scopes.pop();
    emitCode(BydaoOpCode::ScopeEnd);
}

void BydaoParser::declareVariable(const QString& name, const BydaoToken& token) {
    Q_UNUSED(token)
    
    if (m_scopes.isEmpty()) {
        error("Internal error: no active scope");
        return;
    }
    
    if (m_scopes.top().variables.contains(name)) {
        error("Variable '" + name + "' already declared in this scope");
        return;
    }
    
    m_scopes.top().variables.insert(name);
}

bool BydaoParser::isVariableDeclared(const QString& name) {
    for (int i = m_scopes.size() - 1; i >= 0; --i) {
        if (m_scopes[i].variables.contains(name))
            return true;
    }
    return false;
}

// ============================================================
// РАБОТА С МОДУЛЯМИ
// ============================================================

void BydaoParser::addModulePath(const QString& path) {
    QString normalized = QDir::cleanPath(path);
    if (!m_modulePaths.contains(normalized)) {
        m_modulePaths.prepend(normalized);
    }
}

void BydaoParser::clearModuleCache() {
    qDeleteAll(m_moduleInfoCache);
    m_moduleInfoCache.clear();
}

BydaoModuleInfo* BydaoParser::getModuleInfo(const QString& name) {
    return m_moduleInfoCache.value(name);
}

bool BydaoParser::isModule(const QString& name) {
    return m_moduleInfoCache.contains(name);
}

bool BydaoParser::isModuleMember(const QString& moduleName, const QString& member) {
    auto* info = m_moduleInfoCache.value(moduleName);
    return info && info->hasMember(member);
}

int BydaoParser::methodArgCount(const QString& moduleName, const QString& method) {
    auto* info = m_moduleInfoCache.value(moduleName);
    return info ? info->methodArgCount(method) : -1;
}

QStringList BydaoParser::getMethodArgs(const QString& moduleName, const QString& method) {
    auto* info = m_moduleInfoCache.value(moduleName);
    if (!info) return QStringList();
    return info->methodArgs(method);
}

bool BydaoParser::loadModuleInfo(const QString& name) {
    // Уже загружен?
    if (m_moduleInfoCache.contains(name)) {
        return true;
    }
    
    // Загружаем через ModuleManager
    QString errorMsg;
    BydaoModuleInfo* info = BydaoModuleManager::instance().loadModuleInfo(name, &errorMsg);
    
    if (info) {
        m_moduleInfoCache[name] = info;
        // qDebug() << "📦 Module info loaded:" << name << "v" << info->version();
        return true;
    } else {
        error("Cannot load module '" + name + "': " + errorMsg);
        return false;
    }
}

// ============================================================
// ПАРСИНГ - ОСНОВНЫЕ МЕТОДЫ
// ============================================================

bool BydaoParser::parse() {
    enterScope(false);
    bool ok = parseProgram();
    exitScope();
    emitCode(BydaoOpCode::Halt);
    return ok && m_errors.isEmpty();
}

bool BydaoParser::parseProgram() {
    while (!match(BydaoTokenType::EndOfFile)) {
        if (!parseStatement()) {
            // Восстановление после ошибки
            while (!match(BydaoTokenType::EndOfFile) && 
                   !match(BydaoTokenType::RBrace) && 
                   !match(BydaoTokenType::Semicolon)) {
                nextToken();
            }
            if (!m_errors.isEmpty()) return false;
        }
    }
    return true;
}

bool BydaoParser::parseStatement() {
    // Проверяем присваивание (identifier = expression)
    if (match(BydaoTokenType::Identifier) && peek(BydaoTokenType::Assign, 1)) {
        return parseAssign();
    }

    if (match(BydaoTokenType::Var)) return parseVarDecl();
    if (match(BydaoTokenType::While)) return parseWhile();
    if (match(BydaoTokenType::If)) return parseIf();
    if (match(BydaoTokenType::Iter)) return parseIterEnum(true);
    if (match(BydaoTokenType::Enum)) return parseIterEnum(false);
    if (match(BydaoTokenType::Break) || match(BydaoTokenType::Next)) return parseBreakNext();
    if (match(BydaoTokenType::Use)) return parseUse();
    if (match(BydaoTokenType::LBrace)) return parseBlock(true);
    if (parseExpression()) return true;

    error("Unexpected token: " + m_current.text);
    return false;
}

bool BydaoParser::parseBlock(bool requireBraces) {
    if (requireBraces) {
        if (!expect(BydaoTokenType::LBrace))
            return false;
    }
    
    enterScope(false);
    
    while (!match(BydaoTokenType::RBrace) && !match(BydaoTokenType::EndOfFile)) {
        if (!parseStatement()) {
            exitScope();
            return false;
        }
    }
    
    exitScope();
    
    if (requireBraces) {
        if (!expect(BydaoTokenType::RBrace))
            return false;
    }
    
    return true;
}

// ============================================================
// ПАРСИНГ - ОПЕРАТОРЫ
// ============================================================

bool BydaoParser::parseVarDecl() {
    nextToken(); // var
    
    if (!match(BydaoTokenType::Identifier)) {
        error("Expected variable name");
        return false;
    }
    
    QString name = m_current.text;
    BydaoToken nameToken = m_current;
    declareVariable(name, nameToken);
    nextToken();
    
    emitCode(BydaoOpCode::VarDecl, name, nameToken);
    
    if (match(BydaoTokenType::Assign)) {
        nextToken();
        if (!parseExpression()) return false;
        emitCode(BydaoOpCode::Store, name, nameToken);
    } else {
        emitCode(BydaoOpCode::PushNull);
        emitCode(BydaoOpCode::Store, name, nameToken);
    }
    
    return true;
}

bool BydaoParser::parseWhile() {
    BydaoToken token = m_current;
    nextToken(); // while

    int loopStart = m_bytecode.size();  // запоминаем позицию перед условием цикла
    if (!parseExpression()) {
        error("Expected condition");
        return false;
    }

    QVector<BydaoInstruction> nextCode;
    bool hasNext = match(BydaoTokenType::Next);
    if (hasNext) {
        nextToken(); // next
        int savedPos = m_bytecode.size();

        if (!parseStatement()) {
            error("Expected statement after next");
            return false;
        }

        for (int i = savedPos; i < m_bytecode.size(); i++) {
            nextCode.append(m_bytecode[i]);
        }
        m_bytecode.resize(savedPos);
    }

//    int loopStart = m_bytecode.size();  // запоминаем позицию начала цикла
    int condJump = emitCode(BydaoOpCode::JumpIfFalse, "?", token);

    enterScope(true);
    m_inLoop = true;

    if (!parseBlock(true)) {
        m_inLoop = false;
        exitScope();
        return false;
    }

    m_inLoop = false;
    exitScope();

    // Вставляем next-код
    if (hasNext) {
        for (const auto& instr : nextCode) {
            m_bytecode.append(instr);
        }
    }

    // Прыгаем назад
    emitCode(BydaoOpCode::Jump, QString::number(loopStart));

    // Патчим условный переход
    m_bytecode[condJump].arg = QString::number(m_bytecode.size());

    return true;
}

bool BydaoParser::parseIf() {
    nextToken(); // if
    
    if (!parseExpression()) return false;
    
    int elseJump = emitCode(BydaoOpCode::JumpIfFalse, "?");
    
    if (!parseBlock(true)) return false;
    
    int endJump = emitCode(BydaoOpCode::Jump, "?");
    patchJump(elseJump);
    
    if (match(BydaoTokenType::Else)) {
        nextToken();
        if (match(BydaoTokenType::If)) {
            if (!parseIf()) return false;
        } else {
            if (!parseBlock(true)) return false;
        }
    }
    
    patchJump(endJump);
    
    return true;
}

bool BydaoParser::parseIterEnum(bool isIter) {
    BydaoToken token = m_current;
    nextToken(); // iter/enum
    
    if (!parseExpression()) {
        error("Expected collection");
        return false;
    }
    
    if (!expect(BydaoTokenType::As)) return false;
    
    if (!match(BydaoTokenType::Identifier)) {
        error("Expected item variable");
        return false;
    }
    
    QString itemName = m_current.text;
    BydaoToken itemToken = m_current;
    declareVariable(itemName, itemToken);
    nextToken();
    
    QString iterName = QString("__%1_%2")
        .arg(isIter ? "iter" : "enum")
        .arg(m_bytecode.size());
    
    declareVariable(iterName, token);
    emitCode(BydaoOpCode::VarDecl, iterName, token);
    emitCode(BydaoOpCode::Member, isIter ? "iter" : "enum", token);
    emitCode(BydaoOpCode::Call, "0", token);
    emitCode(BydaoOpCode::Store, iterName, token);
    
    int loopStart = m_bytecode.size();
    emitLabel(newLabel());
    emitCode(BydaoOpCode::Load, iterName, token);
    emitCode(BydaoOpCode::Member, "next", token);
    emitCode(BydaoOpCode::Call, "0", token);
    int condJump = emitCode(BydaoOpCode::JumpIfFalse, "", token);
    emitCode(BydaoOpCode::Load, iterName, token);
    emitCode(BydaoOpCode::Member, "value", token);
    emitCode(BydaoOpCode::Store, itemName, itemToken);
    
    enterScope(true);
    m_inLoop = true;
    
    if (!parseBlock(true)) {
        m_inLoop = false;
        exitScope();
        return false;
    }
    
    m_inLoop = false;
    exitScope();
    
    emitCode(BydaoOpCode::Jump, QString::number(loopStart));
    patchJump(condJump);
    
    return true;
}

bool BydaoParser::parseBreakNext() {
    if (!m_inLoop) {
        error("break/next outside loop");
        return false;
    }
    
    if (match(BydaoTokenType::Break)) {
        emitCode(BydaoOpCode::Break);
        nextToken();
    } else if (match(BydaoTokenType::Next)) {
        emitCode(BydaoOpCode::Next);
        nextToken();
    }
    
    return true;
}

bool BydaoParser::parseUse() {
    BydaoToken token = m_current;
    nextToken(); // use
    
    if (!match(BydaoTokenType::Identifier)) {
        error("Expected module name");
        return false;
    }
    
    QString moduleName = m_current.text;
    nextToken();
    
    QString alias = moduleName;
    if (match(BydaoTokenType::As)) {
        nextToken();
        if (!match(BydaoTokenType::Identifier)) {
            error("Expected alias name");
            return false;
        }
        alias = m_current.text;
        nextToken();
    }
    
    // Загружаем информацию о модуле
    if (!loadModuleInfo(moduleName)) {
        return false;
    }
    
    // Генерируем код для VM
    if (alias == moduleName) {
        emitCode(BydaoOpCode::UseModule, moduleName, token);
    } else {
        emitCode(BydaoOpCode::UseModule, moduleName + " as " + alias, token);
    }
    
    return true;
}

// ============================================================
// ПАРСИНГ - ВЫРАЖЕНИЯ
// ============================================================

bool BydaoParser::parseExpression() {
    return parseLogicalOr();
}

bool BydaoParser::parseLogicalOr() {
    if (!parseLogicalAnd()) return false;
    
    while (match(BydaoTokenType::Or)) {
        BydaoToken op = m_current;
        nextToken();
        if (!parseLogicalAnd()) return false;
        emitCode(BydaoOpCode::Or, "", op);
    }
    
    return true;
}

bool BydaoParser::parseLogicalAnd() {
    if (!parseEquality()) return false;
    
    while (match(BydaoTokenType::And)) {
        BydaoToken op = m_current;
        nextToken();
        if (!parseEquality()) return false;
        emitCode(BydaoOpCode::And, "", op);
    }
    
    return true;
}

bool BydaoParser::parseEquality() {
    if (!parseComparison()) return false;
    
    while (match(BydaoTokenType::Equal) || match(BydaoTokenType::NotEqual)) {
        bool isEq = match(BydaoTokenType::Equal);
        BydaoToken op = m_current;
        nextToken();
        if (!parseComparison()) return false;
        emitCode(isEq ? BydaoOpCode::Eq : BydaoOpCode::Neq, "", op);
    }
    
    return true;
}

bool BydaoParser::parseComparison() {
    if (!parseAddition()) return false;
    
    while (match(BydaoTokenType::Less) || match(BydaoTokenType::Greater) ||
           match(BydaoTokenType::LessEqual) || match(BydaoTokenType::GreaterEqual)) {
        BydaoToken op = m_current;
        nextToken();
        if (!parseAddition()) return false;
        
        switch (op.type) {
            case BydaoTokenType::Less: emitCode(BydaoOpCode::Lt, "", op); break;
            case BydaoTokenType::Greater: emitCode(BydaoOpCode::Gt, "", op); break;
            case BydaoTokenType::LessEqual: emitCode(BydaoOpCode::Le, "", op); break;
            case BydaoTokenType::GreaterEqual: emitCode(BydaoOpCode::Ge, "", op); break;
            default: break;
        }
    }
    
    return true;
}

bool BydaoParser::parseAddition() {
    if (!parseTerm()) return false;
    
    while (match(BydaoTokenType::Plus) || match(BydaoTokenType::Minus)) {
        bool isPlus = match(BydaoTokenType::Plus);
        BydaoToken op = m_current;
        nextToken();
        if (!parseTerm()) return false;
        emitCode(isPlus ? BydaoOpCode::Add : BydaoOpCode::Sub, "", op);
    }
    
    return true;
}

bool BydaoParser::parseTerm() {
    if (!parseUnary()) return false;
    
    while (match(BydaoTokenType::Mul) || match(BydaoTokenType::Div)) {
        bool isMul = match(BydaoTokenType::Mul);
        BydaoToken op = m_current;
        nextToken();
        if (!parseUnary()) return false;
        emitCode(isMul ? BydaoOpCode::Mul : BydaoOpCode::Div, "", op);
    }
    
    return true;
}

bool BydaoParser::parseUnary() {
    if (match(BydaoTokenType::Not) || match(BydaoTokenType::Minus)) {
        bool isNot = match(BydaoTokenType::Not);
        BydaoToken op = m_current;
        nextToken();
        if (!parseUnary()) return false;
        emitCode(isNot ? BydaoOpCode::Not : BydaoOpCode::Neg, "", op);
        return true;
    }
    return parsePrimary();
}

bool BydaoParser::parsePrimary() {
    if (match(BydaoTokenType::Identifier)) {
        QString name = m_current.text;
        BydaoToken nameToken = m_current;
        
        if (!isVariableDeclared(name) && !isModule(name)) {
            // Пробуем автоматически загрузить модуль
            loadModuleInfo(name);
            if (!isModule(name)) {
                error("Undeclared variable or unknown module: " + name);
            }
        }
        
        nextToken();
        
        if (match(BydaoTokenType::Dot) || match(BydaoTokenType::LBracket)) {
            m_pos--;
            m_current = m_tokens[m_pos];
            return parseMember(true);
        }
        
        emitCode(BydaoOpCode::Load, name, nameToken);
        return true;
    }
    
    if (match(BydaoTokenType::Number)) {
        QString text = m_current.text;

        // Определяем тип числа
        if (text.contains('.') || text.contains('e') || text.contains('E')) {
            // Вещественное число
            emitCode(BydaoOpCode::PushReal, text, m_current);
        } else {
            // Целое число
            emitCode(BydaoOpCode::PushInt, text, m_current);
        }

        nextToken();
        return true;
    }
    
    if (match(BydaoTokenType::String)) {
        emitCode(BydaoOpCode::PushString, m_current.text, m_current);
        nextToken();
        return true;
    }
    
    if (match(BydaoTokenType::LBracket)) {
        return parseArrayLiteral();
    }
    
    if (match(BydaoTokenType::LParen)) {
        nextToken();
        if (!parseExpression()) return false;
        return expect(BydaoTokenType::RParen);
    }
    
    return false;
}

bool BydaoParser::parseAssign() {
    QString name = m_current.text;
    BydaoToken nameToken = m_current;
    nextToken(); // identifier

    expect(BydaoTokenType::Assign); // =

    if (!parseExpression()) {
        error("Expected expression after =");
        return false;
    }

    // Генерируем код: выражение уже на стеке, теперь сохраняем
    emitCode(BydaoOpCode::Store, name, nameToken);

    return true;
}

bool BydaoParser::parseMember(bool canAssign) {
    if (!match(BydaoTokenType::Identifier)) {
        error("Expected object name");
        return false;
    }
    
    QString object = m_current.text;
    BydaoToken objToken = m_current;
    nextToken();
    
    while (match(BydaoTokenType::Dot) || match(BydaoTokenType::LBracket)) {
        if (match(BydaoTokenType::Dot)) {
            nextToken();
            
            if (!match(BydaoTokenType::Identifier)) {
                error("Expected member name");
                return false;
            }
            
            QString member = m_current.text;
            BydaoToken memberToken = m_current;
            nextToken();
            
            // Проверка для модулей
            if (isModule(object)) {
                auto* info = getModuleInfo(object);

                // === ДИАГНОСТИКА ===
                if (!info) {
                    qDebug() << "❌ CRITICAL: info is NULL for module" << object;
                    error("Internal error: module info is null");
                    return false;
                }

                // ======= Проверяем валидность указателя
                qDebug() << "Module info pointer:" << info;
                qDebug() << "Module name:" << info->name();
                qDebug() << "Module version:" << info->version();
                // ======================================

                if (!info->hasMember(member)) {
                    error(QString("Module '%1' has no member '%2'")
                         .arg(object).arg(member));
                    return false;
                }
                
                if (info->isMethod(member) && !match(BydaoTokenType::LParen)) {
                    error(QString("Method '%1' requires parentheses").arg(member));
                    return false;
                }
            }
            
            if (match(BydaoTokenType::LParen)) {
                // --- сначала кладём объект, потом метод ---
                emitCode(BydaoOpCode::Load, object, objToken);  // объект на стек
                emitCode(BydaoOpCode::PushString, member, memberToken);  // имя метода на стек
                if (!parseCall(object)) {
                    return false;
                }
                object = "_result";
            } else {
                emitCode(BydaoOpCode::Load, object, objToken);
                emitCode(BydaoOpCode::Member, member, memberToken);
                object = "_acc";
            }
        }
        else if (match(BydaoTokenType::LBracket)) {
            nextToken();
            if (!parseExpression()) return false;
            if (!expect(BydaoTokenType::RBracket)) return false;
            
            emitCode(BydaoOpCode::Load, object, objToken);
            emitCode(BydaoOpCode::Index, "", objToken);
            object = "_acc";
        }
    }
    
    if (canAssign && match(BydaoTokenType::Assign)) {
        if (isModule(object)) {
            error("Cannot assign to module");
            return false;
        }
        
        BydaoToken assignToken = m_current;
        nextToken();
        if (!parseExpression()) return false;
        emitCode(BydaoOpCode::Store, object, assignToken);
    }
    
    return true;
}

bool BydaoParser::parseCall(const QString& object) {
    // object уже не нужен - объект и метод уже на стеке!
    Q_UNUSED( object );

    if (!expect(BydaoTokenType::LParen)) return false;

    int argCount = 0;

    if (!match(BydaoTokenType::RParen)) {
        do {
            if (!parseExpression()) return false;
            argCount++;
        } while (match(BydaoTokenType::Comma) && (nextToken(), true));
    }

    if (!expect(BydaoTokenType::RParen)) return false;

    // Генерируем CALL с количеством аргументов
    emitCode(BydaoOpCode::Call, QString::number(argCount));

    return true;
}

bool BydaoParser::parseArrayLiteral() {
    nextToken();
    emitCode(BydaoOpCode::PushArray);
    int idx = 0;
    
    if (!match(BydaoTokenType::RBracket)) {
        do {
            if (!parseExpression()) return false;
            emitCode(BydaoOpCode::Member, QString::number(idx++));
        } while (match(BydaoTokenType::Comma) && (nextToken(), true));
    }
    
    return expect(BydaoTokenType::RBracket);
}

} // namespace BydaoScript
