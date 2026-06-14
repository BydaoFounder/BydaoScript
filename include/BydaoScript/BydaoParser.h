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

#include <QStack>
#include <QSet>
#include <QMap>
#include <QHash>

#include "BydaoLexer.h"
#include "BydaoBytecode.h"
#include "BydaoModule.h"
#include "BydaoConstantFolder.h"
#include "BydaoMetaData.h"
#include "BydaoFuncObject.h"

namespace BydaoScript {

// Информация о переменной
struct VariableInfo {
    QString     name;
    QString     type;
    int         varIndex;       // индекс в списке переменных
    bool        isConstant;     // true для констант
    bool        isPublic;       // true для pub-переменных/констант
    BydaoValue  constValue;     // значение константы для вычислителя
};

// Информация о цикле (для break/next)
struct LoopInfo {
    int conditionAddr;   // адрес начала условия
    int nextAddr;        // адрес next-оператора
    int exitAddr;        // адрес выхода
    QVector<int> breaks; // индексы инструкций JUMP для break
    QVector<int> nexts;  // индексы инструкций JUMP для next
};

class BydaoParser {
public:
    explicit BydaoParser( const QString& moduleName, const QVector<BydaoToken>& tokens);
    ~BydaoParser();

    bool parse();
    
    // Результаты парсинга
    QVector<BydaoInstruction> takeBytecode() { return std::move(m_bytecode); }
    QVector<BydaoConstant> takeConstants() { return std::move(m_constants); }
    QVector<QString> takeStringTable() { return std::move(m_stringTable); }
    QVector<BydaoDebugInfo> takeDebugInfo() { return std::move(m_debugInfo); }
    
    QStringList errors() const { return m_errors; }

    // Работа с модулями
    void addModulePath(const QString& path);
    bool loadMetaData(const QString& name);

    const ModuleInfo&   moduleInfo();

private:

    friend class BydaoConstantFolder;

    ModuleInfo  buildModuleInfo(const QString& moduleName);

    // ===== Лексический анализ =====
    void nextToken();
    bool match(BydaoTokenType type);
    bool expect(BydaoTokenType type);
    bool peek(BydaoTokenType type, int offset = 1) const;
    void error(const QString& msg);
    void error(const QString& msg, const BydaoToken& token );

    // ===== Таблицы =====
    qint16 addString(const QString& str);
    qint16 addConstant(const BydaoConstant& c);
    qint16 addConstant(qint64 value);
    qint16 addConstant(double value);
    qint16 addConstant(bool value);
    qint16 addConstant(const QString& strValue);
    qint16 addNullConstant();
    qint16 addConstant( const BydaoValue& val );

    // ===== Генерация кода =====
    qint16 emitCode(BydaoOpCode op, qint16 arg1 = 0, qint16 arg2 = 0);
    qint16 emitCode(BydaoOpCode op, qint16 arg1, qint16 arg2, const BydaoToken& token);
    void patchJumps(const QVector<int>& jumps, int targetAddr);

    // ===== Области видимости =====
    void enterScope();
    void exitScope();
    bool appendVariable(const QString& name, bool isConstant = false, bool isPublic = false);
    void declareVariable(const QString& name, const BydaoToken& token);
    VariableInfo resolveVariable(const QString& name);
    bool isVariableDeclared(const QString& name);
    bool removeVariable(const QString& name);
    void setVariableType( const QString& name, const QString type, bool isConst = false );

    // ===== Семантический анализ =====
    bool isNameToken(BydaoTokenType type) const;

    // ===== Парсинг грамматики =====
    bool parseProgram();
    bool parseStatement();
    bool parseBlock(bool requireBraces = true);

    // Объявления и присваивания
    bool parsePubDecl();
    bool parseVarDecl( bool isPublic = false );
    bool parseConstDecl( bool isPublic = false );
    bool parseAssign();
    bool parseMemberAssing();
    bool parseAddAssign();
    bool parseSubAssign();
    bool parseMulAssign();
    bool parseDivAssign();
    bool parseModAssign();

    // Управляющие конструкции
    bool parseIf();
    bool parseWhile();
    bool parseDoWhile();
    bool parseIter();
    bool parseEnum();
    bool parseBreakNext();
    bool parseUse();
    bool parseIgnore();
    bool parseStopAbort();

    // Выражения (с приоритетами)
    bool parseExpression();
    bool parseLogicalOr();
    bool parseLogicalAnd();
    bool parseBitOr();
    bool parseBitXor();
    bool parseBitAnd();
    bool parseEquality();
    bool parseComparison();
    bool parseAddition();
    bool parseTerm();
    bool parseUnary();
    bool parseNot();
    bool parseMinus();
    bool parsePrimaryBase();
    bool parsePrimary();
    bool parseCallSuffix();
    bool parseMemberSuffix();
    bool parseIndexSuffix();

