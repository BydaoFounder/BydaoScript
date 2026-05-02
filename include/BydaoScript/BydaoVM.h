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
#include <QVector>

namespace BydaoScript {

// Информация о переменной в runtime
struct RuntimeVar {
    QString name;        // имя переменной (для отладки)
    BydaoValue value;    // значение
};

class BydaoValueStack {

static constexpr int MAX_STACK = 1024;

public:
    BydaoValueStack() : m_stackTop(0) {}

    ~BydaoValueStack() {
        // Вызываем деструкторы для всех активных элементов
        for (int i = 0; i < m_stackTop; ++i) {
            reinterpret_cast<BydaoValue*>(&m_storage[i * sizeof(BydaoValue)])->~BydaoValue();
        }
    }

    void push(BydaoValue&& v) {
        Q_ASSERT(m_stackTop < MAX_STACK);
        // Placement new: конструируем BydaoValue в сырой памяти
        new (&m_storage[m_stackTop * sizeof(BydaoValue)]) BydaoValue(std::move(v));
        ++m_stackTop;
    }

    // Для lvalue — копирование (безопасно, оригинал остаётся жив)
    void push(const BydaoValue& v) {
        new (&m_storage[m_stackTop * sizeof(BydaoValue)]) BydaoValue(v);
        ++m_stackTop;
    }

    BydaoValue pop() {
        Q_ASSERT(m_stackTop > 0);
        --m_stackTop;
        BydaoValue result = std::move(*reinterpret_cast<BydaoValue*>(&m_storage[m_stackTop * sizeof(BydaoValue)]));
        // Ручной вызов деструктора для опустошённого элемента
        reinterpret_cast<BydaoValue*>(&m_storage[m_stackTop * sizeof(BydaoValue)])->~BydaoValue();
        return result;
    }

    BydaoValue& top() {
        return *reinterpret_cast<BydaoValue*>(&m_storage[(m_stackTop - 1) * sizeof(BydaoValue)]);
    }

    int size() const { return m_stackTop; }
    bool isEmpty() const { return m_stackTop == 0; }

    void clear() {
        // Вызываем деструкторы для всех активных элементов
        while ( m_stackTop > 0 ) {
            reinterpret_cast<BydaoValue*>(&m_storage[ (--m_stackTop)  * sizeof(BydaoValue) ])->~BydaoValue();
        }
    }

    // Неконстантная версия (для изменяемого доступа)
    BydaoValue& operator[](int index) {
        return *reinterpret_cast<BydaoValue*>(&m_storage[index * sizeof(BydaoValue)]);
    }

    // Константная версия (для доступа только на чтение)
    const BydaoValue& operator[](int index) const {
        return *reinterpret_cast<const BydaoValue*>(&m_storage[index * sizeof(BydaoValue)]);
    }

private:
    alignas(BydaoValue) unsigned char m_storage[sizeof(BydaoValue) * MAX_STACK];
    int m_stackTop;
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
    inline bool execute(const BydaoInstruction& instr);

    // Вспомогательные методы
    void error(const QString& msg, const BydaoInstruction& instr);
    BydaoValue& getVariable(int varIndex, const BydaoInstruction& instr);
    const BydaoValue& getVariable(int varIndex, const BydaoInstruction& instr) const;
    void setVariable(int varIndex, const BydaoValue& value, const BydaoInstruction& instr);

    void dumpStack(const QString& label = QString());

    // Таблицы из байткода
    QVector<BydaoConstant> m_constants;      // исходные константы
    QVector<BydaoValue> m_constantValues;    // готовые значения констант
    QVector<QString> m_stringTable;          // таблица строк
    QVector<BydaoInstruction> m_code;         // код

    // Состояние выполнения
    int m_pc;                           // program counter
    bool m_running;                     // флаг выполнения
    BydaoValueStack     m_stack;        // стек значений
    QList<RuntimeVar>   m_scopeStack;   // стек областей видимости

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

    QHash<QString, int> m_moduleName;    // для проверки повторной загрузки модулей

};

} // namespace BydaoScript
