#include "BydaoScript/BydaoParser.h"
#include <QCoreApplication>
#include <QDir>

namespace BydaoScript {

// ============================================================
// КОНСТРУКТОР / ДЕСТРУКТОР
// ============================================================

BydaoParser::BydaoParser(const QVector<BydaoToken>& tokens)
    : m_tokens(tokens)
    , m_pos(0)
    , m_currentScopeDepth(0)
    , m_inLoop(false)
    , m_iteratorCounter(0)
{
    if (!m_tokens.isEmpty())
        m_current = m_tokens[0];

    // Добавляем пути поиска модулей
    m_modulePaths << ".";
    m_modulePaths << "./modules";
    m_modulePaths << QCoreApplication::applicationDirPath();
    m_modulePaths << QCoreApplication::applicationDirPath() + "/modules";

    // Добавляем пустую строку под индексом 0
    addString("");

    initBuiltinTypes();
}

BydaoParser::~BydaoParser() {
    qDeleteAll(m_moduleInfoCache);
}

// ============================================================
// РАБОТА С ТАБЛИЦАМИ
// ============================================================

qint16 BydaoParser::addString(const QString& str) {
    if (str.isEmpty()) return 0;  // индекс 0 зарезервирован для пустой строки

    auto it = m_stringIndex.find(str);
    if (it != m_stringIndex.end())
        return it.value();

    if (m_stringTable.size() >= 65536) {
        error("String table overflow (max 65536 strings)");
        return 0;
    }

    qint16 index = m_stringTable.size();
    m_stringTable.append(str);
    m_stringIndex[str] = index;
    return index;
}

qint16 BydaoParser::addConstant(const BydaoConstant& c) {
    // Для строк используем специальный ключ
    QString key;
    if (c.type == CONST_STRING) {
        key = "s:" + QString::number(c.stringIndex);
    } else {
        switch (c.type) {
        case CONST_INT: key = "i:" + QString::number(c.intValue); break;
        case CONST_REAL: key = "r:" + QString::number(c.realValue); break;
        case CONST_BOOL: key = "b:" + QString::number(c.boolValue); break;
        case CONST_NULL: key = "null"; break;
        default: break;
        }
    }

    if (!key.isEmpty()) {
        auto it = m_constantIndex.find(key);
        if (it != m_constantIndex.end())
            return it.value();
    }

    if (m_constants.size() >= 65536) {
        error("Constant table overflow (max 65536 constants)");
        return 0;
    }

    qint16 index = m_constants.size();
    m_constants.append(c);
    if (!key.isEmpty())
        m_constantIndex[key] = index;
    return index;
}

qint16 BydaoParser::addConstant(qint64 value) {
    return addConstant(BydaoConstant(value));
}

qint16 BydaoParser::addConstant(double value) {
    return addConstant(BydaoConstant(value));
}

qint16 BydaoParser::addConstant(bool value) {
    return addConstant(BydaoConstant(value));
}

qint16 BydaoParser::addConstant(const QString& strValue) {
    qint16 strIndex = addString(strValue);
    return addConstant(BydaoConstant(quint32(strIndex)));
}

qint16 BydaoParser::addNullConstant() {
    return addConstant(BydaoConstant());
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
    error(QString("Expected %1, got '%2'")
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

qint16 BydaoParser::emitCode(BydaoOpCode op, qint16 arg1, qint16 arg2) {
    m_bytecode.append(BydaoInstruction(
        op, arg1, arg2,
        (qint16)m_current.line,
        (qint16)m_current.column
    ));
    return m_bytecode.size() - 1;
}

qint16 BydaoParser::emitCode(BydaoOpCode op, qint16 arg1, qint16 arg2, const BydaoToken& token) {
    m_bytecode.append(BydaoInstruction(
        op, arg1, arg2,
        (qint16)token.line,
        (qint16)token.column
    ));
    return m_bytecode.size() - 1;
}

void BydaoParser::patchJump(int instrIndex) {
    if (instrIndex >= 0 && instrIndex < m_bytecode.size()) {
        int target = m_labels[m_bytecode[instrIndex].arg1];
        m_bytecode[instrIndex].arg1 = target;
    }
}

void BydaoParser::patchJumps(const QVector<int>& jumps, int targetAddr) {
    for (int instrIndex : jumps) {
        if (instrIndex >= 0 && instrIndex < m_bytecode.size()) {
            m_bytecode[instrIndex].arg1 = targetAddr;
        }
    }
}

// ============================================================
// ОБЛАСТИ ВИДИМОСТИ
// ============================================================

void BydaoParser::enterScope() {
    // Генерируем SCOPEBEG и запоминаем его индекс
    int begIdx = emitCode(BydaoOpCode::ScopeBegin, 0, 0);

    ScopeInfo info;
    info.beginInstrIndex = begIdx;
    info.hasVariables = false;
    info.scopeDepth = m_scopeStack.size();  // текущая глубина

    m_scopeStack.push(info);
    m_varScopes.push(QSet<QString>());  // пустой набор переменных
}

void BydaoParser::exitScope() {
    if (m_scopeStack.isEmpty()) {
        error("No scope to exit");
        return;
    }

    ScopeInfo info = m_scopeStack.pop();
    QSet<QString> vars = m_varScopes.pop();

    // Удаляем переменные из глобальной карты
    for (const QString& name : vars) {
        m_varMap.remove(name);
    }

    if (info.hasVariables) {
        // В области были переменные - генерируем SCOPEPOP
        emitCode(BydaoOpCode::ScopePop, 0, 0);
    } else {
        // В области не было переменных - генерируем SCOPEEND
        emitCode(BydaoOpCode::ScopeEnd, 0, 0);
    }
}

void BydaoParser::declareVariable(const QString& name, const BydaoToken& token) {
    if (m_scopeStack.isEmpty()) {
        error("No active scope for variable declaration: " + name);
        return;
    }

    // Проверяем, не объявлена ли уже в этой области
    if (m_varScopes.top().contains(name)) {
        error("Variable '" + name + "' already declared in this scope");
        return;
    }

    // Отмечаем, что в текущей области есть переменные
    if (!m_scopeStack.top().hasVariables) {
        // Первая переменная в этой области - меняем SCOPEBEG на SCOPEPUSH
        m_scopeStack.top().hasVariables = true;
        m_bytecode[m_scopeStack.top().beginInstrIndex].op = BydaoOpCode::ScopePush;
    }

    // Добавляем переменную в текущую область
    m_varScopes.top().insert(name);

    // Вычисляем индекс переменной (порядковый номер в области)
    int varIndex = m_varScopes.top().size() - 1;

    // Сохраняем в глобальной карте для быстрого поиска
    VariableInfo info;
    info.name = name;
    info.scopeDepth = m_scopeStack.top().scopeDepth;
    info.varIndex = varIndex;
    m_varMap[name] = info;

    // Генерируем VARDECL (имя переменной нужно только для отладки)
    qint16 nameIndex = addString(name);
    emitCode(BydaoOpCode::VarDecl, nameIndex, 0, token);
}

VariableInfo BydaoParser::resolveVariable(const QString& name) {
    auto it = m_varMap.find(name);
    if (it == m_varMap.end()) {
        error("Undeclared variable: " + name);
        return VariableInfo();
    }

    VariableInfo info = it.value();

    // Проверяем, что переменная всё ещё существует
    // (не была удалена при выходе из области)
    if (info.scopeDepth >= m_scopeStack.size()) {
        error("Variable '" + name + "' is out of scope");
        return VariableInfo();
    }

    return info;
}

bool BydaoParser::isVariableDeclared(const QString& name) {
    return m_varMap.contains(name);
}

// ============================================================
// ВСТРОЕННЫЕ ТИПЫ
// ============================================================

void BydaoParser::initBuiltinTypes() {
    BuiltinTypeInfo intInfo;
    intInfo.name = "Int";
    intInfo.methods["range"] = 2;
    intInfo.methods["parse"] = 1;
    intInfo.methods["max"] = 2;
    intInfo.methods["min"] = 2;
    intInfo.methods["random"] = -1;
    m_builtinTypes["Int"] = intInfo;

    BuiltinTypeInfo stringInfo;
    stringInfo.name = "String";
    stringInfo.methods["length"] = 0;
    stringInfo.methods["upper"] = 0;
    stringInfo.methods["lower"] = 0;
    m_builtinTypes["String"] = stringInfo;

    BuiltinTypeInfo arrayInfo;
    arrayInfo.name = "Array";
    arrayInfo.methods["length"] = 0;
    arrayInfo.methods["push"] = 1;
    arrayInfo.methods["pop"] = 0;
    m_builtinTypes["Array"] = arrayInfo;
}

bool BydaoParser::isBuiltinTypeName(const QString& name) const {
    return m_builtinTypes.contains(name);
}

// ============================================================
// МОДУЛИ
// ============================================================

void BydaoParser::addModulePath(const QString& path) {
    QString normalized = QDir::cleanPath(path);
    if (!m_modulePaths.contains(normalized)) {
        m_modulePaths.prepend(normalized);
    }
}

BydaoModuleInfo* BydaoParser::getModuleInfo(const QString& name) {
    return m_moduleInfoCache.value(name);
}

bool BydaoParser::isModule(const QString& name) {
    return m_moduleInfoCache.contains(name);
}

bool BydaoParser::loadModuleInfo(const QString& name) {
    if (m_moduleInfoCache.contains(name)) {
        return true;
    }

    QString errorMsg;
    BydaoModuleInfo* info = BydaoModuleManager::instance().loadModuleInfo(name, &errorMsg);
    if (info) {
        m_moduleInfoCache[name] = info;
        return true;
    }

    error("Cannot load module '" + name + "': " + errorMsg);
    return false;
}

// ============================================================
// ПАРСИНГ - ОСНОВНЫЕ МЕТОДЫ
// ============================================================

bool BydaoParser::parse() {
    // Глобальная область (без создания, только SCOPEBEG)
    enterScope();

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
    if (match(BydaoTokenType::Iter)) return parseIter();
    if (match(BydaoTokenType::Enum)) return parseEnum();
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

    enterScope();  // новая область для блока

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
    BydaoToken token = m_current;
    nextToken(); // var

    if (!match(BydaoTokenType::Identifier)) {
        error("Expected variable name");
        return false;
    }

    QString name = m_current.text;
    BydaoToken nameToken = m_current;
    declareVariable(name, nameToken);
    nextToken();

    if (match(BydaoTokenType::Assign)) {
        nextToken();
        if (!parseExpression()) return false;

        VariableInfo info = resolveVariable(name);
        emitCode(BydaoOpCode::Store, info.scopeDepth, info.varIndex, nameToken);
    } else {
        qint16 nullConst = addNullConstant();
        emitCode(BydaoOpCode::PushConst, nullConst, 0, token);

        VariableInfo info = resolveVariable(name);
        emitCode(BydaoOpCode::Store, info.scopeDepth, info.varIndex, nameToken);
    }

    return true;
}

bool BydaoParser::parseAssign() {
    QString name = m_current.text;
    BydaoToken nameToken = m_current;

    if (!isVariableDeclared(name)) {
        error("Undeclared variable: " + name);
        return false;
    }

    nextToken(); // identifier
    expect(BydaoTokenType::Assign); // =

    if (!parseExpression()) {
        error("Expected expression after =");
        return false;
    }

    VariableInfo info = resolveVariable(name);
    emitCode(BydaoOpCode::Store, info.scopeDepth, info.varIndex, nameToken);

    return true;
}

bool BydaoParser::parseIf() {
    BydaoToken ifToken = m_current;
    nextToken(); // if

    if (!parseExpression()) {
        error("Expected condition after 'if'");
        return false;
    }

    // Условный переход на else/elsif
    int elseJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, ifToken);

    if (!parseBlock(true)) {
        return false;
    }

    int endJump = emitCode(BydaoOpCode::Jump, 0, 0, ifToken);

    // Патчим условный переход на текущую позицию
    m_bytecode[elseJump].arg1 = m_bytecode.size();

    // Обрабатываем elsif
    while (match(BydaoTokenType::Elsif)) {
        BydaoToken elsifToken = m_current;
        nextToken(); // elsif

        if (!parseExpression()) {
            error("Expected condition after 'elsif'");
            return false;
        }

        int condJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, elsifToken);

        if (!parseBlock(true)) {
            return false;
        }

        emitCode(BydaoOpCode::Jump, endJump, 0);

        m_bytecode[condJump].arg1 = m_bytecode.size();
    }

    // Опциональный else
    if (match(BydaoTokenType::Else)) {
        nextToken(); // else

        if (!parseBlock(true)) {
            return false;
        }
    }

    // Патчим переход из if
    m_bytecode[endJump].arg1 = m_bytecode.size();

    return true;
}

bool BydaoParser::parseWhile() {
    BydaoToken token = m_current;
    nextToken(); // while

    LoopInfo loop;
    loop.conditionAddr = m_bytecode.size();

    if (!parseExpression()) {
        error("Expected condition");
        return false;
    }

    // Парсим next-оператор
    bool hasNext = match(BydaoTokenType::Next);
    QVector<BydaoInstruction> nextCode;

    if (hasNext) {
        nextToken(); // next

        int savedPos = m_bytecode.size();
        if (!parseStatement()) {
            error("Expected statement after next");
            return false;
        }

        // Сохраняем код next-оператора
        for (int i = savedPos; i < m_bytecode.size(); i++) {
            nextCode.append(m_bytecode[i]);
        }
        m_bytecode.resize(savedPos);
    }

    int condJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, token);

