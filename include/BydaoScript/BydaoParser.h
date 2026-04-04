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

#include "BydaoLexer.h"
#include "BydaoBytecode.h"
#include "BydaoConstantFolder.h"
#include "BydaoMetaData.h"
#include <QStack>
#include <QSet>
#include <QMap>
#include <QHash>

namespace BydaoScript {

// Информация о переменной
struct VariableInfo {
    QString     name;
    QString     type;
    int         varIndex;       // индекс в списке переменных
    bool        isConstant;     // true для констант
    bool        isPublic;       // true для pub-переменных/констант
    BydaoValue* constValue;     // значение константы для вычислителя
};

// Информация о цикле (для break/next)
struct LoopInfo {
    int conditionAddr;   // адрес начала условия
    int nextAddr;        // адрес next-оператора
    int exitAddr;        // адрес выхода
    QList<int> breaks; // индексы инструкций JUMP для break
    QList<int> nexts;  // индексы инструкций JUMP для next
};

// Информация о встроенном типе
struct BuiltinTypeInfo {
    QString name;
    QHash<QString, int> methods;  // имя метода -> количество аргументов (-1 = varargs)
};

class BydaoParser {
public:
    explicit BydaoParser(const QList<BydaoToken>& tokens);
    ~BydaoParser();

    bool parse();
    
    // Результаты парсинга
    QList<BydaoInstruction> takeBytecode() { return std::move(m_bytecode); }
    QList<BydaoConstant> takeConstants() { return std::move(m_constants); }
    QList<QString> takeStringTable() { return std::move(m_stringTable); }
    QList<BydaoDebugInfo> takeDebugInfo() { return std::move(m_debugInfo); }
    
    QStringList errors() const { return m_errors; }

    // Работа с модулями
    void addModulePath(const QString& path);
    bool loadMetaData(const QString& name);

private:

    friend class BydaoConstantFolder;

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
    qint16 addConstant( const BydaoValue* val );

    // ===== Генерация кода =====
    qint16 emitCode(BydaoOpCode op, qint16 arg1 = 0, qint16 arg2 = 0);
    qint16 emitCode(BydaoOpCode op, qint16 arg1, qint16 arg2, const BydaoToken& token);
    void patchJump(int instrIndex);
    void patchJumps(const QList<int>& jumps, int targetAddr);

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
    bool parseIter();
    bool parseEnum();
    bool parseBreakNext();
    bool parseUse();
    
    // Выражения (с приоритетами)
    bool parseExpression();
    bool parseLogicalOr();
    bool parseLogicalAnd();
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

    bool    checkTypeStack( int typeStackSize, const BydaoToken& statementToken );
    bool    checkTypeConvert( const QString& typeName, const BydaoToken& token );
    bool    checkTypeConvert( const QString& fromType, const QString& toType, const BydaoToken& token );
    bool    checkTypeOper( const QString& typeName, const QString& operName, const BydaoToken& token );
    bool    isTypeExtend( const QString& typeName, const QString& extendTypeName, const BydaoToken& token );
    QString getResultType( const QString& leftType, const QString& rightType, const QString& operName, const BydaoToken& token );
    QString getResultType( const QString& type, const QString& operName, const BydaoToken& token );

    // ===== Данные =====
    QList<BydaoToken> m_tokens;
    int m_pos;
    BydaoToken m_current;

    // Таблицы
    QList<QString> m_stringTable;
    QHash<QString, qint16> m_stringIndex;
    
    QList<BydaoConstant> m_constants;
    QHash<QString, qint16> m_constantIndex;

    // Байткод и отладка
    QList<BydaoInstruction> m_bytecode;
    QList<BydaoDebugInfo> m_debugInfo;
    
    // Ошибки
    QStringList m_errors;

    // Метки для переходов
    QMap<int, int> m_labels;

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
    QList<ScopeItem>    m_varScopes;    // список переменных (плоская область видимости)
    QStack< int >       m_scopeStart;   // стек начальных индексов областей видимости
                                        // в списке m_varScopes

    // Циклы
    QStack<LoopInfo> m_loopStack;
    bool m_inLoop;

    // Модули и типы
    QStringList m_modulePaths;

    // Информация о встроенном типе
    QHash<QString, BuiltinTypeInfo> m_builtinTypes;

    // Счётчики
    int m_iteratorCounter;

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
    QStack< TypeInfo >          m_typeStack;
};

} // namespace BydaoScript
