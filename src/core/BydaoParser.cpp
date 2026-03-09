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
#include "BydaoScript/BydaoParser.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

namespace BydaoScript {

// ============================================================
// КОНСТРУКТОР / ДЕСТРУКТОР
// ============================================================

BydaoParser::BydaoParser(const QVector<BydaoToken>& tokens)
    : m_tokens(tokens)
    , m_pos(0)
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
    // Генерируем SCOPEPUSH
    emitCode(BydaoOpCode::ScopePush, 0, 0);

    // Добавляем пустой набор переменных в новую область видимости
    m_varScopes.push( ScopeItem() );
}

void BydaoParser::exitScope() {

    // Забираем набор переменных в текущей области видимости
    ScopeItem item = m_varScopes.pop();

    // В области были переменные - генерируем SCOPEPOP
    emitCode(BydaoOpCode::ScopePop, 0, 0);
}

bool BydaoParser::appendVariable(const QString& name, bool isConstant ) {

//    qDebug() << "appendVariable:" << name << "isConstant =" << isConstant;  // ОТЛАДКА

    // Проверяем, не объявлена ли уже в этой области
    if (m_varScopes.top().varInfo.contains(name)) {
        error("Variable '" + name + "' already declared in this scope");
        return false;
    }

    // Индекс области видимости
    int scopeIndex = m_varScopes.size() - 1;

    // Вычисляем индекс переменной (порядковый номер в области)
    int varIndex = m_varScopes.top().varList.size();

    // Сохраняем в глобальной карте для быстрого поиска
    VariableInfo info;
    info.name = name;
    info.scopeDepth = scopeIndex;
    info.varIndex = varIndex;
    info.isConstant = isConstant;
    m_varScopes.top().varList.insert( name );
    m_varScopes.top().varInfo[name] = info;

    return true;
}

void BydaoParser::declareVariable(const QString& name, const BydaoToken& token) {

    if ( appendVariable( name, false ) ) {

        // Генерируем VARDECL (имя переменной нужно только для отладки)
        qint16 nameIndex = addString(name);
        emitCode(BydaoOpCode::VarDecl, nameIndex, 0, token);
    }
}

VariableInfo BydaoParser::resolveVariable(const QString& name) {

    int stackSize = m_varScopes.size();
    for ( int i = stackSize-1; i >= 0; --i ) {
        if ( m_varScopes[ i ].varInfo.contains( name ) ) {
            return m_varScopes[ i ].varInfo[name];
        }
    }
    return VariableInfo();
}

bool BydaoParser::isVariableDeclared(const QString& name) {
    int stackSize = m_varScopes.size();
    for ( int i = stackSize-1; i >= 0; --i ) {
        if ( m_varScopes[ i ].varInfo.contains( name ) ) {
            return true;
        }
    }
    return false;
}

