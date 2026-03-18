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
#include "BydaoModule.h"
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
    int         scopeDepth;     // глубина области, где объявлена
    int         varIndex;       // индекс в этой области
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

// Информация о встроенном типе
struct BuiltinTypeInfo {
    QString name;
    QHash<QString, int> methods;  // имя метода -> количество аргументов (-1 = varargs)
};

class BydaoParser {
public:
    explicit BydaoParser(const QVector<BydaoToken>& tokens);
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
    bool loadModuleInfo(const QString& name);
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

    // ===== Генерация кода =====
    qint16 emitCode(BydaoOpCode op, qint16 arg1 = 0, qint16 arg2 = 0);
    qint16 emitCode(BydaoOpCode op, qint16 arg1, qint16 arg2, const BydaoToken& token);
    void patchJump(int instrIndex);
    void patchJumps(const QVector<int>& jumps, int targetAddr);

    // ===== Области видимости =====
    void enterScope();
    void exitScope();
    bool appendVariable(const QString& name, bool isConstant = false, bool isPublic = false);
    void declareVariable(const QString& name, const BydaoToken& token);
    VariableInfo resolveVariable(const QString& name);
    bool isVariableDeclared(const QString& name);
    bool removeVariable(const QString& name);
    void setVariableType( const QString& name, const QString type );

    // ===== Семантический анализ =====
    void initBuiltinTypes();
    bool isBuiltinTypeName(const QString& name) const;
    bool isModule(const QString& name);
    BydaoModuleInfo* getModuleInfo(const QString& name);
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

    bool    checkTypeConvert( const QString& typeName, const BydaoToken& token );

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

    // Метки для переходов
    QMap<int, int> m_labels;

    struct ScopeItem {
        QSet<QString>                varList;   // линейный список переменных в данной области видимости
        QHash<QString, VariableInfo> varInfo;   // таблица с информацией о переменных
    };
    QStack<ScopeItem> m_varScopes;      // стек областей видимости переменных

    // Циклы
    QStack<LoopInfo> m_loopStack;
    bool m_inLoop;

    // Модули и типы
    QHash<QString, BydaoModuleInfo*> m_moduleInfoCache;
    QStringList m_modulePaths;

    // Информация о встроенном типе
    QHash<QString, BuiltinTypeInfo> m_builtinTypes;

    // Счётчики
    int m_iteratorCounter;

    // Мета-данные типов данных
    QHash<QString, MetaData*>   m_metaData;

    // Стек типов данных
    struct TypeInfo {
        QString type;
        QString member;

        TypeInfo( const QString& type ) {
            this->type = type;
            this->member = QString();
        }

        TypeInfo( const QString& type, const QString& member ) {
            this->type = type;
            this->member = member;
        }
    };
    QStack< TypeInfo >          m_typeStack;
};

} // namespace BydaoScript
