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

#include <QCoreApplication>
#include <QDir>
#include <QDebug>

#include "BydaoScript/BydaoParser.h"
#include "BydaoScript/BydaoIntClass.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoNull.h"

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
    if ( ! m_tokens.isEmpty() ) {
        m_current = m_tokens[0];
    }

    // Добавляем пути поиска модулей
    m_modulePaths << ".";
    m_modulePaths << "./modules";
    m_modulePaths << QCoreApplication::applicationDirPath();
    m_modulePaths << QCoreApplication::applicationDirPath() + "/modules";

    // Добавляем пустую строку под индексом 0
    addString("");

    // Инициализировать встроенные типы данных

    m_metaData["Int"] = new MetaData( BydaoIntClass::metaData() );
    UsedMetaDataList usedIntList = BydaoIntClass::usedMetaData();
    foreach ( const UsedMetaData& used, usedIntList ) {
        if ( ! m_metaData.contains( used.type ) ) {
            m_metaData[ used.type ] = new MetaData( used.metaData );
        }
    }

    m_metaData["Real"] = new MetaData( BydaoReal::metaData() );

    m_metaData["String"] = new MetaData( BydaoString::metaData() );

    m_metaData["Bool"] = new MetaData( BydaoBool::metaData() );

    m_metaData["Null"] = new MetaData( BydaoNull::metaData() );
}

BydaoParser::~BydaoParser() {
    qDeleteAll( m_moduleInfoCache );
    qDeleteAll( m_metaData );
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
    }
    else {
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
    m_errors.append( QString("%1 at %2:%3").arg(msg).arg(m_current.line).arg(m_current.column) );
}

void BydaoParser::error(const QString& msg, const BydaoToken& token) {
    m_errors.append( QString("%1 at %2:%3").arg(msg).arg(token.line).arg(token.column) );
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
    m_scopeStart.push( m_varScopes.size() );
}

void BydaoParser::exitScope() {
    if ( m_varScopes.size() > m_scopeStart.top() ) {
        m_varScopes.resize( m_scopeStart.top() );
        emitCode( BydaoOpCode::ScopeDrop, m_scopeStart.top() );
    }
    m_scopeStart.pop();
}

bool BydaoParser::appendVariable(const QString& name, bool isConstant, bool isPublic ) {

//    qDebug() << "appendVariable:" << name << "isConstant =" << isConstant;  // ОТЛАДКА

    // Проверяем, не объявлена ли уже в этой области

    int scopeStart = m_scopeStart.top();
    int index = m_varScopes.size();
    while ( --index >= scopeStart ) {
        if ( m_varScopes[ index ].name == name ) {
            error("Variable '" + name + "' already declared in this scope");
            return false;
        }
    }

    // Добавляем в список переменных

    VariableInfo info;
    info.name = name;
    info.type = "Null";
    info.varIndex = m_varScopes.size();
    info.isConstant = isConstant;
    info.isPublic = isPublic;

    m_varScopes.append( ScopeItem( name, info ) );

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

    int index = m_varScopes.size();
    while ( --index >= 0 ) {
        if ( m_varScopes[ index ].name == name ) {
            return m_varScopes[ index ].varInfo;
        }
    }
    return VariableInfo();
}

bool BydaoParser::isVariableDeclared(const QString& name) {

    int index = m_varScopes.size();
    while ( --index >= 0 ) {
        if ( m_varScopes[ index ].name == name ) {
            return true;
        }
    }
    return false;
}

bool BydaoParser::removeVariable(const QString& name ) {

    int index = m_varScopes.size();
    while ( --index >= 0 ) {
        if ( m_varScopes[ index ].name == name ) {
            m_varScopes.removeAt( index );
            return true;
        }
    }
    return false;
}

void BydaoParser::setVariableType(const QString& name, const QString type, bool isConst) {

    int index = m_varScopes.size();
    while ( --index >= 0 ) {
        if ( m_varScopes[ index ].name == name ) {
            m_varScopes[ index ].varInfo.type = type;
            m_varScopes[ index ].varInfo.isConstant = isConst;
            return;
        }
    }
}

// ============================================================
// ВСТРОЕННЫЕ ТИПЫ
// ============================================================

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

bool BydaoParser::loadMetaData(const QString& name) {
    if (m_metaData.contains(name)) {
        return true;
    }

    QString errorMsg;
    MetaData* metaData = BydaoModuleManager::instance().loadMetaData(name, &errorMsg);
    if ( metaData ) {
        m_metaData[name] = new MetaData( metaData );
        return true;
    }

    error("Cannot load module '" + name + "': " + errorMsg);
    return false;
}

// ============================================================
// ПАРСИНГ - ОСНОВНЫЕ МЕТОДЫ
// ============================================================

bool BydaoParser::parse() {
    // Глобальная область
    enterScope();

    bool ok = parseProgram();

    exitScope();
    emitCode(BydaoOpCode::Halt);

    return ok && m_errors.isEmpty();
}