    m_loopStack.push(loop);
    m_inLoop = true;

    if (!parseBlock(true)) {
        m_inLoop = false;
        m_loopStack.pop();
        return false;
    }

    m_inLoop = false;
    loop = m_loopStack.pop();

    loop.nextAddr = m_bytecode.size();

    // Вставляем next-оператор
    for (const auto& instr : nextCode) {
        m_bytecode.append(instr);
    }

    emitCode(BydaoOpCode::Jump, loop.conditionAddr, 0);

    loop.exitAddr = m_bytecode.size();

    m_bytecode[condJump].arg1 = loop.exitAddr;
    patchJumps(loop.breaks, loop.exitAddr);
    patchJumps(loop.nexts, loop.nextAddr);

    return true;
}

bool BydaoParser::parseIter() {
    BydaoToken token = m_current;
    nextToken(); // iter

    if (!parseExpression()) {
        error("Expected collection");
        return false;
    }

    if (!expect(BydaoTokenType::As)) return false;

    if (!match(BydaoTokenType::Identifier)) {
        error("Expected iterator variable name");
        return false;
    }

    QString iterName = m_current.text;
    BydaoToken iterToken = m_current;
    nextToken();

    // Получаем итератор: collection.iter()
    qint16 iterMethodIdx = addString("iter");
    emitCode(BydaoOpCode::Member, iterMethodIdx, 0, token);
    emitCode(BydaoOpCode::Call, 0, 0, token);

    // Сохраняем итератор во временную переменную
    declareVariable(iterName, iterToken);  // объявляем в текущей области
    VariableInfo iterInfo = resolveVariable(iterName);
    emitCode(BydaoOpCode::Store, iterInfo.scopeDepth, iterInfo.varIndex, iterToken);

    int loopStart = m_bytecode.size();

    // iter.next()
    emitCode(BydaoOpCode::Load, iterInfo.scopeDepth, iterInfo.varIndex, iterToken);
    emitCode(BydaoOpCode::Next, 0, 0, token);

    int condJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, token);

    // Тело цикла - parseBlock сам создаст область видимости
    if (!parseBlock(true)) {
        return false;
    }

    emitCode(BydaoOpCode::Jump, loopStart, 0);

    // Патчим условный переход
    m_bytecode[condJump].arg1 = m_bytecode.size();

    // Очистка - удаляем переменную итератора
    emitCode(BydaoOpCode::Drop, iterInfo.scopeDepth, iterInfo.varIndex, iterToken);
    m_varMap.remove(iterName);  // удаляем из глобальной карты

    return true;
}