bool BydaoParser::removeVariable(const QString& name ) {
    int stackSize = m_varScopes.size();
    for ( int i = stackSize-1; i >= 0; --i ) {
        if ( m_varScopes[ i ].varInfo.contains( name ) ) {
            m_varScopes[ i ].varList.remove( name );
            m_varScopes[ i ].varInfo.remove( name );
            return true;
        }
    }
    return false;
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

bool BydaoParser::isNameToken(BydaoTokenType type) const {
    return type == BydaoTokenType::Identifier ||
           type == BydaoTokenType::Iter ||
           type == BydaoTokenType::Enum ||
           type == BydaoTokenType::While ||
           type == BydaoTokenType::If ||
           type == BydaoTokenType::Else ||
           type == BydaoTokenType::Break ||
           type == BydaoTokenType::Next ||
           type == BydaoTokenType::Var ||
           type == BydaoTokenType::Drop ||
           type == BydaoTokenType::Use ||
           type == BydaoTokenType::As;
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
    // Проверяем присваивание
    // identifier = expression
    // identifier += expression
    if ( match(BydaoTokenType::Identifier) ) {
        if ( peek(BydaoTokenType::Assign, 1) ) {
            return parseAssign();
        }
        if ( peek(BydaoTokenType::PlusAssign, 1) ) {
            return parseAddAssign();
        }
        if ( peek(BydaoTokenType::MinusAssign, 1) ) {
            return parseSubAssign();
        }
        if ( peek(BydaoTokenType::MulAssign, 1) ) {
            return parseMulAssign();
        }
        if ( peek(BydaoTokenType::DivAssign, 1) ) {
            return parseDivAssign();
        }
        if ( peek(BydaoTokenType::ModAssign, 1) ) {
            return parseModAssign();
        }
    }

    if (match(BydaoTokenType::Var)) return parseVarDecl();
    if (match(BydaoTokenType::Const)) return parseConstDecl();
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

bool BydaoParser::parseConstDecl() {
//    qDebug() << "parseConstDecl: start at line" << m_current.line;
    BydaoToken token = m_current;
    nextToken(); // const

    if (!match(BydaoTokenType::Identifier)) {
        error("Expected constant name");
        return false;
    }

    QString name = m_current.text;
    BydaoToken nameToken = m_current;
    nextToken();

    if (!match(BydaoTokenType::Assign)) {
        error("Expected '=' in constant declaration");
        return false;
    }
    nextToken(); // '='

    // Добавляем константу в таблицу (без значения)
    if (!appendVariable(name, true)) {
        return false;
    }

    // Пытаемся вычислить выражение
    BydaoConstantFolder folder(this);
    BydaoValue constValue = folder.evaluate();  // Один проход!

    if (constValue.isNull()) {
        // Если получили null - значит выражение не константное
        removeVariable(name);
        error("Constant expression expected");
        return false;
    }

    // Обновляем значение в таблице
    VariableInfo info = resolveVariable(name);
    info.constValue = constValue;
    m_varScopes.top().varInfo[name] = info;

    // Генерируем код
    qint16 nameIndex = addString(name);
//    qDebug() << "  generating VarDecl for" << name << "index =" << nameIndex;
    emitCode(BydaoOpCode::VarDecl, nameIndex, 0, token);

    // Добавляем значение в таблицу констант и генерируем PUSH
    qint16 constIndex;
    switch (constValue.typeId()) {
    case TYPE_INT:
        constIndex = addConstant(constValue.toInt());
        break;
    case TYPE_REAL:
        constIndex = addConstant(constValue.toReal());
        break;
    case TYPE_BOOL:
        constIndex = addConstant(constValue.toBool());
        break;
    case TYPE_STRING:
        constIndex = addConstant(constValue.toString());
        break;
    default:
        constIndex = addNullConstant();
        break;
    }
//    qDebug() << "  generating PushConst for value" << constValue.toString() << "index =" << constIndex;
    emitCode(BydaoOpCode::PushConst, constIndex, 0, token);

//    qDebug() << "  generating Store for" << name;
    emitCode(BydaoOpCode::Store, info.scopeDepth, info.varIndex, nameToken);

    return true;
}

bool BydaoParser::parseAssign() {
    QString name = m_current.text;
    BydaoToken nameToken = m_current;

    if (!isVariableDeclared(name)) {
        error("Undeclared variable: " + name);
        return false;
    }

    // ПРОВЕРКА НА КОНСТАНТУ
    VariableInfo info = resolveVariable(name);
    if (info.isConstant) {
        error("Cannot assign to constant: " + name);
        return false;
    }

    nextToken(); // identifier
    if ( ! expect( BydaoTokenType::Assign ) ) {
        error("Unexpected expression after identifier");
        return false;
    }

    if (!parseExpression()) {
        error("Expected expression after =");
        return false;
    }

    emitCode(BydaoOpCode::Store, info.scopeDepth, info.varIndex, nameToken);

    return true;
}

bool BydaoParser::parseAddAssign() {
    QString name = m_current.text;
    BydaoToken nameToken = m_current;

    if (!isVariableDeclared(name)) {
        error("Undeclared variable: " + name);
        return false;
    }

    // ПРОВЕРКА НА КОНСТАНТУ
    VariableInfo info = resolveVariable(name);
    if (info.isConstant) {
        error("Cannot assign to constant: " + name);
        return false;
    }

    nextToken(); // identifier
    if ( ! expect( BydaoTokenType::PlusAssign ) ) {
        error("Unexpected expression after identifier");
        return false;
    }

    if (!parseExpression()) {
        error("Expected expression after =");
        return false;
    }

    emitCode(BydaoOpCode::AddStore, info.scopeDepth, info.varIndex, nameToken);

    return true;
}

bool BydaoParser::parseSubAssign() {
    QString name = m_current.text;
    BydaoToken nameToken = m_current;

    if (!isVariableDeclared(name)) {
        error("Undeclared variable: " + name);
        return false;
    }

    // ПРОВЕРКА НА КОНСТАНТУ
    VariableInfo info = resolveVariable(name);
    if (info.isConstant) {
        error("Cannot assign to constant: " + name);
        return false;
    }

    nextToken(); // identifier
    if ( ! expect( BydaoTokenType::MinusAssign ) ) {
        error("Unexpected expression after identifier");
        return false;
    }

    if (!parseExpression()) {
        error("Expected expression after =");
        return false;
    }

    emitCode(BydaoOpCode::SubStore, info.scopeDepth, info.varIndex, nameToken);

    return true;
}

bool BydaoParser::parseMulAssign() {
    QString name = m_current.text;
    BydaoToken nameToken = m_current;

    if (!isVariableDeclared(name)) {
        error("Undeclared variable: " + name);
        return false;
    }

    // ПРОВЕРКА НА КОНСТАНТУ
    VariableInfo info = resolveVariable(name);
    if (info.isConstant) {
        error("Cannot assign to constant: " + name);
        return false;
    }

    nextToken(); // identifier
    if ( ! expect( BydaoTokenType::MulAssign ) ) {
        error("Unexpected expression after identifier");
        return false;
    }

    if (!parseExpression()) {
        error("Expected expression after =");
        return false;
    }

    emitCode(BydaoOpCode::MulStore, info.scopeDepth, info.varIndex, nameToken);

    return true;
}

bool BydaoParser::parseDivAssign() {
    QString name = m_current.text;
    BydaoToken nameToken = m_current;

    if (!isVariableDeclared(name)) {
        error("Undeclared variable: " + name);
        return false;
    }

    // ПРОВЕРКА НА КОНСТАНТУ
    VariableInfo info = resolveVariable(name);
    if (info.isConstant) {
        error("Cannot assign to constant: " + name);
        return false;
    }

    nextToken(); // identifier
    if ( ! expect( BydaoTokenType::DivAssign ) ) {
        error("Unexpected expression after identifier");
        return false;
    }

    if (!parseExpression()) {
        error("Expected expression after =");
        return false;
    }

    emitCode(BydaoOpCode::DivStore, info.scopeDepth, info.varIndex, nameToken);

    return true;
}

bool BydaoParser::parseModAssign() {
    QString name = m_current.text;
    BydaoToken nameToken = m_current;

    if (!isVariableDeclared(name)) {
        error("Undeclared variable: " + name);
        return false;
    }

    // ПРОВЕРКА НА КОНСТАНТУ
    VariableInfo info = resolveVariable(name);
    if (info.isConstant) {
        error("Cannot assign to constant: " + name);
        return false;
    }

    nextToken(); // identifier
    if ( ! expect( BydaoTokenType::ModAssign ) ) {
        error("Unexpected expression after identifier");
        return false;
    }

    if (!parseExpression()) {
        error("Expected expression after =");
        return false;
    }

    emitCode(BydaoOpCode::ModStore, info.scopeDepth, info.varIndex, nameToken);

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
    // qint16 iterMethodIdx = addString("iter");
    // emitCode(BydaoOpCode::Method, iterMethodIdx, 0, token);
    // emitCode(BydaoOpCode::Call, 0, 0, token);
    emitCode(BydaoOpCode::GetIter, 0, 0, token);

    // Сохраняем итератор во временную переменную
    declareVariable(iterName, iterToken);  // объявляем в текущей области
    VariableInfo iterInfo = resolveVariable(iterName);
    emitCode(BydaoOpCode::Store, iterInfo.scopeDepth, iterInfo.varIndex, iterToken);

    int loopStart = m_bytecode.size();

    // iter.next()
    emitCode(BydaoOpCode::Load, iterInfo.scopeDepth, iterInfo.varIndex, iterToken);
    emitCode(BydaoOpCode::ItNext, 0, 0, token);

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
    removeVariable(iterName);

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
    // qint16 iterMethodIdx = addString("iter");
    // emitCode(BydaoOpCode::Method, iterMethodIdx, 0, token);
    // emitCode(BydaoOpCode::Call, 0, 0, token);
    emitCode(BydaoOpCode::GetIter, 0, 0, token);

    // Создаём временный итератор
    QString tmpIterName = QString("__enum_iter_%1").arg(m_iteratorCounter++);
    declareVariable(tmpIterName, token);  // объявляем в текущей области
    VariableInfo iterInfo = resolveVariable(tmpIterName);
    emitCode(BydaoOpCode::Store, iterInfo.scopeDepth, iterInfo.varIndex, token);

    int loopStart = m_bytecode.size();

    // iter.next()
    emitCode(BydaoOpCode::Load, iterInfo.scopeDepth, iterInfo.varIndex, token);
    emitCode(BydaoOpCode::ItNext, 0, 0, token);

    int condJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, token);

    // Получаем значение и сохраняем в переменную
    emitCode(BydaoOpCode::Load, iterInfo.scopeDepth, iterInfo.varIndex, token);
    emitCode(BydaoOpCode::ItValue, 0, 0, token);
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
    removeVariable(tmpIterName);

    // Удаляем переменную цикла
    emitCode(BydaoOpCode::Drop, varInfo.scopeDepth, varInfo.varIndex, varToken);
    removeVariable(varName);

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
    nextToken(); // next after use

    do {

        if (!match(BydaoTokenType::Identifier)) {
            error("Expected module name");
            return false;
        }

        QString moduleName = m_current.text;
        nextToken();

        QString alias = moduleName;
        if (match(BydaoTokenType::As)) {    // есть "as"
            nextToken();
            if (!match(BydaoTokenType::Identifier)) {
                error("Expected alias name");
                return false;
            }
            alias = m_current.text;         // получить алиас
            nextToken();
        }

        if (!loadModuleInfo(moduleName)) {
            return false;
        }

        // Сохраняем алиас в карту модулей
        m_moduleInfoCache[alias] = getModuleInfo(moduleName);

        // Добавляем переменную в текущую область видимости
        if ( ! appendVariable( alias ) ) {
            return false;
        }

        // Генерируем код
        qint16 moduleNameIdx = addString(moduleName);
        qint16 aliasIdx = addString(alias);
        emitCode(BydaoOpCode::UseModule, moduleNameIdx, aliasIdx, token);

        // Если нет запятой - заканчиваем
        if ( ! match(BydaoTokenType::Comma) ) {
            break;
        }
        nextToken(); // пропускаем запятую
    } while ( true );

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

bool BydaoParser::parsePrimaryBase() {
    // Убираем всю обработку идентификаторов
    if (match(BydaoTokenType::Identifier)) {
        // Идентификаторы теперь обрабатываются в новом parsePrimary()
        error("Identifier should be handled in parsePrimary()");
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

bool BydaoParser::parsePrimary() {
    // Запоминаем позицию для возможного восстановления
    int savedPos = m_pos;
    BydaoToken savedToken = m_current;

    // Обрабатываем идентификаторы (переменные, типы, модули)
    if (match(BydaoTokenType::Identifier)) {
        QString name = m_current.text;
        BydaoToken nameToken = m_current;

        // Определяем тип идентификатора
        if (isBuiltinTypeName(name)) {
            // Встроенный тип: Int, String, Array и т.д.
            nextToken(); // съедаем имя типа
            qint16 typeIdx = addString(name);
            emitCode(BydaoOpCode::TypeClass, typeIdx, 0, nameToken);

//            qDebug() << "parsePrimary: builtin type" << name;
        }
        else if (isModule(name)) {
            // Модуль (загружен через use)
            if (!isVariableDeclared(name)) {
                error("Module not loaded: " + name);
                return false;
            }
            nextToken(); // съедаем имя модуля
            VariableInfo info = resolveVariable(name);
            emitCode(BydaoOpCode::Load, info.scopeDepth, info.varIndex, nameToken);

//            qDebug() << "parsePrimary: module" << name;
        }
        else if (isVariableDeclared(name)) {
            // Обычная переменная
            nextToken(); // съедаем имя переменной
            VariableInfo info = resolveVariable(name);
            emitCode(BydaoOpCode::Load, info.scopeDepth, info.varIndex, nameToken);

//            qDebug() << "parsePrimary: variable" << name;
        }
        else {
            error("Undeclared identifier: " + name);
            return false;
        }
    }
    else {
        // Не идентификатор - пробуем другие варианты (числа, строки, массивы, скобки)
        if (!parsePrimaryBase()) {
            // Если ничего не подошло - восстанавливаем позицию и возвращаем false
            m_pos = savedPos;
            m_current = savedToken;
            return false;
        }
    }

    // После того как получили базовое значение,
    // обрабатываем цепочку вызовов и доступов
    while (true) {
        if (match(BydaoTokenType::LParen)) {
//            qDebug() << "parsePrimary: found call suffix";
            if (!parseCallSuffix()) return false;
        }
        else if (match(BydaoTokenType::Dot)) {
//            qDebug() << "parsePrimary: found member suffix";
            if (!parseMemberSuffix()) return false;
        }
        else if (match(BydaoTokenType::LBracket)) {
//            qDebug() << "parsePrimary: found index suffix";
            if (!parseIndexSuffix()) return false;
        }
        else {
            break;
        }
    }

    return true;
}

bool BydaoParser::parseCallSuffix() {
    // Текущий токен должен быть LParen
    if (!match(BydaoTokenType::LParen)) {
        error("Expected '(' in call");
        return false;
    }

    BydaoToken token = m_current;
//    qDebug() << "parseCallSuffix at line" << token.line << "," << token.column;

    int argCount = 0;

    // Пропускаем '('
    nextToken();

    // Парсим аргументы, если они есть
    if (!match(BydaoTokenType::RParen)) {
        do {
            if (!parseExpression()) {
                error("Expected expression in argument list");
                return false;
            }
            argCount++;

            // Если после выражения нет запятой - заканчиваем
            if ( ! match(BydaoTokenType::Comma) ) {
                break;
            }
            nextToken(); // пропускаем запятую
        } while (true);
    }

    // Ожидаем закрывающую скобку
    if (!expect(BydaoTokenType::RParen)) {
        error("Expected ')' after arguments");
        return false;
    }

    // Генерируем инструкцию CALL
    // Объект для вызова уже лежит на стеке (результат предыдущих операций)
    // Аргументы лежат на стеке в правильном порядке (благодаря parseExpression)
//    qDebug() << "parseCallSuffix: emitting CALL with" << argCount << "arguments";
    emitCode(BydaoOpCode::Call, argCount, 0, token);

    return true;
}

bool BydaoParser::parseMemberSuffix() {
    // Текущий токен должен быть Dot
    if (!match(BydaoTokenType::Dot)) {
        error("Expected '.' in member access");
        return false;
    }

    BydaoToken dotToken = m_current;
//    qDebug() << "parseMemberSuffix at line" << dotToken.line << "," << dotToken.column;

    // Пропускаем '.'
    nextToken();

    // После точки должно быть имя члена (идентификатор или ключевое слово)
    if (!match(BydaoTokenType::Identifier) && !isNameToken(m_current.type)) {
        qDebug() << "  Current token:" << m_current.text << "type:" << (int)m_current.type;
        error("Expected member name after '.'");
        return false;
    }

    QString memberName = m_current.text;
    BydaoToken memberToken = m_current;
//    qDebug() << "parseMemberSuffix: member name =" << memberName;

    // Добавляем имя члена в таблицу строк
    qint16 memberIdx = addString(memberName);

    // Пропускаем имя члена
    nextToken();

    // Смотрим, что идёт после имени члена
    if (match(BydaoTokenType::LParen)) {
        // Это вызов метода - используем METHOD
//        qDebug() << "  -> method call, emitting METHOD";
        emitCode(BydaoOpCode::Method, memberIdx, 0, memberToken);
    } else {
        // Это доступ к свойству - используем MEMBER
//        qDebug() << "  -> property access, emitting MEMBER";
        emitCode(BydaoOpCode::Member, memberIdx, 0, memberToken);
    }

    return true;
}

bool BydaoParser::parseIndexSuffix() {
    // Текущий токен должен быть LBracket
    if (!match(BydaoTokenType::LBracket)) {
        error("Expected '[' in index access");
        return false;
    }

    BydaoToken bracketToken = m_current;
//    qDebug() << "parseIndexSuffix at line" << bracketToken.line << "," << bracketToken.column;

    // Пропускаем '['
    nextToken();

    // Парсим выражение-индекс
    if (!parseExpression()) {
        error("Expected expression inside '[]'");
        return false;
    }

    // Ожидаем закрывающую скобку
    if (!expect(BydaoTokenType::RBracket)) {
        error("Expected ']' after index expression");
        return false;
    }

    // Генерируем инструкцию INDEX
    // Объект для индексации уже лежит на стеке (результат предыдущих операций)
    // Индекс тоже уже на стеке (результат parseExpression)
    emitCode(BydaoOpCode::Index, 0, 0, bracketToken);

//    qDebug() << "parseIndexSuffix: emitted INDEX";

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
