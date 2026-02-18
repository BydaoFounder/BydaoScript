#pragma once

#include "BydaoLexer.h"
#include "BydaoBytecode.h"
#include "BydaoModule.h"  // +++ ВАЖНО: для BydaoModuleInfo +++
#include <QStack>
#include <QSet>
#include <QMap>
#include <QHash>         // +++ ДОБАВИТЬ +++

namespace BydaoScript {

struct BydaoScope {
    QSet<QString> variables;
    bool isLoop;
    
    BydaoScope() : isLoop(false) {}
    BydaoScope(const QSet<QString>& vars, bool loop) : variables(vars), isLoop(loop) {}
};

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
    bool parseIterEnum(bool isIter);
    bool parseBreakNext();
    bool parseUse();           // ← здесь будет загрузка модуля
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
    void enterScope(bool isLoop = false);
    void exitScope();
    void declareVariable(const QString& name, const BydaoToken& token);
    bool isVariableDeclared(const QString& name);
    
    // +++ НОВОЕ: РАБОТА С МОДУЛЯМИ +++
    bool isModule(const QString& name);
    bool isModuleMember(const QString& moduleName, const QString& member);
    int  methodArgCount(const QString& moduleName, const QString& method);
    QStringList getMethodArgs(const QString& moduleName, const QString& method);
    BydaoModuleInfo* getModuleInfo(const QString& name);
    void clearModuleCache();  // для сброса при повторном парсинге

    // Данные
    QVector<BydaoToken> m_tokens;
    int m_pos;
    BydaoToken m_current;

    QVector<BydaoInstruction> m_bytecode;
    QStringList m_errors;

    QMap<int, int> m_labels;

    QStack<BydaoScope> m_scopes;
    int m_labelCounter;
    bool m_inLoop;
    
    // +++ КЭШ МОДУЛЕЙ ДЛЯ ПАРСЕРА +++
    QHash<QString, BydaoModuleInfo*> m_moduleInfoCache;  // имя модуля → информация
    QStringList m_modulePaths;                           // пути поиска модулей
};

} // namespace BydaoScript