/*
 * enum = "enum" expr "as" var_name "{" block "}"
 *
 * expr
 * var _enum_iter_n = expr.iter()
 *
 */
bool BydaoParser::parseEnum() {
    BydaoToken token = m_current;
    nextToken(); // enum

    if (!parseExpression()) {
        error("Expected collection");
        return false;
    }

    if (!expect(BydaoTokenType::As)) return false;

    if (!match(BydaoTokenType::Identifier)) {
        error("Expected variable name");
        return false;
    }

    QString varName = m_current.text;
    BydaoToken varToken = m_current;
    nextToken();

    // Объявляем переменную для значения
    declareVariable(varName, varToken);  // объявляем в текущей области
    VariableInfo varInfo = resolveVariable(varName);

    // Получаем итератор: collection.iter()
    qint16 iterMethodIdx = addString("iter");
    emitCode(BydaoOpCode::Member, iterMethodIdx, 0, token);
    emitCode(BydaoOpCode::Call, 0, 0, token);

    // Создаём временный итератор
    QString tmpIterName = QString("__enum_iter_%1").arg(m_iteratorCounter++);
    declareVariable(tmpIterName, token);  // объявляем в текущей области
    VariableInfo iterInfo = resolveVariable(tmpIterName);
    emitCode(BydaoOpCode::Store, iterInfo.scopeDepth, iterInfo.varIndex, token);

    int loopStart = m_bytecode.size();

    // iter.next()
    emitCode(BydaoOpCode::Load, iterInfo.scopeDepth, iterInfo.varIndex, token);
    emitCode(BydaoOpCode::Next, 0, 0, token);

    int condJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, token);

    // Получаем значение и сохраняем в переменную
    emitCode(BydaoOpCode::Load, iterInfo.scopeDepth, iterInfo.varIndex, token);
    emitCode(BydaoOpCode::Value, 0, 0, token);
    emitCode(BydaoOpCode::Store, varInfo.scopeDepth, varInfo.varIndex, varToken);

    // Тело цикла - parseBlock сам создаст область видимости
    if (!parseBlock(true)) {
        return false;
    }

    emitCode(BydaoOpCode::Jump, loopStart, 0);

    // Патчим условный переход
    m_bytecode[condJump].arg1 = m_bytecode.size();

    // Очистка - удаляем временный итератор
    emitCode(BydaoOpCode::Drop, iterInfo.scopeDepth, iterInfo.varIndex, token);
    m_varMap.remove(tmpIterName);

    // Удаляем переменную цикла
    emitCode(BydaoOpCode::Drop, varInfo.scopeDepth, varInfo.varIndex, varToken);
    m_varMap.remove(varName);

    return true;
}

