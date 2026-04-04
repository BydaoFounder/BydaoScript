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

#include "BydaoBytecode.h"
#include "BydaoValue.h"
#include <QStack>
#include <QHash>
#include <QList>

namespace BydaoScript {

// Информация о переменной в runtime
struct RuntimeVar {
    QString     name;       // имя переменной (для отладки)
    BydaoValue* value;      // значение
};

class BydaoVM {
public:
    BydaoVM();
    ~BydaoVM();

    // Загрузка байткода
    bool load(const QList<BydaoConstant>& constants,
              const QList<QString>& stringTable,
              const QList<BydaoInstruction>& code);

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
    QList<ProfileItem> takeProfile();

private:
    // Выполнение одной инструкции
    bool execute(const BydaoInstruction& instr);

    // Вспомогательные методы
    void error(const QString& msg, const BydaoInstruction& instr);

    BydaoValue*     getVariable(int varIndex, const BydaoInstruction& instr);
    void            setVariable(int varIndex, BydaoValue* value, const BydaoInstruction& instr);

    void dumpStack(const QString& label = QString());

    // Таблицы из байткода
    QList<BydaoConstant>    m_constants;        // исходные константы
    BydaoValueList          m_constantValues;   // готовые значения констант
    QList<QString>          m_stringTable;      // таблица строк
    QList<BydaoInstruction> m_code;             // код

    // Состояние выполнения
    int                     m_pc;               // program counter
    bool                    m_running;          // флаг выполнения
    QStack<BydaoValue*>     m_stack;            // стек значений
    QList<RuntimeVar>       m_scopeStack;       // стек областей видимости

    // Ошибки
    QString                 m_lastError;
    int                     m_errorLine;

    // Режимы
    bool                    m_traceMode;
    bool                    m_profileMode;

    // Профилирование
    struct ProfileData {
        qint64  totalTime;
        int     callCount;
    };
    QHash<QString, ProfileData> m_profile;
    qint64                      m_lastInstrStart;  // время начала текущей инструкции

    QHash<QString, int>         m_moduleName;    // для проверки повторной загрузки модулей
};

} // namespace BydaoScript