bool BydaoParser::parseProgram() {
    while (!match(BydaoTokenType::EndOfFile)) {
        BydaoToken statementToken = m_current;
        int typeStackSize = m_typeStack.size();
        if (!parseStatement()) {
            // Восстановление после ошибки
            while (!match(BydaoTokenType::EndOfFile) &&
                   !match(BydaoTokenType::RBrace) &&
                   !match(BydaoTokenType::Semicolon)) {
                nextToken();
            }
            if (!m_errors.isEmpty()) return false;
        }
        if ( ! checkTypeStack( typeStackSize, statementToken ) ) {
            return false;
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

    if (match(BydaoTokenType::Pub)) return parsePubDecl();
    if (match(BydaoTokenType::Var)) return parseVarDecl();
    if (match(BydaoTokenType::Const)) return parseConstDecl();
    if (match(BydaoTokenType::While)) return parseWhile();
    if (match(BydaoTokenType::If)) return parseIf();
    if (match(BydaoTokenType::Iter)) return parseIter();
    if (match(BydaoTokenType::Enum)) return parseEnum();
    if (match(BydaoTokenType::Break) || match(BydaoTokenType::Next)) return parseBreakNext();
    if (match(BydaoTokenType::Use)) return parseUse();
    if (match(BydaoTokenType::LBrace)) return parseBlock(true);

    if ( parseExpression() ) return true;

    error( "Unknown token '" + m_current.text + "'", m_current );
    return false;
}

bool BydaoParser::parseBlock(bool requireBraces) {
    if (requireBraces) {
        if (!expect(BydaoTokenType::LBrace))
            return false;
    }

    enterScope();  // новая область для блока

    while ( ! match( BydaoTokenType::RBrace ) && ! match( BydaoTokenType::EndOfFile ) ) {

        BydaoToken statementToken = m_current;
        int typeStackSize = m_typeStack.size();
        if ( ! parseStatement() ) {
            exitScope();
            return false;
        }
        if ( ! checkTypeStack( typeStackSize, statementToken ) ) {
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
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ПРОВЕРКИ ТИПОВ
// ============================================================

bool    BydaoParser::checkTypeStack( int typeStackSize, const BydaoToken& statementToken ) {
    if ( m_typeStack.size() > typeStackSize ) {
        TypeInfo typeInfo = m_typeStack.pop();
        if ( typeInfo.type != "Void" ) {
            error("Invalid statement type '" + typeInfo.type +  "'", statementToken );
            return false;
        }
    }
    return true;
}

bool    BydaoParser::checkTypeConvert( const QString& typeName, const BydaoToken& token ) {
    TypeInfo typeInfo = m_typeStack.pop();
    if ( typeInfo.type != typeName ) {
        return checkTypeConvert( typeInfo.type, typeName, token );
    }
    return true;
}

bool    BydaoParser::checkTypeConvert( const QString& fromType, const QString& toType, const BydaoToken& token ) {
    MetaData* metaData = m_metaData[ fromType ];
    if ( ! metaData) {
        error( "Unknown type '" + fromType + "'" , token );
        return false;
    }
    if ( ! metaData->hasFunc("to" + toType) ) {
        error( "Operand cannot be converted to '" + toType + "'", token );
        return false;
    }
    return true;
}

bool    BydaoParser::checkTypeOper( const QString& typeName, const QString& operName, const BydaoToken& token ) {
    MetaData* metaData = m_metaData[ typeName ];
    if ( ! metaData ) {
        error( "Unknown type '" + typeName + "'" , token );
        return false;
    }
    if ( ! metaData->hasOper( operName ) ) {
        error( "Cannot compare operand of type '" + typeName + "'", token );
        return false;
    }
    return true;
}

/**
 * Получить тип результата операции с двумя операндами.
 *
 * @param leftType тип левого операнда
 * @param rightType тип правого операнда
 * @param operName название операции
 * @param token токен правого операнда для вывода ошибки
 *
 * @return Возвращает пустую строку при ошибке или название типа резуьтата операции.
 */
QString     BydaoParser::getResultType( const QString& leftType, const QString& rightType, const QString& operName, const BydaoToken& token ) {
    MetaData* metaData = m_metaData[ leftType ];
    if ( ! metaData ) {
        error( "Unknown type '" + leftType + "'" , token );
        return QString();
    }
    OperMetaData operMetaData = metaData->oper( operName );
    if ( operMetaData.hasOperandType( rightType ) ) {
        return operMetaData.resultType( rightType );
    }
    if ( operMetaData.hasOperandType( "Any" ) ) {
        if ( ! checkTypeConvert( rightType, leftType, token ) ) {
            return QString();
        }
        return operMetaData.resultType( "Any" );
    }
    return QString();
}

/**
 * Получить тип результата унарной операции.
 *
 * @param type тип операнда
 * @param operName название операции
 * @param token токен операнда для вывода ошибки
 *
 * @return Возвращает пустую строку при ошибке или название типа резуьтата операции.
 */
QString     BydaoParser::getResultType( const QString& type, const QString& operName, const BydaoToken& token ) {
    MetaData* metaData = m_metaData[ type ];
    if ( ! metaData ) {
        error( "Unknown type '" + type + "'" , token );
        return QString();
    }
    OperMetaData operMetaData = metaData->oper( operName );
    if ( operMetaData.hasOperandType( "" ) ) {
        return operMetaData.resultType( "" );
    }
    return QString();
}

// ============================================================
// ПАРСИНГ - ОПЕРАТОРЫ
// ============================================================

bool BydaoParser::parsePubDecl() {
    BydaoToken pubToken = m_current;
    nextToken(); // pub

    // После pub может быть var или const
    if (match(BydaoTokenType::Var)) {
        return parseVarDecl(true);  // true = публичная
    }
    if (match(BydaoTokenType::Const)) {
        return parseConstDecl(true); // true = публичная
    }

    error("Expected 'var' or 'const' after 'pub'");
    return false;
}

/**
 * Определение новой переменной: "var" name [ "=" expr ]
 */
bool BydaoParser::parseVarDecl( bool isPublic ) {
    BydaoToken token = m_current;
    nextToken(); // var

    if (!match(BydaoTokenType::Identifier)) {
        error("Expected variable name");
        return false;
    }

    QString name = m_current.text;
    BydaoToken nameToken = m_current;
    nextToken();

    // Добавляем переменную с флагом public
    if (!appendVariable(name, false, isPublic)) {  // false = не константа
        return false;
    }

    VariableInfo info = resolveVariable(name);

    // Генерируем код (как обычно)
    qint16 nameIndex = addString(name);
    emitCode(BydaoOpCode::VarDecl, nameIndex, 0, token);

    if (match(BydaoTokenType::Assign)) {    // с инициализацией

        nextToken();

        BydaoToken exprToken = m_current;
        if (!parseExpression()) return false;

        if ( m_typeStack.size() == 0 ) {
            error("Internal error: empty stack of value types");
            return false;
        }
        TypeInfo typeInfo = m_typeStack.pop();
        if ( typeInfo.type == "Void" ) {
            error("Cannot assign void value", exprToken );
            return false;
        }
        setVariableType( name, typeInfo.type );
    }
    else {                                  // со значением null
        qint16 nullConst = addNullConstant();
        emitCode(BydaoOpCode::PushConst, nullConst, 0, token);
    }

    emitCode(BydaoOpCode::Store, info.varIndex, 0, nameToken);

    return true;
}

bool BydaoParser::parseConstDecl( bool isPublic ) {
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
    if (!appendVariable(name, true, isPublic)) {
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
    m_varScopes[ info.varIndex ].varInfo.type = constValue.toObject()->typeName();
    m_varScopes[ info.varIndex ].varInfo.constValue = constValue;

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
    emitCode(BydaoOpCode::Store, info.varIndex, 0, nameToken);

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
    VariableInfo varInfo = resolveVariable(name);
    if (varInfo.isConstant) {
        error("Cannot assign to constant: " + name);
        return false;
    }

    nextToken(); // must be '='
    if ( ! expect( BydaoTokenType::Assign ) ) {
        error("Unexpected expression after identifier");
        return false;
    }

    if (!parseExpression()) {
        return false;
    }
    TypeInfo exprType = m_typeStack.pop();

    QString varType = varInfo.type;
    if ( varType != "Null" && varType != exprType.type ) {
        error( "Cannot assign value of type '" + exprType.type + "' to variable of type '" + varType + "'", nameToken );
        return false;
    }
    setVariableType( name, exprType.type );

    emitCode(BydaoOpCode::Store, varInfo.varIndex, 0, nameToken);
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

    BydaoToken exprToken = m_current;
    if (!parseExpression()) {
        return false;
    }
    TypeInfo exprTypeInfo = m_typeStack.pop();

    // Проверить, что тип переменной поддерживает операцию "+"

    QString varType = info.type;
    if ( varType == "Null" ) {
        error( "Operation '+=' cannot be applied to uninitialized variable", nameToken );
        return false;
    }

    // Проверить наличие операции сложения для левого операнда

    QString operName("addToValue");
    if ( ! checkTypeOper( varType, operName, nameToken ) ) {
        return false;
    }

    // Определить тип результат операции

    QString exprType = exprTypeInfo.type;
    if ( varType != exprType ) {

        // Проверить возможность использования правого операнда в операции
        // сложения с левым операндом

        QString resultType = getResultType( varType, exprType, operName, exprToken );
        if ( resultType.isEmpty() || resultType != "Void" ) {
            error( "Operation '+=' cannot be applied to a value of type '" + exprType + "'", exprToken );
            return false;
        }
    }

    int rightVarIndex = -1;
    if ( exprTypeInfo.operand == Var ) {

        int size = m_bytecode.size();
        if ( m_bytecode[size-1].op == BydaoOpCode::Load ) {

            rightVarIndex = m_bytecode.takeLast().arg1;
        }
    }
    emitCode(BydaoOpCode::AddStore, info.varIndex, rightVarIndex, nameToken);
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
        return false;
    }

    emitCode(BydaoOpCode::SubStore, info.varIndex, 0, nameToken);

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
        return false;
    }

    emitCode(BydaoOpCode::MulStore, info.varIndex, 0, nameToken);

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
        return false;
    }

    emitCode(BydaoOpCode::DivStore, info.varIndex, 0, nameToken);

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
        return false;
    }

    emitCode(BydaoOpCode::ModStore, info.varIndex, 0, nameToken);

    return true;
}

bool BydaoParser::parseIf() {
    BydaoToken ifToken = m_current;
    nextToken(); // if

    BydaoToken exprToken = m_current;
    if (!parseExpression()) {
        return false;
    }
    TypeInfo exprTypeInfo = m_typeStack.pop();
    if ( exprTypeInfo.type != "Bool" ) {

        if ( ! checkTypeConvert( exprTypeInfo.type, "Bool", exprToken ) ) {
            return false;
        }
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
    while ( match(BydaoTokenType::Elsif) ) {

        BydaoToken elsifToken = m_current;
        nextToken(); // elsif

        exprToken = m_current;
        if (!parseExpression()) {
            return false;
        }
        exprTypeInfo = m_typeStack.pop();
        if ( exprTypeInfo.type != "Bool" ) {

            if ( ! checkTypeConvert( exprTypeInfo.type, "Bool", exprToken ) ) {
                return false;
            }
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
    BydaoToken whileToken = m_current;
    nextToken(); // while

    LoopInfo loop;
    loop.conditionAddr = m_bytecode.size();

    BydaoToken exprToken = m_current;
    if (!parseExpression()) {
        return false;
    }
    TypeInfo exprTypeInfo = m_typeStack.pop();
    if ( exprTypeInfo.type != "Bool" ) {

        if ( ! checkTypeConvert( exprTypeInfo.type, "Bool", exprToken ) ) {
            return false;
        }
    }

    // Парсим next-оператор

    QVector<BydaoInstruction> nextCode;

    bool hasNext = match(BydaoTokenType::Next);
    if ( hasNext ) {
        nextToken(); // next

        int savedPos = m_bytecode.size();

        do {
            BydaoToken statementToken = m_current;
            int typeStackSize = m_typeStack.size();
            if ( ! parseStatement() ) {
                return false;
            }
            if ( ! checkTypeStack( typeStackSize, statementToken ) ) {
                return false;
            }

            // Сохраняем код next-оператора
            for (int i = savedPos; i < m_bytecode.size(); i++) {
                nextCode.append(m_bytecode[i]);
            }

            // Если после оператора нет запятой - заканчиваем
            if ( ! match(BydaoTokenType::Comma) ) {
                break;
            }
            nextToken(); // пропускаем запятую

        } while( true );

        m_bytecode.resize(savedPos);
    }

    int condJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, whileToken);

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
    emitCode(BydaoOpCode::Store, iterInfo.varIndex, 0, iterToken);

    int loopStart = m_bytecode.size();

    // iter.next()
    emitCode(BydaoOpCode::Load, iterInfo.varIndex, 0, iterToken);
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
    emitCode(BydaoOpCode::Drop, iterInfo.varIndex, 0, iterToken);
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

    BydaoToken exprToken = m_current;
    if (!parseExpression()) {
        return false;
    }
    TypeInfo exprTypeInfo = m_typeStack.pop();
    MetaData* exprMetaData = m_metaData[ exprTypeInfo.type ];
    if ( ! exprMetaData ) {
        error("Unknown type '" + exprTypeInfo.type + "'", exprToken );
        return false;
    }
    if ( ! exprMetaData->hasFunc("iter") ) {
        error("Type '" + exprTypeInfo.type + "' does not have function 'iter'", exprToken );
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

    // Объявляем переменную а текущей области видимости для значения
    declareVariable(varName, varToken);
    VariableInfo varInfo = resolveVariable(varName);

    // Подготовить тип переменной
    const FuncMetaData funcIter = exprMetaData->func("iter");
    QString iterType = funcIter.retType;
    MetaData* iterMetaData = m_metaData[ iterType ];
    if ( ! iterMetaData ) {
        error("Unknown type '" + iterType + "'", exprToken );
        return false;
    }
    if ( ! iterMetaData->hasVar("value") ) {
        error("Iterator of type '" + iterType + "' does not have variable 'value'", exprToken );
        return false;
    }
    VarMetaData varMetaData = iterMetaData->var("value");
    setVariableType( varName, varMetaData.type, varMetaData.isConst );

    // Получаем итератор: collection.iter()
    emitCode(BydaoOpCode::GetIter, 0, 0, token);

    // Создаём временный итератор
    if ( ! iterMetaData->hasFunc("next") ) {
        error("Iterator of type '" + iterType + "' does not have func 'next'", exprToken );
        return false;
    }
    QString tmpIterName = QString("__enum_iter_%1").arg(m_iteratorCounter++);
    declareVariable(tmpIterName, token);  // объявляем в текущей области
    VariableInfo iterInfo = resolveVariable(tmpIterName);
    setVariableType( tmpIterName, iterType );
    emitCode(BydaoOpCode::Store, iterInfo.varIndex, 0, token);

    int loopStart = m_bytecode.size();

    // iter.next()
    emitCode(BydaoOpCode::Load, iterInfo.varIndex, 0, token);
    emitCode(BydaoOpCode::ItNext, 0, 0, token);

    int condJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, token);

    // Получаем значение и сохраняем в переменную
    emitCode(BydaoOpCode::Load, iterInfo.varIndex, 0, token);
    emitCode(BydaoOpCode::ItValue, 0, 0, token);
    emitCode(BydaoOpCode::Store, varInfo.varIndex, 0, varToken);

    // Тело цикла - parseBlock сам создаст область видимости
    if (!parseBlock(true)) {
        return false;
    }

    emitCode(BydaoOpCode::Jump, loopStart, 0);

    // Патчим условный переход
    m_bytecode[condJump].arg1 = m_bytecode.size();

    // Очистка - удаляем временный итератор
    emitCode(BydaoOpCode::Drop, iterInfo.varIndex, 0, token);
    removeVariable(tmpIterName);

    // Удаляем переменную цикла
    emitCode(BydaoOpCode::Drop, varInfo.varIndex, 0, varToken);
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
    }
    else if (match(BydaoTokenType::Next)) {
        int jumpInstr = emitCode(BydaoOpCode::Jump, 0, 0, m_current);
        loop.nexts.append(jumpInstr);
        nextToken();
        return true;
    }

    return false;
}

/**
 * @brief Разбор оператора use <moduleName> [as <alias>] {, <moduleName> [as <alias>]}
 * @return bool
 */
bool BydaoParser::parseUse() {
    BydaoToken token = m_current;

    do {

        // Получим следующий токен после оператора "use" или запятой

        nextToken();
        if (!match(BydaoTokenType::Identifier)) {
            error("Expected module name");
            return false;
        }

        QString moduleName = m_current.text;
        if ( ! m_metaData.contains( moduleName ) ) {

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

            if ( ! loadMetaData( moduleName ) ) {
                return false;
            }

            // Добавляем переменную в текущую область видимости
            if ( ! appendVariable( alias ) ) {
                return false;
            }

            // Генерируем код
            qint16 moduleNameIdx = addString(moduleName);
            qint16 aliasIdx = addString(alias);
            emitCode(BydaoOpCode::UseModule, moduleNameIdx, aliasIdx, token);
        }

        // Продолжим обработку, если есть запятая

    } while ( match(BydaoTokenType::Comma) );

    return true;
}

// ============================================================
// ПАРСИНГ - ВЫРАЖЕНИЯ
// ============================================================

bool BydaoParser::parseExpression() {
    return parseLogicalOr();
}

bool BydaoParser::parseLogicalOr() {

    BydaoToken leftToken = m_current;
    if (!parseLogicalAnd()) return false;

    while (match(BydaoTokenType::Or)) {

        // Проверить тип левого операнда операции

        if ( ! checkTypeConvert( "Bool", leftToken ) ) {
            return false;
        }

        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseLogicalAnd()) return false;

        // Проверить тип правого операнда

        if ( ! checkTypeConvert( "Bool", rightToken ) ) {
            return false;
        }

        emitCode(BydaoOpCode::Or, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseLogicalAnd() {
    BydaoToken leftToken = m_current;
    if (!parseEquality()) return false;

    while (match(BydaoTokenType::And)) {

        // Проверить тип левого операнда операции

        if ( ! checkTypeConvert( "Bool", leftToken ) ) {
            return false;
        }

        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseEquality()) return false;

        // Проверить тип правого операнда

        if ( ! checkTypeConvert( "Bool", rightToken ) ) {
            return false;
        }

        emitCode(BydaoOpCode::And, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseEquality() {
    BydaoToken leftToken = m_current;
    if (!parseComparison()) return false;

    while (match(BydaoTokenType::Equal) || match(BydaoTokenType::NotEqual)) {

        TypeInfo leftTypeInfo = m_typeStack.pop();

        bool isEq = match(BydaoTokenType::Equal);
        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseComparison()) return false;

        TypeInfo rightTypeInfo = m_typeStack.pop();

        // Проверить наличие операции сравнения для левого операнда

        QString leftType = leftTypeInfo.type;
        if ( ! checkTypeOper( leftType, isEq ? "eq" : "neq" , leftToken ) ) {
            return false;
        }

        // Если сравнение значений разного типа

        if ( leftTypeInfo.type != rightTypeInfo.type ) {

            // Проверить возможность преобразования типа правого операнда
            // к типу левого операнда

            if ( ! checkTypeConvert( rightTypeInfo.type, leftType, rightToken ) ) {
                return false;
            }
        }

        m_typeStack.push( TypeInfo("Bool", Expr ) );

        emitCode(isEq ? BydaoOpCode::Eq : BydaoOpCode::Neq, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseComparison() {
    BydaoToken leftToken = m_current;
    if (!parseAddition()) return false;

    while (match(BydaoTokenType::Less) || match(BydaoTokenType::Greater) ||
           match(BydaoTokenType::LessEqual) || match(BydaoTokenType::GreaterEqual)) {

        TypeInfo leftTypeInfo = m_typeStack.pop();

        BydaoOpCode opCode;
        QString operName;
        switch (m_current.type) {
        case BydaoTokenType::Less: opCode = BydaoOpCode::Lt; operName = "lt"; break;
        case BydaoTokenType::Greater: opCode = BydaoOpCode::Gt; operName = "gt"; break;
        case BydaoTokenType::LessEqual: opCode = BydaoOpCode::Le; operName = "le"; break;
        case BydaoTokenType::GreaterEqual: opCode = BydaoOpCode::Ge; operName = "ge"; break;
        default: return false;
        }

        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseAddition()) return false;

        TypeInfo rightTypeInfo = m_typeStack.pop();

        // Проверить наличие операции сравнения на равенство для левого операнда

        QString leftType = leftTypeInfo.type;
        if ( ! checkTypeOper( leftType, operName, leftToken ) ) {
            return false;
        }

        // Если сравнение значений разного типа

        if ( leftType != rightTypeInfo.type ) {

            // Проверить возможность преобразования типа правого операнда
            // к типу левого операнда

            if ( ! checkTypeConvert( rightTypeInfo.type, leftType, rightToken ) ) {
                return false;
            }
        }

        m_typeStack.push( TypeInfo("Bool", Expr) );

        bool emitStd = true;
        if ( leftTypeInfo.operand == Var && rightTypeInfo.operand == Var ) {

            int size = m_bytecode.size();
            if ( m_bytecode[size-1].op == BydaoOpCode::Load && m_bytecode[size-2].op == BydaoOpCode::Load ) {

                int rightVarIndex = m_bytecode.takeLast().arg1;
                int leftVarIndex = m_bytecode.takeLast().arg1;
                if ( opCode == BydaoOpCode::Lt ) {
                    emitCode(BydaoOpCode::VarLt, leftVarIndex, rightVarIndex, op);
                    emitStd = false;
                }
            }
        }

        if ( emitStd ) {
            emitCode(opCode, 0, 0, op);
        }
    }

    return true;
}

bool BydaoParser::parseAddition() {
    BydaoToken leftToken = m_current;
    if (!parseTerm()) return false;

    while (match(BydaoTokenType::Plus) || match(BydaoTokenType::Minus)) {

        TypeInfo leftTypeInfo = m_typeStack.pop();

        bool isPlus = match(BydaoTokenType::Plus);

        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseTerm()) return false;
        TypeInfo rightTypeInfo = m_typeStack.pop();

        // Проверить наличие операции сложения/вычитания для левого операнда

        QString leftType = leftTypeInfo.type;
        QString operName = isPlus ? "add" : "sub";
        if ( ! checkTypeOper( leftType, operName, leftToken ) ) {
            return false;
        }

        // Определить тип результат операции

        QString resultType = leftType;
        if ( leftType != rightTypeInfo.type ) {

            // Проверить возможность использования правого операнда в операции
            // сложения/вычитания с левым операндом

            QString rightType = rightTypeInfo.type;
            resultType = getResultType( leftType, rightType, operName, rightToken );
            if ( resultType.isEmpty() ) {
                error( "Operation '" + op.text + "' cannot be applied to a value of type '" + rightType + "'", rightToken );
                return false;
            }
        }
        m_typeStack.push( TypeInfo( resultType, Expr ) );

        emitCode(isPlus ? BydaoOpCode::Add : BydaoOpCode::Sub, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseTerm() {
    BydaoToken leftToken = m_current;
    if (!parseUnary()) return false;

    while (match(BydaoTokenType::Mul) || match(BydaoTokenType::Div) || match(BydaoTokenType::Mod)) {

        TypeInfo leftTypeInfo = m_typeStack.pop();

        BydaoOpCode opCode;
        QString operName;
        switch (m_current.type) {
        case BydaoTokenType::Mul: opCode = BydaoOpCode::Mul; operName = "mul"; break;
        case BydaoTokenType::Div: opCode = BydaoOpCode::Div; operName = "div"; break;
        case BydaoTokenType::Mod: opCode = BydaoOpCode::Mod; operName = "mod"; break;
        default: return false;
        }

        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseUnary()) return false;
        TypeInfo rightTypeInfo = m_typeStack.pop();

        // Проверить наличие операции для левого операнда

        QString leftType = leftTypeInfo.type;
        if ( ! checkTypeOper( leftType, operName, leftToken ) ) {
            return false;
        }

        // Определить тип результат операции

        QString resultType = leftType;
        if ( leftType != rightTypeInfo.type ) {

            // Проверить возможность использования правого операнда в операции
            // сложения/вычитания с левым операндом

            QString rightType = rightTypeInfo.type;
            resultType = getResultType( leftType, rightType, operName, rightToken );
            if ( resultType.isEmpty() ) {
                error( "Operation '" + op.text + "' cannot be applied to a value of type '" + rightType + "'", rightToken );
                return false;
            }
        }
        m_typeStack.push( TypeInfo( resultType, Expr ) );

        emitCode(opCode, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseUnary() {
    if ( match(BydaoTokenType::Not) ) {
        return parseNot();
    }
    if ( match(BydaoTokenType::Minus) ) {
        return parseMinus();
    }
    return parsePrimary();
}

bool BydaoParser::parseNot() {
    BydaoToken op = m_current;
    nextToken();

    BydaoToken token = m_current;
    if (!parseUnary()) return false;

    // Проверить тип операнда операции

    if ( ! checkTypeConvert( "Bool", token ) ) {
        return false;
    }

    emitCode(BydaoOpCode::Not, 0, 0, op);
    return true;
}

bool BydaoParser::parseMinus() {
    BydaoToken op = m_current;
    nextToken();

    BydaoToken token = m_current;
    if ( ! parseUnary() ) return false;
    TypeInfo typeInfo = m_typeStack.pop();

    if ( ! checkTypeOper( typeInfo.type, "neg", token ) ) {
        return false;
    }
    QString resultType = getResultType( typeInfo.type, "neg", token );
    if ( resultType.isEmpty() ) {
        error( "The '" + op.text + "' operation cannot be applied to a value of type '" + typeInfo.type + "'", token );
        return false;
    }
    m_typeStack.push( TypeInfo( resultType, Expr ) );

    emitCode(BydaoOpCode::Neg, 0, 0, op);
    return true;
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

        if ( text.contains('x') || text.contains('X')) {
            constIndex = addConstant(text.toLongLong(nullptr,16));
            m_typeStack.push( TypeInfo("Int", Const) );
        }
        else if (text.contains('.') || text.contains('e') || text.contains('E')) {
            constIndex = addConstant(text.toDouble());
            m_typeStack.push( TypeInfo("Real", Const) );
        }
        else if ( text.contains('b') || text.contains('B')) {
            constIndex = addConstant(text.toLongLong(nullptr,2));
            m_typeStack.push( TypeInfo("Int", Const) );
        }
        else {
            constIndex = addConstant(text.toLongLong());
            m_typeStack.push( TypeInfo("Int", Const) );
        }

        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        nextToken();
        return true;
    }

    if (match(BydaoTokenType::String)) {
        QString text = m_current.text;
        qint16 constIndex = addConstant(text);
        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        m_typeStack.push( TypeInfo("String", Const) );
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
        m_typeStack.push( TypeInfo("Bool", Const) );
        nextToken();
        return true;
    }

    if (match(BydaoTokenType::True)) {
        qint16 constIndex = addConstant(true);
        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        m_typeStack.push( TypeInfo("Bool", Const) );
        nextToken();
        return true;
    }

    if (match(BydaoTokenType::Null)) {
        qint16 constIndex = addNullConstant();
        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        m_typeStack.push( TypeInfo("Null", Const) );
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

        if ( m_metaData.contains( name ) ) {    // это название типа данных или модуля

            MetaData* metaData = m_metaData[ name ];
            if ( metaData->external ) {         // внешний модуль/класс
                // Должен быть уже загружен через use
                if (!isVariableDeclared(name)) {
                    error("External module not loaded: " + name);
                    return false;
                }
                nextToken(); // съедаем имя модуля
                VariableInfo info = resolveVariable(name);
                emitCode(BydaoOpCode::Load, info.varIndex, 0, nameToken);
                m_typeStack.push( TypeInfo( name, Module ) );
            }
            else {                              // внутренний тип/класс
                // Встроенный тип: Int, String, Array и т.д.
                nextToken(); // съедаем имя типа
                qint16 typeIdx = addString(name);
                emitCode(BydaoOpCode::TypeClass, typeIdx, 0, nameToken);
                m_typeStack.push( TypeInfo( name, Type ) );
            }
        }
        else if (isVariableDeclared(name)) {    // обычная переменная?
            nextToken(); // съедаем имя переменной
            VariableInfo info = resolveVariable(name);
            emitCode(BydaoOpCode::Load, info.varIndex, 0, nameToken);
            m_typeStack.push( TypeInfo( info.type, Var ) );
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
            if (!parseCallSuffix()) return false;
        }
        else if (match(BydaoTokenType::Dot)) {
            if (!parseMemberSuffix()) return false;
        }
        else if (match(BydaoTokenType::LBracket)) {
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

    BydaoToken token = m_current;   // '('
    nextToken();                    // следущий токен за '('

    const TypeInfo& typeInfo = m_typeStack.top();
    QString memberName = typeInfo.member;
    MetaData* metaData = m_metaData[ typeInfo.type ];
    const FuncMetaData func = metaData->func( memberName );

    // Парсим аргументы, если они есть

    int argCount = 0;
    if (!match(BydaoTokenType::RParen)) {
        do {

            BydaoToken argToken = m_current;

            if (!parseExpression()) {
                error( QString( "Invalid expression in argument %1 function '%2'").arg( argCount + 1 ).arg( memberName ) );
                return false;
            }

            const QStringList& argTypeList = func.argList[ argCount ].types;    // допустимые типы аргумента функции

            TypeInfo exprTypeInfo = m_typeStack.pop();      // тип выражения аргумента
            QString exprType = exprTypeInfo.type;
            if ( ! argTypeList.contains( exprType ) ) {     // тип выражения не соответствует ни одному допустимому типу аргумента функции

                // Для типа выражения проверить наличие функции приведения к типу аргумента

                if ( ! m_metaData.contains( exprType ) ) {
                    error("Unknown expession type '" + exprType + "'" );
                    return false;
                }
                bool canConvert = false;
                MetaData* exprMetaData = m_metaData[ exprType ];
                for ( int i = 0; i < argTypeList.size(); ++i ) {
                    const QString& argType = argTypeList[ i ];
                    if ( exprMetaData->hasFunc( QString("to%1").arg(argType) ) ) {
                        canConvert = true;
                        break;
                    }
                }
                if ( ! canConvert ) {
                    error( QString( "Argument %1 cannot convert to allowed type" ).arg( argCount + 1 ), argToken );
                    return false;
                }
            }
            argCount++;

            // Если после выражения нет запятой - заканчиваем
            if ( ! match(BydaoTokenType::Comma) ) {
                break;
            }
            nextToken(); // пропускаем запятую
        } while (true);
    }

    // TODO: Добавить недостающие аргументы значениями по умолчанию

    if ( argCount != func.argCount() ) {
        error("Insufficient argument number for " + memberName );
        return false;
    }

    // Ожидаем закрывающую скобку
    if (!expect(BydaoTokenType::RParen)) {
        error("Expected ')' after arguments");
        return false;
    }

    // Удалим информацию о типе и методе
    m_typeStack.pop();

    // Сохраним тип возвращаемого значения
    m_typeStack.push( func.retType );

    // Генерируем инструкцию CALL
    // Объект для вызова уже лежит на стеке (результат предыдущих операций)
    // Аргументы лежат на стеке в правильном порядке (благодаря parseExpression)
//    qDebug() << "parseCallSuffix: emitting CALL with" << argCount << "arguments";
    emitCode( func.retType == "Void" ? BydaoOpCode::CallVoid : BydaoOpCode::Call, argCount, 0, token);

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

    // Проверим наличие текущего типа данных
    TypeInfo& typeInfo = m_typeStack.top();
    if ( ! m_metaData.contains( typeInfo.type ) ) {
        qDebug() << "Check member for type" + typeInfo.type;
        error("Not found metadata for " + typeInfo.type);
        return false;
    }
    MetaData* metaData = m_metaData[ typeInfo.type ];

    // Добавляем имя члена в таблицу строк
    qint16 memberIdx = addString(memberName);

    // Пропускаем имя члена
    nextToken();

    // Смотрим, что идёт после имени члена
    if (match(BydaoTokenType::LParen)) {

        // Проверим, что у текущего типа есть такая функция

        if ( ! metaData->hasFunc( memberName ) ) {
            error("Type " + typeInfo.type + " does not have function " + memberName);
            return false;
        }

        // Запомним название метода

        typeInfo.member = memberName;

        // Это вызов метода - используем METHOD

        emitCode(BydaoOpCode::Method, memberIdx, 0, memberToken);
    }
    else {

        // Проверим, что у текущего типа есть такая переменная

        if ( ! metaData->hasVar( memberName ) ) {
            error("Type '" + typeInfo.type + "' does not have member '" + memberName + "'", memberToken );
            return false;
        }

        // Сохраним тип переменной

        m_typeStack.pop();
        m_typeStack.push( metaData->var( memberName ).type );

        // Это доступ к свойству - используем MEMBER
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

    m_typeStack.push( TypeInfo( "Array", Const ) );

    // Генерируем PushArray с количеством элементов
    // Элементы уже на стеке в правильном порядке (благодаря parseExpression)
    emitCode(BydaoOpCode::PushArray, elementCount, 0, token);

    return true;
}

} // namespace BydaoScript
