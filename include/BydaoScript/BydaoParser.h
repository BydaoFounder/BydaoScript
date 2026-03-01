#pragma once

#include "BydaoLexer.h"
#include "BydaoBytecode.h"
#include "BydaoModule.h"
#include <QStack>
#include <QSet>
#include <QMap>
#include <QHash>

namespace BydaoScript {

// Информация о переменной
struct VariableInfo {
    QString name;
    int scopeDepth;      // глубина области, где объявлена
    int varIndex;        // индекс в этой области
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

private:
    // ===== Лексический анализ =====
    void nextToken();
    bool match(BydaoTokenType type);
    bool expect(BydaoTokenType type);
    bool peek(BydaoTokenType type, int offset = 1) const;
    void error(const QString& msg);

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
    void declareVariable(const QString& name, const BydaoToken& token);
    VariableInfo resolveVariable(const QString& name);
    bool isVariableDeclared(const QString& name);

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
    bool parseVarDecl();
    bool parseAssign();
    
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
    bool parsePrimaryBase();
    bool parsePrimary();
    bool parseCallSuffix();
    bool parseMemberSuffix();
    bool parseIndexSuffix();

    // Составные конструкции
    bool parseMember(bool canAssign = false);
    bool parseCall();
    bool parseArrayLiteral();

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

    // Области видимости
    struct ScopeInfo {
        int beginInstrIndex;      // индекс инструкции SCOPEBEG
        bool hasVariables;        // были ли переменные в этой области
        int scopeDepth;           // глубина области
    };

    // В классе BydaoParser:
    QStack<ScopeInfo> m_scopeStack;  // стек с информацией об областях
    QStack<QSet<QString>> m_varScopes; // стек с именами переменных

    QHash<QString, VariableInfo> m_varMap;  // глобальная карта переменных
    int m_currentScopeDepth;

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
};

} // namespace BydaoScript