    // Составные конструкции
    bool parseArrayLiteral();

    QString         m_moduleName;

    ModuleInfo      m_moduleInfo;

    // Информация о текущей парсируемой функции
    struct FuncParseContext {
        QString name;
        bool isPublic;
        bool isImmutable;
        bool isStatic;
        QString retType;
        FuncArgMetaDataList argList;
        QHash<QString, int> selfAccesses;  // переменные self, к которым обращаются
        QVector<BydaoInstruction> m_funcCode;   // байт-код функции во время разбора
    };
    FuncParseContext* m_currentFunc = nullptr;
    QList<BydaoFuncObject*> m_funcs;        // список функция модуля

    // Новые методы парсинга
    bool parseFuncDecl();
    bool parseFuncBody(FuncParseContext& ctx);
    bool parseReturn();
    bool parseLambda();
    bool parseFuncCall(const BydaoFuncObject* funcObj);
    bool parseArguments(FuncArgMetaDataList& argList, QVector<QString>& argNames, QVector<bool>& argIsOut);

    // Проверка типов
    bool canConvertType(const QString& from, const QString& to);
    BydaoValue convertValue(const BydaoValue& val, const QString& toType);

    bool    checkTypeStack( int typeStackSize, const BydaoToken& statementToken );
    bool    checkTypeConvert( const QString& typeName, const BydaoToken& token );
    bool    checkTypeConvert( const QString& fromType, const QString& toType, const BydaoToken& token );
    bool    checkTypeOper( const QString& typeName, const QString& operName, const BydaoToken& token );
    bool    isTypeExtend( const QString& typeName, const QString& extendTypeName, const BydaoToken& token );
    QString getResultType( const QString& leftType, const QString& rightType, const QString& operName, const BydaoToken& token );
    QString getResultType( const QString& type, const QString& operName, const BydaoToken& token );

    // ===== Данные =====
    QVector<BydaoToken> m_tokens;
    int m_pos;
    BydaoToken m_current;

    // Таблицы
    QVector<QString> m_stringTable;
    QHash<QString, qint16> m_stringIndex;
    
    QVector<BydaoConstant> m_constants;
    QHash<QString, qint16> m_constantIndex;

    // Байткод и отладка
    QVector<BydaoInstruction> m_bytecode;
    QVector<BydaoDebugInfo> m_debugInfo;
    
    // Ошибки
    QStringList m_errors;

    struct ScopeItem {
        QString         name;
        VariableInfo    varInfo;        // таблица с информацией о переменных

        ScopeItem(){
        }
        ScopeItem( const QString& name, const VariableInfo& info ) {
            this->name = name;
            this->varInfo = info;
        }
    };
    typedef QList<ScopeItem>    VarScope;   // список переменных (плоская область видимости)
    QList<VarScope>     m_varScopes;        // стек областей видимости:
                                            // индекс 0 - область видимости текущего модуля
                                            // индекс 1 - область видимости функций модуля
                                            // индекс 2 и далее - область функций, вложенных в другие функции

    int                 m_scopeLevel;       // уровень области видимости:
                                            // 0 - уровень модуля
                                            // 1 - уровень функции модуля и т.д.

    void        appendScope();
    void        dropScope();

    QStack< int >       m_scopeStart;       // стек начальных индексов областей видимости
                                            // в списке m_varScopes

    // Циклы
    QStack<LoopInfo> m_loopStack;
    bool m_inLoop;

    // Модули и типы
    QStringList m_modulePaths;

    // Список втсроенных типов
    QList<QString> m_builtinTypes;

    // Счётчики
    int         m_iteratorCounter;
    int         m_exprCount;

    // Мета-данные типов данных
    QHash<QString, MetaData*>   m_metaData;

    enum Operand {
        Unknown,
        Expr,
        Var,
        Const,
        Type,
        Module,
    };

    // Стек типов данных
    struct TypeInfo {
        QString type;
        QString member;
        Operand operand;

        TypeInfo( const QString& type ) {
            this->type = type;
            this->member = QString();
            this->operand = Unknown;
        }

        TypeInfo( const QString& type, Operand operand ) {
            this->type = type;
            this->member = QString();
            this->operand = operand;
        }

        TypeInfo( const QString& type, const QString& member, Operand operand ) {
            this->type = type;
            this->member = member;
            this->operand = operand;
        }
    };
    QStack< TypeInfo >      m_typeStack;

    TypeInfo            getLastType() {
        if ( m_typeStack.isEmpty() ) {
            error( "Undefined expression type", m_current );
            return TypeInfo("Indefined");
        }
        return m_typeStack.pop();
    }

    void                setLastType( const TypeInfo& type ) {
        m_typeStack.push( type );
    }
};

} // namespace BydaoScript
