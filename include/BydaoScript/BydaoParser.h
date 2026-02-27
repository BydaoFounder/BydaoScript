#pragma once

#include "BydaoLexer.h"
#include "BydaoBytecode.h"
#include "BydaoModule.h"  // +++ ВАЖНО: для BydaoModuleInfo +++
#include <QStack>
#include <QSet>
#include <QMap>
#include <QHash>         // +++ ДОБАВИТЬ +++

namespace BydaoScript {

using BydaoScope = QSet<QString>;

class BydaoParser {
public:
    explicit BydaoParser(const QVector<BydaoToken>& tokens);
    ~BydaoParser();  // +++ ДОБАВИТЬ ДЕСТРУКТОР ДЛЯ ОЧИСТКИ КЭША +++
    
    bool parse();
    QVector<BydaoInstruction> takeBytecode() { return std::move(m_bytecode); }
    QStringList errors() const { return m_errors; }
    
    // Регистрация модулей (теперь только через загрузку info)
    void addModulePath(const QString& path);  // +++ ДОБАВИТЬ +++
    bool loadModuleInfo(const QString& name); // +++ ЗАГРУЗКА INFO +++

private:
    // Грамматика
    bool parseProgram();
    bool parseStatement();
    bool parseBlock(bool requireBraces = true);
    bool parseVarDecl();
    bool parseAssign();
    bool parseWhile();
    bool parseIf();
    bool parseIter();
    bool parseEnum();
    bool parseBreakNext();
    bool parseUse();
    bool parseExpression();
    bool parseLogicalOr();
    bool parseLogicalAnd();
    bool parseEquality();
    bool parseComparison();
    bool parseAddition();
    bool parseTerm();
    bool parseUnary();
    bool parsePrimary();
    bool parseMember(bool canAssign = false);
    bool parseCall(const QString& object);
    bool parseArrayLiteral();

    // Helpers
    void nextToken();
    bool match(BydaoTokenType type);
    bool expect(BydaoTokenType type);
    bool peek(BydaoTokenType type, int offset = 1) const;
    void error(const QString& msg);

    // Code generation
    int emitCode(BydaoOpCode op, QString arg = "", const BydaoToken& token = BydaoToken());
    void emitLabel(int label);
    int newLabel();
    void patchJump(int instrIndex);

    // Semantic analysis
    void enterScope();
    void exitScope();
    void declareVariable(const QString& name, const BydaoToken& token);
    bool isVariableDeclared(const QString& name);
    void undeclareVariable(const QString& name);
    
    // +++ НОВОЕ: РАБОТА С МОДУЛЯМИ +++
    bool isModule(const QString& name);
    bool isModuleMember(const QString& moduleName, const QString& member);
    int  methodArgCount(const QString& moduleName, const QString& method);
    QStringList getMethodArgs(const QString& moduleName, const QString& method);
    BydaoModuleInfo* getModuleInfo(const QString& name);
    void clearModuleCache();  // для сброса при повторном парсинге

    // Поддержка встроенных типов
    struct BuiltinTypeInfo {
        QString name;
        QHash<QString, int> methods;  // имя метода -> количество аргументов (для проверки)
    };
    QHash<QString, BuiltinTypeInfo> m_builtinTypes;
    void initBuiltinTypes();
    bool isBuiltinTypeName(const QString& name) const;

    // Данные
    QVector<BydaoToken> m_tokens;
    int m_pos;
    BydaoToken m_current;

    QVector<BydaoInstruction> m_bytecode;
    QStringList m_errors;

    QMap<int, int> m_labels;

    struct ScopeInfo {
        int beginInstrIndex;      // индекс инструкции SCOPEBEG
        bool hasVariables;        // были ли переменные в этой области
    };
    QStack<ScopeInfo> m_scopeStack;

    QStack<QSet<QString>> m_scopes;

    int m_labelCounter;

    struct LoopInfo {
        int conditionAddr;          // адрес начала условия
        int nextAddr;               // адрес next-оператора (станет известен позже)
        int exitAddr;               // адрес выхода (станет известен позже)
        QVector<int> breaks;        // индексы инструкций JUMP для break
        QVector<int> nexts;         // индексы инструкций JUMP для next
    };
    QStack<LoopInfo> m_loopStack;   // стек для вложенных циклов
    bool m_inLoop;
    
    // +++ КЭШ МОДУЛЕЙ ДЛЯ ПАРСЕРА +++
    QHash<QString, BydaoModuleInfo*> m_moduleInfoCache;  // имя модуля → информация
    QStringList m_modulePaths;                           // пути поиска модулей

    int m_iteratorCounter;  // счётчик для уникальных имён итераторов
};

} // namespace BydaoScript
