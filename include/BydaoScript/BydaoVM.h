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
#include <QHash>
#include <QVector>

#include "BydaoBytecode.h"
#include "BydaoRuntime.h"
#include "BydaoFuncObject.h"

namespace BydaoScript {

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

    void    popTo( BydaoValue& result ) {
        Q_ASSERT(m_stackTop > 0);
        --m_stackTop;
        result = std::move(*reinterpret_cast<BydaoValue*>(&m_storage[m_stackTop * sizeof(BydaoValue)]));
        // Ручной вызов деструктора для опустошённого элемента
        reinterpret_cast<BydaoValue*>(&m_storage[m_stackTop * sizeof(BydaoValue)])->~BydaoValue();
    }

    BydaoValue& top() {
        return *reinterpret_cast<BydaoValue*>(&m_storage[(m_stackTop - 1) * sizeof(BydaoValue)]);
    }

    void    resize( int pos ) { m_stackTop = pos;    }
    int     size() const { return m_stackTop; }
    bool    isEmpty() const { return m_stackTop == 0; }

    void    clear() {
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

struct CallFrame {
    BydaoInstruction*   code;           // предыдущий исполняемый код
    int                 codeSize;       // размер предыдущего кода
    int                 returnPc;       // адрес возврата в предыдущем коде
    int                 scopeDeep;      // глубина стека областей видимости перед вызовом функции
    int                 scopeOffset;
};


class BydaoCallStack {

    static constexpr int MAX_STACK = 1024;

public:
    BydaoCallStack() : m_stackTop(0) {}

    ~BydaoCallStack() {
        // Вызываем деструкторы для всех активных элементов
        for (int i = 0; i < m_stackTop; ++i) {
            reinterpret_cast<CallFrame*>(&m_storage[i * sizeof(CallFrame)])->~CallFrame();
        }
    }

    void push(CallFrame&& v) {
        Q_ASSERT(m_stackTop < MAX_STACK);
        // Placement new: конструируем CallFrame в сырой памяти
        new (&m_storage[m_stackTop * sizeof(CallFrame)]) CallFrame(std::move(v));
        ++m_stackTop;
    }

    // Для lvalue — копирование (безопасно, оригинал остаётся жив)
    void push(const CallFrame& v) {
        new (&m_storage[m_stackTop * sizeof(CallFrame)]) CallFrame(v);
        ++m_stackTop;
    }

    CallFrame pop() {
        Q_ASSERT(m_stackTop > 0);
        --m_stackTop;
        CallFrame result = std::move(*reinterpret_cast<CallFrame*>(&m_storage[m_stackTop * sizeof(CallFrame)]));
        // Ручной вызов деструктора для опустошённого элемента
        reinterpret_cast<CallFrame*>(&m_storage[m_stackTop * sizeof(CallFrame)])->~CallFrame();
        return result;
    }

    void    popTo( CallFrame& result ) {
        Q_ASSERT(m_stackTop > 0);
        --m_stackTop;
        result = std::move(*reinterpret_cast<CallFrame*>(&m_storage[m_stackTop * sizeof(CallFrame)]));
        // Ручной вызов деструктора для опустошённого элемента
        reinterpret_cast<CallFrame*>(&m_storage[m_stackTop * sizeof(CallFrame)])->~CallFrame();
    }

    CallFrame& top() {
        return *reinterpret_cast<CallFrame*>(&m_storage[(m_stackTop - 1) * sizeof(CallFrame)]);
    }

    void    resize( int pos ) { m_stackTop = pos;    }
    int     size() const { return m_stackTop; }
    bool    isEmpty() const { return m_stackTop == 0; }

    void    clear() {
        // Вызываем деструкторы для всех активных элементов
        while ( m_stackTop > 0 ) {
            reinterpret_cast<CallFrame*>(&m_storage[ (--m_stackTop)  * sizeof(CallFrame) ])->~CallFrame();
        }
    }

    // Неконстантная версия (для изменяемого доступа)
    CallFrame& operator[](int index) {
        return *reinterpret_cast<CallFrame*>(&m_storage[index * sizeof(CallFrame)]);
    }

    // Константная версия (для доступа только на чтение)
    const CallFrame& operator[](int index) const {
        return *reinterpret_cast<const CallFrame*>(&m_storage[index * sizeof(CallFrame)]);
    }

private:
    alignas(CallFrame) unsigned char m_storage[sizeof(CallFrame) * MAX_STACK];
    int m_stackTop;
};


class BydaoVM : public BydaoRuntime {
public:
    BydaoVM();
    ~BydaoVM();

    // Методы BydaoRuntime

    QString         moduleName() override {
        return m_moduleName;
    }

    int             line() override {
        int pos = m_pc - 1;
        if ( pos < 0 ) {
            pos = 0;
        }
        if ( pos >= m_bytecodeSize ) {
            pos = m_bytecodeSize - 1;
        }
        return m_bytecode[ pos ].line;
    }

    void            setConfig( BydaoConfig* conf ) override {
        m_config = conf;
    }

    BydaoConfig*    getConfig() override {
        return m_config;
    }

    void            setLogger( BydaoLogger* log ) override {
        m_logger = log;
    }

    BydaoLogger*    getLogger() override {
        return m_logger;
    }

    void    setEnvironment( Environment* env ) override {
        m_environment = env;
    };
    Environment*    getEnvironment() override {
        return m_environment;
    };

    void    setInputData( const QByteArray* inputData ) override {
        m_inputData = inputData;
    }

    const QByteArray* getInputData() const override {
        return m_inputData;
    }

    void        setTraceId( const QString& traceId ) override {
        m_traceId = traceId;
    }
    QString     getTraceId() const override {
        return m_traceId;
    }

    QTextStream* outStream() override { return m_outStream; }
    QTextStream* errStream() override { return m_outStream; }
    void logError(const QString& msg)  override;
    bool callFunction(BydaoValue func, const QVector<BydaoValue>& args, BydaoValue& result) override;

    // Методы виртуальной машины

    void setOutputStream(QTextStream* stream);

    bool loadModule(const ModuleInfo& module);

    // Выполнение
    bool run();
    void stop();

    // Режимы
    void setTraceMode(bool enable) { m_traceMode = enable; }
    void setProfileMode(bool enable) { m_profileMode = enable; }

    // Информация об ошибках
    QString lastError() const override { return m_lastError; }
    int errorLine() const { return m_errorLine; }

    // Профилирование
    struct ProfileItem {
        QString name;
        qint64 time;      // наносекунды
        int count;        // количество вызовов
    };
    QVector<ProfileItem> takeProfile();

    int         exitStatus() {
        return m_exitStatus;
    }

private:

    BydaoConfig*        m_config;
    BydaoLogger*        m_logger;

    BydaoCallStack      m_callStack;            // Стек вызовов
    QList<RuntimeVar>*  m_currentSelfFrame;     // Текущий self-фрейм

    void loadConstants( const QVector<BydaoConstant>& constants );

    // Преобразование значения к нужному типу
    BydaoValue convertValue(const BydaoValue& val, const QString& toType);

    // Выполнение одной инструкции
    inline bool execute(const BydaoInstruction& instr);

    // Вспомогательные методы
    void error(const QString& msg, const BydaoInstruction& instr);
    BydaoValue& getVariable(int varIndex, const BydaoInstruction& instr);
    const BydaoValue& getVariable(int varIndex, const BydaoInstruction& instr) const;
    void setVariable(int varIndex, const BydaoValue& value, const BydaoInstruction& instr);

    void dumpStack(const QString& label = QString());

    QString             m_moduleName;

    // Статус возврата: 0 - нормально, 1 - ошибка
    int                 m_exitStatus;

    struct BuiltinType {
        QString     name;
        BydaoValue  typeValue;
    };
    QList<BuiltinType>  s_builtinTypes;
    BydaoValue&     getBuiltinType(int index);

    // Таблицы из байткода
    QVector<BydaoConstant>      m_constants;        // исходные константы
    QVector<BydaoValue>         m_constantValues;   // готовые значения констант
    QVector<QString>            m_stringTable;      // таблица строк
//    QVector<BydaoInstruction>   m_code;             // код
    BydaoInstruction*           m_bytecode;             // код
    qint32                      m_bytecodeSize;

    // Функции
    QVector< BydaoFuncObject*>  m_funcs;            // функции

    // Состояние выполнения
    int m_pc;                           // счетчик байткода
    bool m_running;                     // флаг выполнения
    BydaoValueStack     m_stack;        // стек значений

    VarScope            m_scopeStack;   // стек областей видимости:
                                        // индекс 0 - область видимости текущего модуля
                                        // индекс 1 - область видимости функций модуля
                                        // индекс 2 и далее - область функций, вложенных в другие функции
    int                 m_scopeOffset;  // смещение индекса переменных в стеке областей видимости

    // Ошибки
    QString             m_lastError;
    int                 m_errorLine;

    // Режимы
    bool                m_traceMode;
    bool                m_profileMode;

    // Профилирование
    struct ProfileData {
        qint64 totalTime;
        int callCount;
    };
    QHash<QString, ProfileData> m_profile;
    qint64 m_lastInstrStart;  // время начала текущей инструкции

    QHash<QString, int> m_moduleList;    // для проверки повторной загрузки модулей

    QTextStream*    m_outStream;
    bool            m_ownOutStream;

    QTextStream*    m_errStream;
    bool            m_ownErrStream;

    Environment*    m_environment;

    const QByteArray*   m_inputData;

    QString         m_traceId;
};

} // namespace BydaoScript
