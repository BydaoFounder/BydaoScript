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
#include "BydaoScript/BydaoArray.h"
#include "BydaoScript/BydaoReal.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoBool.h"
#include "BydaoScript/BydaoNull.h"
#include "BydaoScript/BydaoFuncObject.h"

namespace BydaoScript {

// ============================================================
// КОНСТРУКТОР / ДЕСТРУКТОР
// ============================================================

BydaoParser::BydaoParser(const QString& moduleName, const QVector<BydaoToken>& tokens)
{
    m_tokens = tokens;
    m_pos = 0;
    m_inLoop = false;
    m_iteratorCounter = 0;
    m_exprCount = 0;
    m_moduleName = moduleName;

    // Начальная область видимости модуля
    m_scopeLevel = 0;
    m_varScopes.append( VarScope() );

    if ( ! m_tokens.isEmpty() ) {
        m_current = m_tokens[0];
    }

    // Добавляем пути поиска модулей
    m_modulePaths << ".";
    m_modulePaths << "./modules";
    m_modulePaths << QCoreApplication::applicationDirPath();
    m_modulePaths << QCoreApplication::applicationDirPath() + "/modules";

    // Добавляем константу NULL, чтобы все другие константы имели индекс > 0

    addNullConstant();

    // Добавляем служебную переменную, чтобы все индексы переменных были > 0.
    // TODO: Служебная переменная специального типа, которая содержит информацию
    // о выполняемом скрипте.

    appendVariable(SPECIAL_VAR, true, false);

    // Инициализировать встроенные типы данных

    m_metaData["Int"] = new MetaData( BydaoIntClass::metaData() );
    UsedMetaDataList usedIntList = BydaoIntClass::usedMetaData();
    foreach ( const UsedMetaData& used, usedIntList ) {
        if ( used.append ) {
            m_metaData["Int"]->append( used.metaData );
        }
        else if ( ! m_metaData.contains( used.type ) ) {
            m_metaData[ used.type ] = new MetaData( used.metaData );
        }
    }

    m_metaData["String"] = new MetaData( BydaoStringClass::metaData() );
    UsedMetaDataList usedStringList = BydaoStringClass::usedMetaData();
    foreach ( const UsedMetaData& used, usedStringList ) {
        if ( used.append ) {
            m_metaData["String"]->append( used.metaData );
        }
        else if ( ! m_metaData.contains( used.type ) ) {
            m_metaData[ used.type ] = new MetaData( used.metaData );
        }
    }

    m_metaData["Real"]   = new MetaData( BydaoReal::metaData() );
    m_metaData["Bool"]   = new MetaData( BydaoBool::metaData() );
    m_metaData["Null"]   = new MetaData( BydaoNull::metaData() );

    m_metaData["Array"]  = new MetaData( BydaoArray::metaData() );
    UsedMetaDataList usedArrayList = BydaoArray::usedMetaData();
    foreach ( const UsedMetaData& used, usedArrayList ) {
        if ( ! m_metaData.contains( used.type ) ) {
            m_metaData[ used.type ] = new MetaData( used.metaData );
        }
    }

    // Список встроенных типов.
    // В виртуальной машине должен быть такой же список в том же порядке.
    m_builtinTypes << "Int" << "String" << "Real" << "Bool" << "Null" << "Array";
}

BydaoParser::~BydaoParser() {
    qDeleteAll( m_metaData );
}

ModuleInfo BydaoParser::buildModuleInfo(const QString& moduleName) {
    ModuleInfo module;
    module.name = moduleName;
    module.globalStringTable = m_stringTable;
    module.globalConstants = m_constants;
    module.globalCode = m_bytecode;

    // Собираем публичные переменные
/*
    for (const auto& scopeItem : m_varScopes) {
        const VariableInfo& info = scopeItem.varInfo;
        if (info.isPublic && info.type != "Func") {
            ModuleInfo::PubVar pubVar;
            pubVar.name = info.name;
            pubVar.type = info.type;
            pubVar.isConst = info.isConstant;
            pubVar.initValue = info.constValue;
            module.pubVars.append(pubVar);
        }
    }
*/

    // Собираем функции

    for ( int i = 0; i < m_funcs.size(); ++i ) {
        auto* funcObj = m_funcs[ i ];

        FunctionInfo funcInfo;
        funcInfo.name = funcObj->name;
        funcInfo.isImmutable = funcObj->funcMetaData.isImmutable;
        funcInfo.isStatic = funcObj->funcMetaData.isStatic;
        funcInfo.isPublic = funcObj->funcMetaData.isPublic;
        funcInfo.retType = funcObj->funcMetaData.retType;
        funcInfo.arity = funcObj->arity;
        funcInfo.code = funcObj->bytecode;
        funcInfo.selfRefs = funcObj->selfVarIndices;

        // Конвертируем аргументы
        for (int i = 0; i < funcObj->funcMetaData.argList.size(); i++) {
            const FuncArgMetaData& argMeta = funcObj->funcMetaData.argList[i];
            FunctionArgInfo argInfo;
            argInfo.name = argMeta.name;
            argInfo.types = argMeta.types;
            argInfo.isOut = argMeta.isOut;
            argInfo.hasDefault = !argMeta.defVal.isEmpty();
            // TODO: распарсить defVal в BydaoValue
            funcInfo.args.append(argInfo);
        }

        module.functions.append(funcInfo);
    }

    return module;
}

const ModuleInfo&   BydaoParser::moduleInfo() {
    return m_moduleInfo;
}

// ============================================================
// РАБОТА С ТАБЛИЦАМИ
// ============================================================

qint16 BydaoParser::addString(const QString& str) {

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
    switch (c.type) {
    case CONST_STRING: key = "s:" + QString::number(c.stringIndex); break;
    case CONST_INT:    key = "i:" + QString::number(c.intValue); break;
    case CONST_REAL:   key = "r:" + QString::number(c.realValue); break;
    case CONST_BOOL:   key = "b:" + QString::number(c.boolValue); break;
    case CONST_NULL:   key = "null"; break;
    default:
        error("Invalid type of constant");
        break;
    }

    if ( ! key.isEmpty() ) {
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

qint16 BydaoParser::addConstant( const BydaoValue& val ) {
    switch ( val.typeId() ) {
    case TYPE_STRING: return addConstant( val.toString() );
    case TYPE_INT:    return addConstant( val.toInt() );
    case TYPE_REAL:   return addConstant( val.toReal() );
    case TYPE_BOOL:   return addConstant( val.toBool() );
    case TYPE_NULL:   return addNullConstant();
    }
    error("Invalid type of constant");
    return 0;
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
          .arg(int(type))
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

void        BydaoParser::appendScope() {
    ++m_scopeLevel;
    m_varScopes.append( VarScope() );
}

void        BydaoParser::dropScope() {
    m_varScopes.takeLast();
    --m_scopeLevel;
}

void BydaoParser::enterScope() {
    m_scopeStart.push( m_varScopes[ m_scopeLevel ].size() );
}

void BydaoParser::exitScope() {
    if ( m_varScopes[ m_scopeLevel ].size() > m_scopeStart.top() ) {
        m_varScopes[ m_scopeLevel ].resize( m_scopeStart.top() );
        emitCode( BydaoOpCode::ScopeDrop, m_scopeStart.top() );
    }
    m_scopeStart.pop();
}

bool BydaoParser::appendVariable(const QString& name, bool isConstant, bool isPublic ) {

//    qDebug() << "appendVariable:" << name << "isConstant =" << isConstant;  // ОТЛАДКА

    VarScope& varScopes = m_varScopes[ m_scopeLevel ];

    // Проверяем, не объявлена ли уже в этой области

    int index = varScopes.size();
    if ( index > 0 ) {
        int scopeStart = m_scopeStart.top();
        while ( --index >= scopeStart ) {
            if ( varScopes[ index ].name == name ) {
                error("Variable '" + name + "' already declared in this scope");
                return false;
            }
        }
    }

    // Добавляем в список переменных

    VariableInfo info;
    info.name = name;
    info.type = "Null";
    info.varIndex = varScopes.size();
    info.isConstant = isConstant;
    info.isPublic = isPublic;

    m_varScopes[ m_scopeLevel ].append( ScopeItem( name, info ) );

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

    VarScope& varScopes = m_varScopes[ m_scopeLevel ];
    int index = varScopes.size();
    while ( --index >= 0 ) {
        if ( varScopes[ index ].name == name ) {
            return varScopes[ index ].varInfo;
        }
    }
    return VariableInfo();
}

bool BydaoParser::isVariableDeclared(const QString& name) {

    VarScope& varScopes = m_varScopes[ m_scopeLevel ];
    int index = varScopes.size();
    while ( --index >= 0 ) {
        if ( varScopes[ index ].name == name ) {
            return true;
        }
    }
    return false;
}

bool BydaoParser::removeVariable(const QString& name ) {

    VarScope& varScopes = m_varScopes[ m_scopeLevel ];
    int index = varScopes.size();
    while ( --index >= 0 ) {
        if ( varScopes[ index ].name == name ) {
            varScopes.removeAt( index );
            return true;
        }
    }
    return false;
}

void BydaoParser::setVariableType(const QString& name, const QString type, bool isConst) {

    VarScope& varScopes = m_varScopes[ m_scopeLevel ];
    int index = varScopes.size();
    while ( --index >= 0 ) {
        if ( varScopes[ index ].name == name ) {
            varScopes[ index ].varInfo.type = type;
            varScopes[ index ].varInfo.isConstant = isConst;
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
           type == BydaoTokenType::Do ||
           type == BydaoTokenType::If ||
           type == BydaoTokenType::Else ||
           type == BydaoTokenType::Break ||
           type == BydaoTokenType::Next ||
           type == BydaoTokenType::Var ||
           type == BydaoTokenType::Drop ||
           type == BydaoTokenType::Use ||
           type == BydaoTokenType::Ignore ||
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

    QString errorMsg;
    BydaoModule* module = BydaoModuleManager::instance().loadModule( name, &errorMsg );
    if ( ! module ) {
        error("Cannot load module '" + name + "': " + errorMsg);
        return false;
    }

    // Сохраняем мета-данные модуля
    MetaData* metaData = module->metaData();
    if ( ! metaData ) {
        BydaoModuleManager::instance().unloadModule( name );
        error( "Metadata is NULL for module '" + name + "'" );
        return false;
    }
    QString typeName = metaData->name.isEmpty() ? name : metaData->name;
    if ( ! m_metaData.contains( typeName ) ) {

        m_metaData[typeName] = new MetaData( metaData );

        // Сохраняем мета-данные используемых типов
        UsedMetaDataList usedList = module->usedMetaData();
        foreach ( const UsedMetaData& used, usedList ) {

            if ( ! m_metaData.contains( used.type ) ) {
                m_metaData[ used.type ] = new MetaData( used.metaData );
            }
            else if ( used.append ) {
                m_metaData[ used.type ]->append( used.metaData );
            }
        }
    }

    BydaoModuleManager::instance().unloadModule( name );
    return true;
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

    // Проверить, что все функции полностью определены

    if ( ok ) {
        foreach (auto* func, m_funcs ) {
            if ( func->bytecode.isEmpty() ) {
                ok = false;
                error( QString( "Undefined body of function '%1'" ).arg( func->name ) );
            }
        }
    }

    if ( ok ) {
        m_moduleInfo = buildModuleInfo( m_moduleName );
    }

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


    // Проверка на func (может быть с модификаторами)
    if (match(BydaoTokenType::Func) ||
        match(BydaoTokenType::Pub) ||
        match(BydaoTokenType::Immut)) {

        // Запоминаем позицию, чтобы откатиться если это не объявление функции
        int savedPos = m_pos;
        BydaoToken savedToken = m_current;

        if (match(BydaoTokenType::Pub)) {
            nextToken();
        }
        if (match(BydaoTokenType::Immut)) {
            nextToken();
        }

        bool isFunc = match(BydaoTokenType::Func);
        m_pos = savedPos;
        m_current = savedToken;

        if ( isFunc ) {
            // Парсим как функцию с самого начала
            return parseFuncDecl();
        }
    }

    if (match(BydaoTokenType::Pub)) return parsePubDecl();
    if (match(BydaoTokenType::Var)) return parseVarDecl();
    if (match(BydaoTokenType::Const)) return parseConstDecl();
    if (match(BydaoTokenType::While)) return parseWhile();
    if (match(BydaoTokenType::Do)) return parseDoWhile();
    if (match(BydaoTokenType::If)) return parseIf();
    if (match(BydaoTokenType::Iter)) return parseIter();
    if (match(BydaoTokenType::Enum)) return parseEnum();
    if (match(BydaoTokenType::Break) || match(BydaoTokenType::Next)) return parseBreakNext();
    if (match(BydaoTokenType::Use)) return parseUse();
    if (match(BydaoTokenType::LBrace)) return parseBlock(true);
    if (match(BydaoTokenType::Ignore)) return parseIgnore();
    if (match(BydaoTokenType::Return)) return parseReturn();

    if ( parseExpression() ) return true;

//    error( "Unknown token '" + m_current.text + "'", m_current );
    return false;
}

bool BydaoParser::parseBlock(bool requireBraces) {
    if (requireBraces) {
        if (!expect(BydaoTokenType::LBrace))
            return false;
        enterScope();  // новая область для блока
    }

    while ( ! match( BydaoTokenType::RBrace ) && ! match( BydaoTokenType::EndOfFile ) ) {

        BydaoToken statementToken = m_current;
        int typeStackSize = m_typeStack.size();
        if ( ! parseStatement() ) {
            if (requireBraces) {
                exitScope();
            }
            return false;
        }
        if ( m_exprCount == 0 ||
             ( m_exprCount > 0 && m_current.type != BydaoTokenType::RBrace ) ) {
            if ( ! checkTypeStack( typeStackSize, statementToken ) ) {
                return false;
            }
        }
    }

    if (requireBraces) {
        if (!expect(BydaoTokenType::RBrace))
            return false;
        exitScope();
    }

    return true;
}

// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ПРОВЕРКИ ТИПОВ
// ============================================================

bool    BydaoParser::checkTypeStack( int typeStackSize, const BydaoToken& statementToken ) {
    if ( m_typeStack.size() > typeStackSize ) {
        TypeInfo typeInfo = getLastType();
        if ( typeInfo.type != "Void" ) {
            error("Invalid statement type '" + typeInfo.type +  "'", statementToken );
            return false;
        }
    }
    return true;
}

bool    BydaoParser::checkTypeConvert( const QString& typeName, const BydaoToken& token ) {
    TypeInfo typeInfo = getLastType();
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
    if ( ! metaData->hasObjFunc("to" + toType) ) {
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
    if ( ! metaData->hasObjOper( operName ) ) {
        error( "Cannot compare operand of type '" + typeName + "'", token );
        return false;
    }
    return true;
}
bool    BydaoParser::canConvertType(const QString& from, const QString& to) {
    if (from == to) return true;
    if (to == "Any") return true;
    if (from == "Null" && to != "Void") return true;

    // Проверяем наличие метода toType
    if (m_metaData.contains(from)) {
        MetaData* meta = m_metaData[from];
        QString convertMethod = "to" + to;
        if (meta->hasObjFunc(convertMethod)) {
            return true;
        }
    }

    // Встроенные приведения
    // Int -> Real
    if (from == "Int" && to == "Real") return true;
    // Real -> Int (с потерей точности)
    if (from == "Real" && to == "Int") return true;
    // Bool -> Int
    if (from == "Bool" && to == "Int") return true;
    // Int -> Bool (0 = false, != 0 = true)
    if (from == "Int" && to == "Bool") return true;
/*
    // Любой тип -> String (через toString)
    if (to == "String") return true;
*/
    return false;
}

bool    BydaoParser::isTypeExtend( const QString& typeName, const QString& extendTypeName, const BydaoToken& token ) {
    MetaData* metaData = m_metaData[ typeName ];
    if ( ! metaData) {
        error( "Unknown type '" + typeName + "'" , token );
        return false;
    }
    while ( metaData->extend != extendTypeName ) {
        if ( metaData->extend.isEmpty() ) {
            return false;
        }
        metaData = m_metaData[ metaData->extend ];
        if ( ! metaData ) {
            return false;
        }
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
    OperMetaData operMetaData = metaData->objOper( operName );
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
    OperMetaData operMetaData = metaData->objOper( operName );
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

    int valueIndex = 0;  // без инициализации
    if (match(BydaoTokenType::Assign)) {    // с инициализацией

        nextToken();

        int prevSize = m_bytecode.size();

        BydaoToken exprToken = m_current;
        if (!parseExpression()) return false;

        if ( m_typeStack.size() == 0 ) {
            error("Internal error: empty stack of value types");
            return false;
        }
        TypeInfo typeInfo = getLastType();
        if ( typeInfo.type == "Void" ) {
            error("Cannot assign void value", exprToken );
            return false;
        }
        setVariableType( name, typeInfo.type );

        int size = m_bytecode.size();
        if ( size - prevSize == 1 && m_bytecode[ size-1 ].op == BydaoOpCode::PushConst ) {

            // Инициализация переменной константным значением
            valueIndex = -m_bytecode.takeLast().arg1;
        }
        else {

            // Инициализация значением со стека значений
            valueIndex = 1;
        }
    }

    emitCode(BydaoOpCode::VarDecl, nameIndex, valueIndex, token);

    return true;
}

bool BydaoParser::parseConstDecl( bool isPublic ) {
//    qDebug() << "parseConstDecl: start at line" << m_current.line;
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
    BydaoToken exprToken = m_current;
    BydaoConstantFolder folder(this);
    BydaoValue constValue = folder.evaluate();  // Один проход!
    if ( ! constValue.isObject() ) {
        removeVariable(name);
        error("Constant expression expected");
        return false;
    }

    // Обновляем значение в таблице

    VariableInfo info = resolveVariable(name);
    VarScope& varScopes = m_varScopes[ m_scopeLevel ];
    varScopes[ info.varIndex ].varInfo.type = constValue.toObject()->typeName();
    varScopes[ info.varIndex ].varInfo.constValue = constValue;

    // Добавляем значение в таблицу констант и генерируем PUSH
    qint16 constIndex = addConstant( constValue );
    if ( constIndex == 0 ) {
        removeVariable(name);
        error("Invalid constant expression", exprToken);
        return false;
    }

    // Генерируем код
    qint16 nameIndex = addString(name);
    //    qDebug() << "  generating VarDecl for" << name << "index =" << nameIndex;
    emitCode(BydaoOpCode::ConstDecl, nameIndex, constIndex, nameToken);

    return true;
}

bool BydaoParser::parseAssign() {
    QString name = m_current.text;

    // Проверяем self.var = expr
    if (match(BydaoTokenType::Identifier) && name == "self") {
        BydaoToken selfToken = m_current;
        nextToken();

        if (!expect(BydaoTokenType::Dot)) return false;

        if (!match(BydaoTokenType::Identifier)) {
            error("Expected member name after 'self.'");
            return false;
        }

        QString memberName = m_current.text;
        nextToken();

        if (!expect(BydaoTokenType::Assign)) return false;

        // Проверяем, что переменная существует
        VariableInfo info = resolveVariable(memberName);
        if (info.name.isEmpty()) {
            error("Undeclared module variable: " + memberName, selfToken);
            return false;
        }

        // Проверяем, не константа ли
        if (info.isConstant) {
            error("Cannot assign to constant: " + memberName, selfToken);
            return false;
        }

        // Парсим выражение
        if (!parseExpression()) return false;

        TypeInfo exprType = m_typeStack.pop();
        if (exprType.type != info.type && info.type != "Null") {
            if (!canConvertType(exprType.type, info.type)) {
                error("Type mismatch: cannot assign '" + exprType.type + "' to '" + info.type + "'", selfToken);
                return false;
            }
        }

        // Генерируем StoreSelf
        emitCode(BydaoOpCode::StoreSelf, info.varIndex, 0, selfToken);

        // Запоминаем доступ к self
        if (m_currentFunc) {
            m_currentFunc->selfAccesses[memberName] = info.varIndex;
        }

        return true;
    }

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
    TypeInfo exprType = getLastType();

    QString varType = varInfo.type;
    if ( varType != "Null" && varType != exprType.type ) {
        if ( exprType.type != "Any" ) {
            error( "Cannot assign value of type '" + exprType.type + "' to variable of type '" + varType + "'", nameToken );
            return false;
        }
    }
    setVariableType( name, exprType.type );

    int rightIndex = 0;
    int size = m_bytecode.size();
    if ( m_bytecode[size-1].op == BydaoOpCode::VarAdd ) {

        if ( m_bytecode[size-1].arg1 == varInfo.varIndex ) {

            // Левый операнд сложения является переменной присвоения

            int rightVarIndex = m_bytecode.takeLast().arg2;
            emitCode(BydaoOpCode::AddStore, varInfo.varIndex, rightVarIndex, nameToken);
            return true;
        }
    }
    else if ( m_bytecode[size-1].op == BydaoOpCode::Load ) {

        // Присвение переменной

        rightIndex = m_bytecode.takeLast().arg1;
    }
    else if ( m_bytecode[size-1].op == BydaoOpCode::PushConst ) {

        // Присвение константы

        rightIndex = -m_bytecode.takeLast().arg1;
    }

    emitCode(BydaoOpCode::Store, varInfo.varIndex, rightIndex, nameToken);
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
    TypeInfo exprTypeInfo = getLastType();

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

    int rightVarIndex = 0;
    int size = m_bytecode.size();
    if ( m_bytecode[size-1].op == BydaoOpCode::Load ) {

        rightVarIndex = m_bytecode.takeLast().arg1;
    }
    else if ( m_bytecode[size-1].op == BydaoOpCode::PushConst ) {

        rightVarIndex = -m_bytecode.takeLast().arg1;
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
    TypeInfo exprTypeInfo = getLastType();
    if ( exprTypeInfo.type != "Bool" ) {

        if ( ! checkTypeConvert( exprTypeInfo.type, "Bool", exprToken ) ) {
            return false;
        }
    }

    // Условный переход на else/elsif
    int elseJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, ifToken);

    // Запомним состояние стека типов
    int typeStackSize = m_typeStack.size();

    if (!parseBlock(true)) {
        return false;
    }

    TypeInfo ifTypeInfo = TypeInfo("");
    if ( m_exprCount > 0 ) {    // оператор if используется в выражении
        if ( m_typeStack.size() > typeStackSize ) { // после блока if осталось значение
            ifTypeInfo = getLastType();
        }
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
        exprTypeInfo = getLastType();
        if ( exprTypeInfo.type != "Bool" ) {

            if ( ! checkTypeConvert( exprTypeInfo.type, "Bool", exprToken ) ) {
                return false;
            }
        }

        int condJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, elsifToken);

        typeStackSize = m_typeStack.size();

        if (!parseBlock(true)) {
            return false;
        }

        if ( m_exprCount > 0 ) {    // оператор if используется в выражении
            if ( m_typeStack.size() > typeStackSize ) { // после блока if осталось значение
                TypeInfo elsifTypeInfo = getLastType();
                if ( ifTypeInfo.type != elsifTypeInfo.type ) {
                    error( "Expression type after 'elsif'-block not matched 'if'-block", ifToken );
                    return false;
                }
            }
        }

        emitCode(BydaoOpCode::Jump, endJump, 0);

        m_bytecode[condJump].arg1 = m_bytecode.size();
    }

    // Опциональный else
    if (match(BydaoTokenType::Else)) {
        nextToken(); // else

        typeStackSize = m_typeStack.size();

        if (!parseBlock(true)) {
            return false;
        }

        if ( m_exprCount > 0 ) {    // оператор if используется в выражении
            if ( m_typeStack.size() > typeStackSize ) { // после блока if осталось значение
                TypeInfo elseTypeInfo = getLastType();
                if ( ifTypeInfo.type != elseTypeInfo.type ) {
                    error( "Expression type after 'else'-block not matched 'if'-block", ifToken );
                    return false;
                }
            }
        }
    }
    else if ( m_exprCount > 0 ) {   // пропущен else-блок, когда if используется в выражении
        error( "Messed 'else'-block in 'if'-expression", ifToken );
    }

    if ( m_exprCount > 0 ) {
        setLastType( ifTypeInfo );
    }

    // Патчим переход из if
    m_bytecode[endJump].arg1 = m_bytecode.size();

    return true;
}

bool BydaoParser::parseIgnore() {

    BydaoToken ignoreToken = m_current;
    nextToken(); // ignore

    BydaoToken exprToken = m_current;
    if ( ! parseExpression() ) {
        return false;
    }
    TypeInfo exprTypeInfo = getLastType();
    if ( exprTypeInfo.type != "Void" ) {

        emitCode(BydaoOpCode::Ignore, 0, 0);

        // Сохраним тип возвращаемого значения
        setLastType( TypeInfo("Void") );
    }
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
    TypeInfo exprTypeInfo = getLastType();
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

/**
 * Разбор цикла do-while.
 */
bool BydaoParser::parseDoWhile() {
    BydaoToken doToken = m_current;
    nextToken();    // do

    LoopInfo loop;

    m_loopStack.push(loop);
    m_inLoop = true;

    int loopBegin = m_bytecode.size();

    if (!parseBlock(true)) {
        m_inLoop = false;
        m_loopStack.pop();
        return false;
    }

    if ( ! match( BydaoTokenType::While ) ) {
        error( "missed keyword 'while' after block" );
        return false;
    }
    nextToken();    // while

    loop.conditionAddr = m_bytecode.size();

    BydaoToken exprToken = m_current;
    if ( ! parseExpression() ) {
        return false;
    }
    TypeInfo exprTypeInfo = getLastType();
    if ( exprTypeInfo.type != "Bool" ) {

        if ( ! checkTypeConvert( exprTypeInfo.type, "Bool", exprToken ) ) {
            return false;
        }
    }

    emitCode(BydaoOpCode::JumpIfTrue, loopBegin, 0, doToken);

    m_inLoop = false;
    loop = m_loopStack.pop();

    loop.exitAddr = m_bytecode.size();

    patchJumps(loop.breaks, loop.exitAddr);
    patchJumps(loop.nexts, loop.conditionAddr);

    return true;
}

bool BydaoParser::parseIter() {
    BydaoToken token = m_current;
    nextToken(); // iter

    LoopInfo loop;
    loop.conditionAddr = m_bytecode.size();

    BydaoToken exprToken = m_current;
    if (!parseExpression()) {
        return false;
    }
    TypeInfo exprTypeInfo = getLastType();
    MetaData* exprMetaData = m_metaData[ exprTypeInfo.type ];
    if ( ! exprMetaData ) {
        error("Unknown type '" + exprTypeInfo.type + "'", exprToken );
        return false;
    }
    if ( ! exprMetaData->hasObjFunc("iter") ) {
        error("Type '" + exprTypeInfo.type + "' does not have function 'iter'", exprToken );
        return false;
    }
    const FuncMetaData funcIter = exprMetaData->objFunc("iter");
    QString iterType = funcIter.retType;
    MetaData* iterMetaData = m_metaData[ iterType ];
    if ( ! iterMetaData ) {
        error("Unknown type '" + iterType + "'", exprToken );
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

    // Получить итератор и сохранить в переменной
    declareVariable(iterName, iterToken);  // объявляем в текущей области
    VariableInfo iterInfo = resolveVariable(iterName);
    setVariableType( iterName, iterType, true );
    emitCode(BydaoOpCode::GetIter, iterInfo.varIndex, 0, iterToken);

    int loopStart = m_bytecode.size();

    // iter.next()
    emitCode(BydaoOpCode::ItNext, iterInfo.varIndex, 0, token);

    int condJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, token);

    m_loopStack.push(loop);
    m_inLoop = true;

    // Тело цикла - parseBlock сам создаст область видимости
    if (!parseBlock(true)) {
        m_inLoop = false;
        m_loopStack.pop();
        return false;
    }

    m_inLoop = false;
    loop = m_loopStack.pop();

    if ( m_bytecode[ m_bytecode.size() - 1 ].op == BydaoOpCode::ScopeDrop ) {
        loop.nextAddr = m_bytecode.size() - 1;
    }
    else {
        loop.nextAddr = loopStart;
    }

    emitCode(BydaoOpCode::Jump, loopStart, 0);

    loop.exitAddr = m_bytecode.size();

    m_bytecode[condJump].arg1 = loop.exitAddr;

    patchJumps(loop.breaks, loop.exitAddr);
    patchJumps(loop.nexts, loop.nextAddr);

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

    LoopInfo loop;
    loop.conditionAddr = m_bytecode.size();

    BydaoToken exprToken = m_current;
    if (!parseExpression()) {
        return false;
    }
    TypeInfo exprTypeInfo = getLastType();
    MetaData* exprMetaData = m_metaData[ exprTypeInfo.type ];
    if ( ! exprMetaData ) {
        error("Unknown type '" + exprTypeInfo.type + "'", exprToken );
        return false;
    }
    if ( ! exprMetaData->hasObjFunc("iter") ) {
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
    const FuncMetaData funcIter = exprMetaData->objFunc("iter");
    QString iterType = funcIter.retType;
    MetaData* iterMetaData = m_metaData[ iterType ];
    if ( ! iterMetaData ) {
        error("Unknown type '" + iterType + "'", exprToken );
        return false;
    }
    if ( ! iterMetaData->hasObjVar("value") ) {
        error("Iterator of type '" + iterType + "' does not have variable 'value'", exprToken );
        return false;
    }
    VarMetaData varMetaData = iterMetaData->objVar("value");
    setVariableType( varName, varMetaData.type, varMetaData.isConst );

    // Создаём временный итератор
    if ( ! iterMetaData->hasObjFunc("next") ) {
        error("Iterator of type '" + iterType + "' does not have func 'next'", exprToken );
        return false;
    }
    QString tmpIterName = QString("__enum_iter_%1").arg(m_iteratorCounter++);
    declareVariable(tmpIterName, token);  // объявляем в текущей области
    VariableInfo iterInfo = resolveVariable(tmpIterName);
    setVariableType( tmpIterName, iterType );

    // Сохраняем итератор во временной переменной
    emitCode(BydaoOpCode::GetIter, iterInfo.varIndex, 0, token);

    int loopStart = m_bytecode.size();

    // iter.next()
    FuncMetaData nextMetaData = iterMetaData->objFunc( "next" );
    emitCode(BydaoOpCode::ItNext, iterInfo.varIndex, nextMetaData.index, token);

    int condJump = emitCode(BydaoOpCode::JumpIfFalse, 0, 0, token);

    // Получаем значение и сохраняем в переменную
    emitCode(BydaoOpCode::ItValue, iterInfo.varIndex, varInfo.varIndex, token);

    m_loopStack.push(loop);
    m_inLoop = true;

    // Тело цикла - parseBlock сам создаст область видимости
    if (!parseBlock(true)) {
        m_inLoop = false;
        m_loopStack.pop();
        return false;
    }

    m_inLoop = false;
    loop = m_loopStack.pop();

    if ( m_bytecode[ m_bytecode.size() - 1 ].op == BydaoOpCode::ScopeDrop ) {
        loop.nextAddr = m_bytecode.size() - 1;
    }
    else {
        loop.nextAddr = loopStart;
    }

    emitCode(BydaoOpCode::Jump, loopStart, 0);

    loop.exitAddr = m_bytecode.size();

    m_bytecode[condJump].arg1 = loop.exitAddr;

    patchJumps(loop.breaks, loop.exitAddr);
    patchJumps(loop.nexts, loop.nextAddr);

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

bool BydaoParser::parseFuncDecl() {
    BydaoToken funcToken = m_current;

    // Модификаторы

    bool isPublic = false;
    if (match(BydaoTokenType::Pub)) {
        isPublic = true;
        nextToken();
    }
    bool isImmutable = false;
    if (match(BydaoTokenType::Immut)) {
        isImmutable = true;
        nextToken();
    }

    // Ключевое слово func
    if (!expect(BydaoTokenType::Func)) {
        return false;
    }

    // Имя функции
    if (!match(BydaoTokenType::Identifier)) {
        error("Expected function name");
        return false;
    }

    QString funcName = m_current.text;
    BydaoToken nameToken = m_current;
    nextToken();

    // Проверим, что ранее нет переменной с таким именем

    if ( isVariableDeclared( funcName ) ) {
        error( QString( "Variable '%1' already exists" ).arg( funcName ), nameToken );
        return false;
    }

    // Создаём контекст парсинга функции
    FuncParseContext ctx;
    ctx.name = funcName;
    ctx.isPublic = isPublic;
    ctx.isImmutable = isImmutable;
    ctx.isStatic = true;  // Функции модуля всегда статические

    // Парсим параметры
    if (!expect(BydaoTokenType::LParen)) {
        return false;
    }
    while ( ! match(BydaoTokenType::RParen) ) {
        bool isOut = false;
        if (match(BydaoTokenType::Out)) {
            isOut = true;
            nextToken();
        }

        if (!match(BydaoTokenType::Identifier)) {
            error("Expected parameter name");
            return false;
        }
        QString argName = m_current.text;
        BydaoToken argToken = m_current;
        nextToken();

        // Тип аргумента
        QString argType = "Any";
        if (match(BydaoTokenType::Colon)) {
            nextToken();
            argType = m_current.text;
            nextToken();
        }

        // Значение по умолчанию
        QString defVal;
        if (match(BydaoTokenType::Assign)) {
            nextToken();
            BydaoToken defToken = m_current;

            BydaoConstantFolder folder(this);
            BydaoValue constVal = folder.evaluate();
            if (constVal.isNull() && !match(BydaoTokenType::Null)) {
                error("Default value must be constant expression", defToken);
                return false;
            }
            defVal = m_current.text;
            // m_pos уже на следующем токене после evaluate()
        }

        FuncArgMetaData argMeta(argName, argType, isOut, defVal);
        ctx.argList.append(argMeta);

        if ( match(BydaoTokenType::Comma) ) {
            nextToken();
        }
        else if ( ! match(BydaoTokenType::RParen) ) {
            error("Missed comma in argument list");
            return false;
        }
    }

    if (!expect(BydaoTokenType::RParen)) {
        return false;
    }

    // Тип возврата
    if (match(BydaoTokenType::Colon)) {
        nextToken();
        if (!match(BydaoTokenType::Identifier)) {
            error("Expected return type");
            return false;
        }
        ctx.retType = m_current.text;
        nextToken();
    } else {
        ctx.retType = "Void";
    }

    // Добавим определение функции до разбора тела, чтобы можно было
    // определять рекурсивные функции

    // Создаём объект функции
    auto* funcObj = new BydaoFuncObject();
    funcObj->ref();
    funcObj->name = funcName;
    funcObj->entryPc = 0;  // Начало функции — всегда 0 в её пуле
    funcObj->arity = ctx.argList.size();
    funcObj->funcMetaData.retType = ctx.retType;
    funcObj->funcMetaData.isImmutable = ctx.isImmutable;
    funcObj->funcMetaData.isPublic = ctx.isPublic;
    funcObj->funcMetaData.argList = ctx.argList;

    // Байт код будет определен после разбора тела функции
    funcObj->bytecode.clear();

    // Self-фрейм — текущий скоуп модуля
    /*
    funcObj->selfFrame = &m_varScopes;
    // Сохраняем индексы self-переменных
    for (auto it = ctx.selfAccesses.begin(); it != ctx.selfAccesses.end(); ++it) {
        // Находим переменную в текущем скоупе
        int index = m_varScopes.size();
        while (--index >= 0) {
            if (m_varScopes[index].name == it.key()) {
                funcObj->selfVarIndices[it.key()] = index;
                break;
            }
        }
    }
*/

    int funcIdx = m_funcs.size();
    m_funcs.append( funcObj );

    // Объявляем постоянную переменную с именем функции в текущей области видимости
    if ( ! appendVariable(funcName, true, isPublic) ) {
        m_funcs.removeAt( funcIdx );
        delete funcObj;
        return false;
    }
    VariableInfo info = resolveVariable(funcName);
    VarScope& varScopes = m_varScopes[ m_scopeLevel ];
    varScopes[info.varIndex].varInfo.type = "Func";
    varScopes[info.varIndex].varInfo.constValue = BydaoValue(funcObj, TYPE_FUNC);

    // Индекс текущей области видимости для self
    funcObj->selfIndex = m_scopeLevel;

    // Тело функции
    if ( match(BydaoTokenType::LBrace) ) {

        nextToken();

        // Сохраняем текущий контекст
        FuncParseContext* savedCtx = m_currentFunc;
        m_currentFunc = &ctx;

        // Генерируем байт-код тела функции в ctx->m_funcCode
        if ( ! parseFuncBody(ctx) ) {
            m_currentFunc = savedCtx;
            return false;
        }

        m_currentFunc = savedCtx;

        if (!expect(BydaoTokenType::RBrace)) {
            return false;
        }

        // Сохраняем байт-код тела функции
        funcObj->bytecode = std::move(ctx.m_funcCode);
    }

    // Генерируем FuncDecl для функции
    qint16 nameIdx = addString(funcName);
    emitCode( BydaoOpCode::FuncDecl, nameIdx, funcIdx, funcToken );

    return true;
}

bool BydaoParser::parseFuncBody(FuncParseContext& ctx) {
    // Сохраняем текущий байт-код модуля
    QVector<BydaoInstruction> savedCode = std::move(m_bytecode);
    m_bytecode = QVector<BydaoInstruction>();

    // Для тела функции добавляем новую области видимости

    appendScope();

    // Новая область видимости для параметров и локальных переменных
    // TODO: Сброс области видимости нужно сделать до выполнения оператора return

    enterScope();

    // TODO: Добавить локальнную область видимости, и все аргументы и внутренние
    // переменные функции должны быть определены в этой области видимости.
    // Доступ к переменным модуля или вышестоящей функции - через self.

    // Объявляем параметры как локальные переменные
    for (int i = 0; i < ctx.argList.size(); i++) {

        // Аргументы объявляем как константы - их изменять нельзя
        if (!appendVariable(ctx.argList[i].name, true, false)) {
            return false;
        }
        QString type = ctx.argList[i].types.isEmpty() ? "Any" : ctx.argList[i].types[0];
        setVariableType(ctx.argList[i].name, type, false);
    }

    // Парсим тело функции

    while ( ! match( BydaoTokenType::RBrace ) ) {

        BydaoToken statementToken = m_current;
//        int typeStackSize = m_typeStack.size();
        if ( ! parseStatement() ) {
            exitScope();
            return false;
        }
/*
        if ( m_exprCount == 0 ||
            ( m_exprCount > 0 && m_current.type != BydaoTokenType::RBrace ) ) {
            if ( ! checkTypeStack( typeStackSize, statementToken ) ) {
                return false;
            }
        }
*/
    }

    // Проверяем, что функция возвращает значение, если нужно
    // TODO: Анализ потока управления

    // Если последняя инструкция не Return, и функция должна возвращать значение
    bool hasReturn = ! m_bytecode.isEmpty() && m_bytecode.last().op == BydaoOpCode::Return;
    if ( ! hasReturn ) {    // оператора return нет

        if ( ctx.retType != "Void") {   // но функция должна вернуть результат
            error("Function must return a value of type '" + ctx.retType + "'");
            return false;
        }

        // Добавить выход из области видимости и оператор возврата из функции

        exitScope();
        emitCode( BydaoOpCode::Return, 0, 0 );
    }

    // Вернемся к предыдущей области видимости

    dropScope();

    // Сохраняем байт-код функции в пул
    ctx.m_funcCode = std::move(m_bytecode);

    // Восстанавливаем байт-код модуля
    m_bytecode = std::move(savedCode);

    return true;
}

bool BydaoParser::parseFuncCall(const BydaoFuncObject* funcObj) {
    BydaoToken lparenToken = m_current;
    nextToken(); // '('

    const FuncMetaData& metaData = funcObj->funcMetaData;

    int argCount = 0;
    QVector<BydaoValue> args;

    while ( ! match(BydaoTokenType::RParen) ) {
        BydaoToken argToken = m_current;
//        int typeStackSize = m_typeStack.size();

        // Для out-аргументов ожидаем идентификатор переменной
        bool isOut = (argCount < metaData.argList.size() && metaData.argList[argCount].isOut);

        if (isOut) {
            if (!match(BydaoTokenType::Identifier)) {
                error("'out' argument must be a variable name", argToken);
                return false;
            }

            QString varName = m_current.text;
            if (!isVariableDeclared(varName)) {
                error("Undeclared variable: " + varName, argToken);
                return false;
            }

            VariableInfo info = resolveVariable(varName);
            nextToken();

            // Генерируем PUSH_ADDR вместо обычного PUSH
            emitCode(BydaoOpCode::PushAddr, info.varIndex, 0, argToken);
            m_typeStack.push(TypeInfo(info.type, Var));
        } else {
            // Обычный аргумент
            if (!parseExpression()) {
                return false;
            }
        }

        // Проверяем тип аргумента
        if (argCount < metaData.argList.size()) {
            TypeInfo argTypeInfo = m_typeStack.pop();
            const FuncArgMetaData& expected = metaData.argList[argCount];

            bool typeOk = false;
            for (const QString& allowedType : expected.types) {
                if (argTypeInfo.type == allowedType || allowedType == "Any") {
                    typeOk = true;
                    break;
                }
                if (canConvertType(argTypeInfo.type, allowedType)) {
                    // TODO: добавить инструкцию приведения
                    typeOk = true;
                    break;
                }
            }

            if (!typeOk) {
                error(QString("Argument %1 type mismatch: expected %2, got %3")
                          .arg(argCount + 1)
                          .arg(expected.types.join(" or "))
                          .arg(argTypeInfo.type), argToken);
                return false;
            }
        }

        argCount++;

        if (  match(BydaoTokenType::Comma) ) {
            nextToken();
        }
        else if ( ! match(BydaoTokenType::RParen) ) {
            error( "Missed comma in argument list");
            return false;
        }
    }

    expect(BydaoTokenType::RParen);

    // Проверяем количество аргументов
    if (argCount > metaData.argList.size()) {
        error(QString("Too many arguments: expected %1, got %2")
                  .arg(metaData.argList.size())
                  .arg(argCount), lparenToken);
        return false;
    }

    // Добавляем недостающие аргументы из значений по умолчанию
    for (int i = argCount; i < metaData.argList.size(); i++) {
        const FuncArgMetaData& arg = metaData.argList[i];
        if (arg.defVal.isEmpty()) {
            error(QString("Missing argument %1: '%2'").arg(i + 1).arg(arg.name), lparenToken);
            return false;
        }

        // Парсим значение по умолчанию
        BydaoLexer lexer(arg.defVal);
        auto tokens = lexer.tokenize();
        BydaoParser parser( m_moduleName, tokens);
        BydaoConstantFolder folder(&parser);
        BydaoValue defVal = folder.evaluate();

        if (defVal.isNull()) {
            error("Invalid default value for argument: " + arg.name, lparenToken);
            return false;
        }

        qint16 constIdx = addConstant(defVal);
        emitCode(BydaoOpCode::PushConst, constIdx, 0);
    }

    // Генерируем вызов
    bool isVoid = (metaData.retType == "Void");
    emitCode(isVoid ? BydaoOpCode::CallFuncVoid : BydaoOpCode::CallFunc,
             metaData.argList.size(), m_funcs.indexOf( funcObj ), lparenToken);

    // Тип результата
    m_typeStack.push(TypeInfo(metaData.retType, Expr));

    return true;
}

bool BydaoParser::parseReturn() {
    BydaoToken returnToken = m_current;
    nextToken(); // return

    if (!m_currentFunc) {
        error("'return' outside function", returnToken);
        return false;
    }

    if (  ! match(BydaoTokenType::RBrace) ) {

        // Парсим возвращаемое выражение
        if (!parseExpression()) {
            return false;
        }

        TypeInfo exprType = m_typeStack.pop();

        // Проверяем соответствие типа возврата
        if (m_currentFunc->retType == "Void") {
            error("Void function cannot return a value", returnToken);
            return false;
        }

        if (exprType.type != m_currentFunc->retType) {
            if (!canConvertType(exprType.type, m_currentFunc->retType)) {
                error("Return type mismatch: expected '" + m_currentFunc->retType +
                          "', got '" + exprType.type + "'", returnToken);
                return false;
            }
            // TODO: инструкция приведения
        }

        exitScope();
        emitCode(BydaoOpCode::Return, 1, 0, returnToken);
    }
    else {
        if (m_currentFunc->retType != "Void") {
            error("Function must return a value of type '" + m_currentFunc->retType + "'", returnToken);
            return false;
        }
        exitScope();
        emitCode(BydaoOpCode::Return, 0, 0, returnToken);
    }

    return true;
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
    ++m_exprCount;
    bool ok = parseLogicalOr();
    --m_exprCount;
    return ok;
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

        setLastType( TypeInfo( "Bool", Expr ) );

        emitCode(BydaoOpCode::Or, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseLogicalAnd() {
    BydaoToken leftToken = m_current;
    if (!parseBitOr()) return false;

    while (match(BydaoTokenType::And)) {

        // Проверить тип левого операнда операции

        if ( ! checkTypeConvert( "Bool", leftToken ) ) {
            return false;
        }

        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseBitOr()) return false;

        // Проверить тип правого операнда

        if ( ! checkTypeConvert( "Bool", rightToken ) ) {
            return false;
        }

        setLastType( TypeInfo( "Bool", Expr ) );

        emitCode(BydaoOpCode::And, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseBitOr() {
    BydaoToken leftToken = m_current;
    if (!parseBitXor()) return false;

    while ( match(BydaoTokenType::BitOr) ) {

        // Проверить тип левого операнда операции

        if ( ! checkTypeConvert( "Int", leftToken ) ) {
            return false;
        }

        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseBitXor()) return false;

        // Проверить тип правого операнда

        if ( ! checkTypeConvert( "Int", rightToken ) ) {
            return false;
        }

        setLastType( TypeInfo( "Int", Expr ) );

        emitCode(BydaoOpCode::BitOr, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseBitXor() {
    BydaoToken leftToken = m_current;
    if (!parseBitAnd()) return false;

    while ( match(BydaoTokenType::BitXor) ) {

        // Проверить тип левого операнда операции

        if ( ! checkTypeConvert( "Int", leftToken ) ) {
            return false;
        }

        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseBitAnd()) return false;

        // Проверить тип правого операнда

        if ( ! checkTypeConvert( "Int", rightToken ) ) {
            return false;
        }

        setLastType( TypeInfo( "Int", Expr ) );

        emitCode(BydaoOpCode::BitXor, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseBitAnd() {
    BydaoToken leftToken = m_current;
    if (!parseEquality()) return false;

    while ( match(BydaoTokenType::BitAnd) ) {

        // Проверить тип левого операнда операции

        if ( ! checkTypeConvert( "Int", leftToken ) ) {
            return false;
        }

        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseEquality()) return false;

        // Проверить тип правого операнда

        if ( ! checkTypeConvert( "Int", rightToken ) ) {
            return false;
        }

        setLastType( TypeInfo( "Int", Expr ) );

        emitCode(BydaoOpCode::BitAnd, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseEquality() {
    BydaoToken leftToken = m_current;
    if (!parseComparison()) return false;

    while (match(BydaoTokenType::Equal) || match(BydaoTokenType::NotEqual)) {

        TypeInfo leftTypeInfo = getLastType();

        bool isEq = match(BydaoTokenType::Equal);
        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseComparison()) return false;

        TypeInfo rightTypeInfo = getLastType();

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

        setLastType( TypeInfo("Bool", Expr ) );

        emitCode(isEq ? BydaoOpCode::Eq : BydaoOpCode::Neq, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseComparison() {
    BydaoToken leftToken = m_current;
    if (!parseAddition()) return false;

    while (match(BydaoTokenType::Less) || match(BydaoTokenType::Greater) ||
           match(BydaoTokenType::LessEqual) || match(BydaoTokenType::GreaterEqual)) {

        TypeInfo leftTypeInfo = getLastType();

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

        TypeInfo rightTypeInfo = getLastType();

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

        setLastType( TypeInfo("Bool", Expr) );

        int size = m_bytecode.size();
        if ( m_bytecode[size-1].op == BydaoOpCode::Load && m_bytecode[size-2].op == BydaoOpCode::Load ) {

            if ( opCode == BydaoOpCode::Lt ) {
                int rightVarIndex = m_bytecode.takeLast().arg1;
                int leftVarIndex = m_bytecode.takeLast().arg1;
                emitCode(BydaoOpCode::VarLt, leftVarIndex, rightVarIndex, op);
                continue;
            }
        }
        else if ( m_bytecode[size-1].op == BydaoOpCode::PushConst && m_bytecode[size-2].op == BydaoOpCode::Load ) {

            if ( opCode == BydaoOpCode::Lt ) {
                int rightVarIndex = m_bytecode.takeLast().arg1;
                int leftVarIndex = m_bytecode.takeLast().arg1;
                emitCode(BydaoOpCode::VarLt, leftVarIndex, -rightVarIndex, op);
                continue;
            }
        }
        else if ( m_bytecode[size-2].op == BydaoOpCode::Load ) {

            if ( opCode == BydaoOpCode::Lt ) {
                int leftVarIndex = m_bytecode.takeAt( size - 2 ).arg1;
                emitCode(BydaoOpCode::VarLt, leftVarIndex, 0, op);
                continue;
            }
        }

        emitCode(opCode, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseAddition() {
    BydaoToken leftToken = m_current;
    if (!parseTerm()) return false;

    while (match(BydaoTokenType::Plus) || match(BydaoTokenType::Minus)) {

        TypeInfo leftTypeInfo = getLastType();

        bool isPlus = match(BydaoTokenType::Plus);

        BydaoToken op = m_current;
        nextToken();

        BydaoToken rightToken = m_current;
        if (!parseTerm()) return false;
        TypeInfo rightTypeInfo = getLastType();

        // Проверить наличие операции сложения/вычитания для левого операнда

        QString leftType = leftTypeInfo.type;
        QString operName = isPlus ? "add" : "sub";
        if ( leftType != "Any" ) {
            if ( ! checkTypeOper( leftType, operName, leftToken ) ) {
                return false;
            }
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
        setLastType( TypeInfo( resultType, Expr ) );

        if ( isPlus ) {
            int size = m_bytecode.size();
            if ( m_bytecode[size-1].op == BydaoOpCode::Load && m_bytecode[size-2].op == BydaoOpCode::Load ) {

                // Сложение двух переменных

                int rightVarIndex = m_bytecode.takeLast().arg1;
                int leftVarIndex = m_bytecode.takeLast().arg1;
                emitCode( BydaoOpCode::VarAdd, leftVarIndex, rightVarIndex, op);
                continue;
            }
            if ( m_bytecode[size-1].op == BydaoOpCode::PushConst && m_bytecode[size-2].op == BydaoOpCode::Load ) {

                // Сложение переменной и константы

                int constIndex = m_bytecode.takeLast().arg1;
                int leftVarIndex = m_bytecode.takeLast().arg1;
                emitCode( BydaoOpCode::VarAdd, leftVarIndex, -constIndex, op);
                continue;
            }
        }

        emitCode(isPlus ? BydaoOpCode::Add : BydaoOpCode::Sub, 0, 0, op);
    }

    return true;
}

bool BydaoParser::parseTerm() {
    BydaoToken leftToken = m_current;
    if (!parseUnary()) return false;

    while (match(BydaoTokenType::Mul) || match(BydaoTokenType::Div) || match(BydaoTokenType::Mod)) {

        TypeInfo leftTypeInfo = getLastType();

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
        TypeInfo rightTypeInfo = getLastType();

        // Проверить наличие операции для левого операнда

        QString leftType = leftTypeInfo.type;
        if ( ! checkTypeOper( leftType, operName, leftToken ) ) {
            return false;
        }

        // Определить тип результат операции

        QString resultType = leftType;
        if ( leftType != rightTypeInfo.type ) {

            // Проверить возможность использования правого операнда в операции
            // умножения/деления/по модулю с левым операндом

            QString rightType = rightTypeInfo.type;
            resultType = getResultType( leftType, rightType, operName, rightToken );
            if ( resultType.isEmpty() ) {
                error( "Operation '" + op.text + "' cannot be applied to a value of type '" + rightType + "'", rightToken );
                return false;
            }
        }
        setLastType( TypeInfo( resultType, Expr ) );

        if ( opCode == BydaoOpCode::Div ) { // оптимизация операции деления

            int size = m_bytecode.size();
            if ( m_bytecode[size-1].op == BydaoOpCode::Load && m_bytecode[size-2].op == BydaoOpCode::Load ) {

                // Деление двух переменных

                int rightVarIndex = m_bytecode.takeLast().arg1;
                int leftVarIndex = m_bytecode.takeLast().arg1;
                emitCode( BydaoOpCode::VarDiv, leftVarIndex, rightVarIndex, op);
                continue;
            }
            if ( m_bytecode[size-1].op == BydaoOpCode::PushConst && m_bytecode[size-2].op == BydaoOpCode::Load ) {

                // Деление переменной и константы

                int constIndex = m_bytecode.takeLast().arg1;
                int leftVarIndex = m_bytecode.takeLast().arg1;
                emitCode( BydaoOpCode::VarDiv, leftVarIndex, -constIndex, op);
                continue;
            }
        }

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
    setLastType( TypeInfo( "Bool", Expr ) );

    emitCode(BydaoOpCode::Not, 0, 0, op);
    return true;
}

bool BydaoParser::parseMinus() {
    BydaoToken op = m_current;
    nextToken();

    BydaoToken token = m_current;
    if ( ! parseUnary() ) return false;
    TypeInfo typeInfo = getLastType();

    if ( ! checkTypeOper( typeInfo.type, "neg", token ) ) {
        return false;
    }
    QString resultType = getResultType( typeInfo.type, "neg", token );
    if ( resultType.isEmpty() ) {
        error( "The '" + op.text + "' operation cannot be applied to a value of type '" + typeInfo.type + "'", token );
        return false;
    }
    setLastType( TypeInfo( resultType, Expr ) );

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
            setLastType( TypeInfo("Int", Const) );
        }
        else if (text.contains('.') || text.contains('e') || text.contains('E')) {
            constIndex = addConstant(text.toDouble());
            setLastType( TypeInfo("Real", Const) );
        }
        else if ( text.contains('b') || text.contains('B')) {
            constIndex = addConstant(text.toLongLong(nullptr,2));
            setLastType( TypeInfo("Int", Const) );
        }
        else {
            constIndex = addConstant(text.toLongLong());
            setLastType( TypeInfo("Int", Const) );
        }

        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        nextToken();
        return true;
    }

    if (match(BydaoTokenType::String)) {
        QString text = m_current.text;
        qint16 constIndex = addConstant(text);
        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        setLastType( TypeInfo("String", Const) );
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
        setLastType( TypeInfo("Bool", Const) );
        nextToken();
        return true;
    }

    if (match(BydaoTokenType::True)) {
        qint16 constIndex = addConstant(true);
        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        setLastType( TypeInfo("Bool", Const) );
        nextToken();
        return true;
    }

    if (match(BydaoTokenType::Null)) {
        qint16 constIndex = addNullConstant();
        emitCode(BydaoOpCode::PushConst, constIndex, 0, m_current);
        setLastType( TypeInfo("Null", Const) );
        nextToken();
        return true;
    }

    return false;
}

bool BydaoParser::parsePrimary() {
    // Запоминаем позицию для возможного восстановления
    int savedPos = m_pos;
    BydaoToken savedToken = m_current;

    if ( match( BydaoTokenType::If ) ) {
        return parseIf();
    }

    // Обрабатываем идентификаторы (переменные, типы, модули)
    if (match(BydaoTokenType::Identifier)) {
        QString name = m_current.text;
        BydaoToken nameToken = m_current;

        // Проверяем self
        if (name == "self") {
            if (!m_currentFunc) {
                error("'self' can only be used inside module functions", nameToken);
                return false;
            }

            nextToken(); // self

            if (!match(BydaoTokenType::Dot)) {
                error("Expected '.' after 'self'", nameToken);
                return false;
            }
            nextToken(); // '.'

            if (!match(BydaoTokenType::Identifier)) {
                error("Expected member name after 'self.'");
                return false;
            }

            QString memberName = m_current.text;
            nextToken();

            // Проверяем, есть ли такая переменная в модуле
            VariableInfo info = resolveVariable(memberName);
            if (info.name.isEmpty()) {
                error("Undeclared module variable: " + memberName, nameToken);
                return false;
            }

            // Запоминаем, что функция обращается к этой переменной self
            m_currentFunc->selfAccesses[memberName] = info.varIndex;

            // Генерируем LoadSelf
            emitCode(BydaoOpCode::LoadSelf, info.varIndex, 0, nameToken);
            m_typeStack.push(TypeInfo(info.type, Var));

            return true;
        }

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
                setLastType( TypeInfo( name, Module ) );
            }
            else {                              // внутренний тип/класс
                // Встроенный тип: Int, String, Array и т.д.
                nextToken(); // съедаем имя типа
                qint16 nameIdx = addString(name);
                emitCode(BydaoOpCode::UseBuiltin, m_builtinTypes.indexOf( name ), nameIdx, nameToken);
                setLastType( TypeInfo( name, Type ) );
            }
        }
        else if (isVariableDeclared(name)) {    // обычная переменная
            nextToken(); // съедаем имя переменной
            VariableInfo info = resolveVariable(name);

            // Проверяем, является ли переменная функцией
            // и есть ли за ней '('
            if (info.type == "Func" && match(BydaoTokenType::LParen)) {
                // Вызов функции
                BydaoValue funcVal = info.constValue;
                BydaoObject* obj = funcVal.toObject();
                if ( obj && obj->typeName() == "Func") {
                    auto* func = static_cast<BydaoFuncObject*>(funcVal.toObject());
                    return parseFuncCall( func );
                }
                error( QString("Function '%1' is not define").arg( name ), nameToken );
                return false;
            }

            emitCode(BydaoOpCode::Load, info.varIndex, 0, nameToken);
            setLastType( TypeInfo( info.type, Var ) );
        }
        else {
            error( QString( "Undeclared identifier '%1'" ).arg( name ) );
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
            if ( match(  BydaoTokenType::Assign ) ) {
                return parseMemberAssing();
            }
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

    bool createObj = false;
    bool callFunc = false;
    QString retType;

    const TypeInfo& typeInfo = m_typeStack.top();
    QString memberName = typeInfo.member;

    MetaData* metaData = m_metaData[ typeInfo.type ];
    if ( memberName.isEmpty() ) {
        if (typeInfo.operand == Var && typeInfo.type == "Func") {
            // Это вызов функции, а не метода
            // Генерируем CallFunc вместо Call
            callFunc = true;
        }
        else {
            if ( ! metaData->hasTypeFunc( "new" ) ) {
                error("Cannot create object of type '" + typeInfo.type + "'", token );
                return false;
            }
            memberName = "new";
            createObj = true;
        }
    }
    FuncMetaData func;
    if ( callFunc ) {

        // TODO: получить метаданные функции модуля

    }
    else if ( typeInfo.operand == Module || typeInfo.operand == Type ) {
        func = metaData->typeFunc( memberName );
    }
    else {
        func = metaData->objFunc( memberName );
    }

    // Парсим аргументы, если они есть

    int argCount = 0;
    if ( ! match(BydaoTokenType::RParen) ) {
        do {

            BydaoToken argToken = m_current;
            if ( argCount >= func.argList.size() ) {
                error( QString( "Invalid argument count"), argToken );
                return false;
            }

            if (!parseExpression()) {
                error( QString( "Invalid expression in argument %1 function '%2'").arg( argCount + 1 ).arg( memberName ) );
                return false;
            }

            const QStringList& argTypeList = func.argList[ argCount ].types;    // допустимые типы аргумента функции

            TypeInfo exprTypeInfo = getLastType();      // тип выражения аргумента
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
                    if ( exprMetaData->hasObjFunc( QString("to%1").arg(argType) ) ) {
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

    // Проверить число аргументов

    if ( argCount > func.argCount() ) {
        error( QString( "Invalid argument count for function '%1'").arg( memberName ) );
        return false;
    }

    // Добавить недостающие аргументы значениями по умолчанию

    if ( argCount < func.argCount() ) {

        for ( ; argCount < func.argCount(); ++argCount ) {

            QString defaultExpr = func.argList[ argCount ].defVal;
            if ( defaultExpr.isEmpty() ) {
                error( QString( "No default value specified for argument %1 of function '%2'" ).arg( argCount + 1 ).arg(  memberName ), token );
                return false;
            }

            BydaoLexer lexer( defaultExpr );
            auto tokens = lexer.tokenize();

            if ( ! lexer.errorMessage().isEmpty() ) {
                error( QString( "Error on compile default value  for argument %1 of function '%2': " + lexer.errorMessage() ).arg( argCount + 1 ).arg(  memberName ), token );
                return false;
            }

            BydaoParser parser( m_moduleName, tokens );

            BydaoConstantFolder folder( &parser );
            BydaoValue constValue = folder.evaluate();  // Один проход!
            if ( constValue.isObject() ) {
                qint64 constIndex = addConstant( constValue );
                if ( constIndex > 0 ) {
                    emitCode( BydaoOpCode::PushConst, constIndex, 0, token );
                    continue;
                }
            }
            error( QString( "Invalid default value specified for argument %1 of function '%2'" ).arg( argCount + 1 ).arg(  memberName ), token );
            return false;
        }
    }

    // Ожидаем закрывающую скобку
    if (!expect(BydaoTokenType::RParen)) {
        error("Expected ')' after arguments");
        return false;
    }

    // Удалим информацию о типе и методе
    getLastType();

    // Сохраним тип возвращаемого значения
    setLastType( func.retType );

    // Генерируем инструкцию CALL
    // Объект для вызова уже лежит на стеке (результат предыдущих операций)
    // Аргументы лежат на стеке в правильном порядке (благодаря parseExpression)
    if ( createObj ) {
        emitCode( BydaoOpCode::NewObj, argCount, func.index, token );
    }
    else if ( callFunc ) {
        emitCode( retType == "Void" ? BydaoOpCode::CallFuncVoid : BydaoOpCode::CallFunc, argCount, 0, token);
    }
    else {
        emitCode( func.retType == "Void" ? BydaoOpCode::CallVoid : BydaoOpCode::Call, argCount, func.index, token );
    }

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
    TypeInfo& objTypeInfo = m_typeStack.top();
    bool thisIsType = ( objTypeInfo.operand == Module || objTypeInfo.operand == Type );
    QString objType = objTypeInfo.type;
    if ( /* typeInfo.type != "Any" && */ ! m_metaData.contains( objType ) ) {
//        qDebug() << "Check member for type" + typeInfo.type;
        error("Not found metadata for " + objType, memberToken );
        return false;
    }
    MetaData* objMetaData = m_metaData[ objType ];

    // Добавляем имя члена в таблицу строк
    qint16 memberIdx = addString(memberName);

    // Пропускаем имя члена
    nextToken();

    // Смотрим, что идёт после имени члена
    if (match(BydaoTokenType::LParen)) {            // это вызов метода

        // Проверим, что у текущего типа есть такая функция

        if ( thisIsType ) {
            if ( ! objMetaData->hasTypeFunc( memberName ) ) {
                error("Type '" + objType + "' does not have function '" + memberName + "'", memberToken);
                return false;
            }
        }
        else {
            if ( ! objMetaData->hasObjFunc( memberName ) ) {
                error("Object of type '" + objType + "' does not have function '" + memberName + "'", memberToken);
                return false;
            }
        }

        // Запомним название метода

        objTypeInfo.member = memberName;

        FuncMetaData func = thisIsType ? objMetaData->typeFunc( memberName ) : objMetaData->objFunc( memberName );
        if ( func.index < 0 ) {

            // Это вызов метода по имени - используем METHOD

            emitCode(BydaoOpCode::Method, memberIdx, 0, memberToken);
        }
    }
    else if ( match( BydaoTokenType::Assign ) ) {   // это присвоение переменной

        // Проверим, что у текущего типа есть такая переменная и ей можно присвоить значение

        if ( thisIsType ) {
            if ( /* typeInfo.type != "Any" && */ ! objMetaData->hasTypeVar( memberName ) ) {
                error("Type '" + objType + "' does not have member '" + memberName + "'", memberToken );
                return false;
            }
        }
        else {
            if ( /* typeInfo.type != "Any" && */ ! objMetaData->hasObjVar( memberName ) ) {
                error("Object of type '" + objType + "' does not have member '" + memberName + "'", memberToken );
                return false;
            }
        }

        const VarMetaData varMetaData = thisIsType ? objMetaData->typeVar( memberName ) : objMetaData->objVar( memberName );
        if ( varMetaData.isConst ) {
            error( QString( "Member '%1' of '%2' is read only" ).arg( memberName, objType ), memberToken );
            return false;
        }

        nextToken();    // забрали '='

        BydaoToken exprToken = m_current;
        if (!parseExpression()) {
            return false;
        }
        TypeInfo exprTypeInfo = getLastType();
        QString exprType = exprTypeInfo.type;

        QString memberType = varMetaData.type;
        if ( memberType != exprType ) {
            error( "Cannot assign value of type '" + exprType + "' to member of type '" + memberType + "'", exprToken );
            return false;
        }

        getLastType();
        emitCode(BydaoOpCode::SetMember, memberIdx, 0, exprToken);

    }
    else {                                          // это чтение переменной

        // Проверим, что у текущего типа есть такая переменная

        QString memberType;
        if ( thisIsType ) {
            if ( /* typeInfo.type != "Any" && */ ! objMetaData->hasTypeVar( memberName ) ) {
                error("Type '" + objType + "' does not have member '" + memberName + "'", memberToken );
                return false;
            }
            memberType = objMetaData->typeVar( memberName ).type;
        }
        else {
            if ( /* typeInfo.type != "Any" && */ ! objMetaData->hasObjVar( memberName ) ) {
                error("Object of type '" + objType + "' does not have member '" + memberName + "'", memberToken );
                return false;
            }
            memberType = objMetaData->objVar( memberName ).type;
        }

        // Сохраним тип переменной

        getLastType();
        setLastType( objType != "Any" ? memberType : "Any" );

        // Это доступ к свойству - используем MEMBER

        int varIndex = 0;
        if ( m_bytecode[m_bytecode.size()-1].op == BydaoOpCode::Load ) {    // это член переменной

            varIndex = m_bytecode.takeLast().arg1;
            if ( isTypeExtend( objType, "Iter", memberToken ) ) {       // объект является типом итегратора

                if ( memberName == "value" ) {

                    // Получаем значение итератора и сохраняем на стеке
                    emitCode(BydaoOpCode::ItValue, varIndex, 0, memberToken);
                    return true;
                }
                if ( memberName == "key" ) {

                    // Получаем значение итератора и сохраняем на стеке
                    emitCode(BydaoOpCode::ItKey, varIndex, 0, memberToken);
                    return true;
                }
            }
        }
        emitCode(BydaoOpCode::Member, memberIdx, varIndex, memberToken);
    }

    return true;
}

bool    BydaoParser::parseMemberAssing() {

    TypeInfo memberTypeInfo = getLastType();
    BydaoInstruction memberInst = m_bytecode.takeLast();

    // Символ '=' уже забрали.

    nextToken();

    BydaoToken exprToken = m_current;
    if (!parseExpression()) {
        return false;
    }
    TypeInfo exprTypeInfo = getLastType();
    QString exprType = exprTypeInfo.type;

    QString memberType = memberTypeInfo.type;
    if ( memberType != exprType ) {
        error( "Cannot assign value of type '" + exprType + "' to member of type '" + memberType + "'", exprToken );
        return false;
    }

    emitCode(BydaoOpCode::SetMember, memberInst.arg1, memberInst.arg2, exprToken);
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

    while ( ! match(BydaoTokenType::RBracket) ) {

        if ( ! parseExpression() ) return false;
        ++elementCount;

        getLastType();

        if ( match(BydaoTokenType::Comma) ) {
            nextToken();
        }
        else if ( ! match(BydaoTokenType::RBracket) ) {
            error( "Missed comma in array");
            return false;
        }
    }
    if ( ! expect(BydaoTokenType::RBracket) ) return false;

    setLastType( TypeInfo( "Array", Expr ) );

    // Генерируем PushArray с количеством элементов
    // Элементы уже на стеке в правильном порядке (благодаря parseExpression)
    emitCode(BydaoOpCode::PushArray, elementCount, 0, token);

    return true;
}

} // namespace BydaoScript