bool BydaoParser::parseBreakNext() {
    if (m_loopStack.isEmpty()) {
        error("break/next outside loop");
        return false;
    }

    LoopInfo& loop = m_loopStack.top();

    if (match(BydaoTokenType::Break)) {
        int jumpInstr = emitCode(BydaoOpCode::Jump, 0, 0, m_current);
        loop.breaks.append(jumpInstr);
        nextToken();
        return true;
    } else if (match(BydaoTokenType::Next)) {
        int jumpInstr = emitCode(BydaoOpCode::Jump, 0, 0, m_current);
        loop.nexts.append(jumpInstr);
        nextToken();
        return true;
    }

    return false;
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

    if (!loadModuleInfo(moduleName)) {
        return false;
    }

    // Сохраняем алиас в карту модулей
    m_moduleInfoCache[alias] = getModuleInfo(moduleName);

    // Добавляем переменную в текущую область
    m_varScopes.top().insert(alias);

    // Вычисляем индекс переменной (порядковый номер в области)
    int varIndex = m_varScopes.top().size() - 1;

    // Сохраняем в глобальной карте для быстрого поиска
    VariableInfo info;
    info.name = alias;
    info.scopeDepth = m_scopeStack.top().scopeDepth;
    info.varIndex = varIndex;
    m_varMap[alias] = info;

    // Генерируем код
    qint16 moduleNameIdx = addString(moduleName);
    qint16 aliasIdx = addString(alias);
    emitCode(BydaoOpCode::UseModule, moduleNameIdx, aliasIdx, token);

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
        emitCode(BydaoOpCode::Or, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseLogicalAnd() {
    if (!parseEquality()) return false;

    while (match(BydaoTokenType::And)) {
        BydaoToken op = m_current;
        nextToken();
        if (!parseEquality()) return false;
        emitCode(BydaoOpCode::And, 0, 0, op);
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
        emitCode(isEq ? BydaoOpCode::Eq : BydaoOpCode::Neq, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseComparison() {
    if (!parseAddition()) return false;

    while (match(BydaoTokenType::Less) || match(BydaoTokenType::Greater) ||
           match(BydaoTokenType::LessEqual) || match(BydaoTokenType::GreaterEqual)) {
        BydaoOpCode opCode;
        switch (m_current.type) {
        case BydaoTokenType::Less: opCode = BydaoOpCode::Lt; break;
        case BydaoTokenType::Greater: opCode = BydaoOpCode::Gt; break;
        case BydaoTokenType::LessEqual: opCode = BydaoOpCode::Le; break;
        case BydaoTokenType::GreaterEqual: opCode = BydaoOpCode::Ge; break;
        default: return false;
        }

        BydaoToken op = m_current;
        nextToken();
        if (!parseAddition()) return false;
        emitCode(opCode, 0, 0, op);
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
        emitCode(isPlus ? BydaoOpCode::Add : BydaoOpCode::Sub, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseTerm() {
    if (!parseUnary()) return false;

    while (match(BydaoTokenType::Mul) || match(BydaoTokenType::Div) || match(BydaoTokenType::Mod)) {
        BydaoOpCode opCode;
        switch (m_current.type) {
        case BydaoTokenType::Mul: opCode = BydaoOpCode::Mul; break;
        case BydaoTokenType::Div: opCode = BydaoOpCode::Div; break;
        case BydaoTokenType::Mod: opCode = BydaoOpCode::Mod; break;
        default: return false;
        }

        BydaoToken op = m_current;
        nextToken();
        if (!parseUnary()) return false;
        emitCode(opCode, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseUnary() {
    if (match(BydaoTokenType::Not) || match(BydaoTokenType::Minus)) {
        bool isNot = match(BydaoTokenType::Not);
        BydaoToken op = m_current;
        nextToken();
        if (!parseUnary()) return false;
        emitCode(isNot ? BydaoOpCode::Not : BydaoOpCode::Neg, 0, 0, op);
        return true;
    }
    return parsePrimary();
}

bool BydaoParser::parsePrimary() {
    if (match(BydaoTokenType::Identifier)) {
        QString name = m_current.text;
        BydaoToken nameToken = m_current;

        if (isBuiltinTypeName(name)) {
            nextToken();
            if (match(BydaoTokenType::Dot)) {
                m_pos--;
                m_current = m_tokens[m_pos];
                return parseMember(true);
            }
            qint16 typeIdx = addString(name);
            emitCode(BydaoOpCode::TypeClass, typeIdx, 0, nameToken);
            return true;
        }

        if (isModule(name)) {
            nextToken();
            if (match(BydaoTokenType::Dot)) {
                m_pos--;
                m_current = m_tokens[m_pos];
                return parseMember(true);
            }
            // Используем модуль как значение
            VariableInfo info = resolveVariable(name);
            if (info.scopeDepth >= 0) {
                emitCode(BydaoOpCode::Load, info.scopeDepth, info.varIndex, nameToken);
                return true;
            }
            error("Module not loaded: " + name);
            return false;
        }

        if (isVariableDeclared(name)) {
            nextToken();
            if (match(BydaoTokenType::Dot) || match(BydaoTokenType::LBracket)) {
                m_pos--;
                m_current = m_tokens[m_pos];
                return parseMember(true);
            }

            VariableInfo info = resolveVariable(name);
            emitCode(BydaoOpCode::Load, info.scopeDepth, info.varIndex, nameToken);
            return true;
        }

        error("Undeclared variable: " + name);
        return false;
    }

    if (match(BydaoTokenType::Number)) {
        QString text = m_current.text;
        qint16 constIndex;

        if (text.contains('.') || text.contains('e') || text.contains('E')) {
            constIndex = addConstant(text.toDouble());
        } else {
            constIndex = addConstant(text.toLongLong());
        }

        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        nextToken();
        return true;
    }

    if (match(BydaoTokenType::String)) {
        QString text = m_current.text;
        if (text.length() >= 2 && (text.startsWith('\'') || text.startsWith('"'))) {
            text = text.mid(1, text.length() - 2);
        }
        qint16 constIndex = addConstant(text);
        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
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

    if (match(BydaoTokenType::False)) {
        qint16 constIndex = addConstant(false);
        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        nextToken();
        return true;
    }

    if (match(BydaoTokenType::True)) {
        qint16 constIndex = addConstant(true);
        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        nextToken();
        return true;
    }

    if (match(BydaoTokenType::Null)) {
        qint16 constIndex = addNullConstant();
        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        nextToken();
        return true;
    }

    return false;
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
            qint16 memberIdx = addString(member);

            nextToken();

            if (match(BydaoTokenType::LParen)) {
                // Вызов метода
                if (isBuiltinTypeName(object)) {
                    qint16 typeIdx = addString(object);
                    emitCode(BydaoOpCode::TypeClass, typeIdx, 0, objToken);
                }
                // else if (isModule(object)) {
                //     qint16 typeIdx = addString(object);
                //     emitCode(BydaoOpCode::TypeClass, typeIdx, 0, objToken);
                // }
                else {
                    VariableInfo info = resolveVariable(object);
                    emitCode(BydaoOpCode::Load, info.scopeDepth, info.varIndex, objToken);
                }

                emitCode(BydaoOpCode::Member, memberIdx, 0, memberToken);

                if (!parseCall()) {
                    return false;
                }
            } else {
                // Доступ к свойству
                if (isBuiltinTypeName(object)) {
                    error("Type '" + object + "' has no property '" + member + "'");
                    return false;
                } else {
                    VariableInfo info = resolveVariable(object);
                    emitCode(BydaoOpCode::Load, info.scopeDepth, info.varIndex, objToken);
                    emitCode(BydaoOpCode::Member, memberIdx, 0, memberToken);
                }
            }

            object = "_acc";
        }
        else if (match(BydaoTokenType::LBracket)) {
            nextToken();
            if (!parseExpression()) return false;
            if (!expect(BydaoTokenType::RBracket)) return false;

            VariableInfo info = resolveVariable(object);
            emitCode(BydaoOpCode::Load, info.scopeDepth, info.varIndex, objToken);
            emitCode(BydaoOpCode::Index, 0, 0, objToken);
            object = "_acc";
        }
    }

    // Обработка присваивания (obj.member = value)
    if (canAssign && match(BydaoTokenType::Assign)) {
        if (isModule(object) || isBuiltinTypeName(object)) {
            error("Cannot assign to module or type");
            return false;
        }

        BydaoToken assignToken = m_current;
        nextToken();
        if (!parseExpression()) return false;

        // TODO: реализовать присваивание свойству
        error("Property assignment not implemented yet");
        return false;
    }

    return true;
}

bool BydaoParser::parseCall() {
    if (!expect(BydaoTokenType::LParen)) return false;

    int argCount = 0;

    if (!match(BydaoTokenType::RParen)) {
        do {
            if (!parseExpression()) return false;
            argCount++;
        } while (match(BydaoTokenType::Comma) && (nextToken(), true));
    }

    if (!expect(BydaoTokenType::RParen)) return false;

    emitCode(BydaoOpCode::Call, argCount, 0);

    return true;
}

bool BydaoParser::parseArrayLiteral() {
    BydaoToken token = m_current;
    nextToken(); // [

    int elementCount = 0;

    if (!match(BydaoTokenType::RBracket)) {
        do {
            if (!parseExpression()) return false;
            elementCount++;
        } while (match(BydaoTokenType::Comma) && (nextToken(), true));

        // Разрешаем висящую запятую
        if (match(BydaoTokenType::Comma)) {
            nextToken();
        }
    }

    if (!expect(BydaoTokenType::RBracket)) return false;

    // Генерируем PushArray с количеством элементов
    // Элементы уже на стеке в правильном порядке (благодаря parseExpression)
    emitCode(BydaoOpCode::PushArray, elementCount, 0, token);

    return true;
}

} // namespace BydaoScript
