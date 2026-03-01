#pragma once

#include "BydaoBytecode.h"
#include "BydaoValue.h"
#include <QStack>
#include <QHash>
#include <QVector>

namespace BydaoScript {

// Информация о переменной в runtime
struct RuntimeVar {
    QString name;        // имя переменной (для отладки)
    BydaoValue value;    // значение
};

// Область видимости в runtime
struct RuntimeScope {
    QVector<RuntimeVar> vars;           // переменные в этой области
    QHash<QString, int> nameToIndex;    // имя -> индекс (для быстрого доступа при загрузке)
};

class BydaoVM {
public:
    BydaoVM();
    ~BydaoVM();

    // Загрузка байткода
    bool load(const QVector<BydaoConstant>& constants,
              const QVector<QString>& stringTable,
              const QVector<BydaoInstruction>& code);

    // Выполнение
    bool run();
    void stop();

    // Режимы
    void setTraceMode(bool enable) { m_traceMode = enable; }
    void setProfileMode(bool enable) { m_profileMode = enable; }

    // Информация об ошибках
    QString lastError() const { return m_lastError; }
    int errorLine() const { return m_errorLine; }

    // Профилирование
    struct ProfileItem {
        QString name;
        qint64 time;      // наносекунды
        int count;        // количество вызовов
    };
    QVector<ProfileItem> takeProfile();

private:
    // Выполнение одной инструкции
    bool execute(const BydaoInstruction& instr);

    // Вспомогательные методы
    void error(const QString& msg, const BydaoInstruction& instr);
    BydaoValue& getVariable(int scopeLevel, int varIndex, const BydaoInstruction& instr);
    const BydaoValue& getVariable(int scopeLevel, int varIndex, const BydaoInstruction& instr) const;
    void setVariable(int scopeLevel, int varIndex, const BydaoValue& value, const BydaoInstruction& instr);

    void dumpStack(const QString& label = QString());

    // Таблицы из байткода
    QVector<BydaoConstant> m_constants;      // исходные константы
    QVector<BydaoValue> m_constantValues;    // готовые значения констант
    QVector<QString> m_stringTable;          // таблица строк
    QVector<BydaoInstruction> m_code;         // код

    // Состояние выполнения
    int m_pc;                    // program counter
    bool m_running;              // флаг выполнения
    QStack<BydaoValue> m_stack;  // стек значений
    QStack<RuntimeScope> m_scopeStack;  // стек областей видимости

    // Ошибки
    QString m_lastError;
    int m_errorLine;

    // Режимы
    bool m_traceMode;
    bool m_profileMode;

    // Профилирование
    struct ProfileData {
        qint64 totalTime;
        int callCount;
    };
    QHash<QString, ProfileData> m_profile;
    qint64 m_lastInstrStart;  // время начала текущей инструкции
};

} // namespace BydaoScript
