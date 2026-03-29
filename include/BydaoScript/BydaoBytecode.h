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

#include <QString>
#include <QVector>
#include <QMetaType>

namespace BydaoScript {

// Инструкции байткода (1 байт)
enum BydaoOpCode : quint8 {
    // Управление
    Nop = 0, Halt,

    // Переменные и области видимости
    ConstDecl,      // определение константы: arg1 - индекс имени, arg2 - индекс константы
    VarDecl,        // arg1 = индекс в таблице строк (имя переменной)
    Drop,           // arg1 = индекс переменной
    Load,           // arg1 = индекс переменной
    Store,          // arg1 = индекс переменной
    AddStore,       // arg1 = индекс переменной, arg2 >= 0 - индекс второй переменной
    SubStore,       // arg1 = индекс переменной
    MulStore,       // arg1 = индекс переменной
    DivStore,       // arg1 = индекс переменной
    ModStore,       // arg1 = индекс переменной

    // Константы
    PushConst,      // arg1 = индекс в таблице констант

    // Арифметика
    Add, Sub, Neg,
    Mul, Div, Mod,
    VarAdd,         // сложение двух переменных: arg1 - левая, arg2 - правая

    // Сравнение
    Eq, Neq, Lt, Gt, Le, Ge,

    // Сравнение переменных
    VarLt,

    // Логические
    And, Or, Not,

    // Итераторы
    GetIter,     // obj.iter() - получение итератора
    ItNext,      // вызов iter.next() -> bool
    ItValue,     // iter.value -> значение
    ItKey,       // iter.key -> ключ

    // Доступ к членам
    Member,         // arg1 = индекс имени свойства
    SetMember,      // сохранить значение со стека в переменной объекта, arg1 = индекс имени свойства
    Method,         // arg1 = индекс имени метода (подготовка к вызову)
    Index,          // arg1 = не используется (индекс на стеке)
    Call,           // arg1 = количество аргументов
    CallVoid,       // arg1 = количество аргументов

    // Переходы
    Jump,           // arg1 = смещение (индекс инструкции)
    JumpIfFalse,    // arg1 = смещение
    JumpIfTrue,     // arg1 = смещение

    // Области видимости
    ScopeDrop,      // сброс до заданного размера, arg1 - размер списка переменных

    // Модули и типы
    UseModule,      // arg1 = индекс имени модуля в таблице строк
    TypeClass,      // arg1 = индекс имени типа в таблице строк

    // Массивы
    PushArray       // arg1 = количество элементов
};

// Инструкция (9 байт)
struct BydaoInstruction {
    BydaoOpCode op;     // 1 байт
    qint16 arg1;        // 2 байта
    qint16 arg2;        // 2 байта
    qint16 line;        // 2 байта (опционально, для отладки)
    qint16 column;      // 2 байта (опционально, для отладки)

    BydaoInstruction(BydaoOpCode o = BydaoOpCode::Nop, 
                     qint16 a1 = 0, qint16 a2 = 0,
                     qint16 l = 0, qint16 c = 0)
        : op(o), arg1(a1), arg2(a2), line(l), column(c) {}
};

// Типы констант
enum BydaoConstantType : quint8 {
    CONST_INT,
    CONST_REAL,
    CONST_STRING,
    CONST_BOOL,
    CONST_NULL
};

// Константа
struct BydaoConstant {
    BydaoConstantType type;  // 1 байт
    
    union {
        qint64 intValue;     // 8 байт
        double realValue;    // 8 байт
        quint32 stringIndex; // 4 байта (индекс в таблице строк)
        quint8 boolValue;    // 1 байт
    };
    
    BydaoConstant() : type(CONST_NULL), intValue(0) {}
    explicit BydaoConstant(qint64 v) : type(CONST_INT), intValue(v) {}
    explicit BydaoConstant(double v) : type(CONST_REAL), realValue(v) {}
    explicit BydaoConstant(bool v) : type(CONST_BOOL), boolValue(v ? 1 : 0) {}
    explicit BydaoConstant(quint32 strIdx) : type(CONST_STRING), stringIndex(strIdx) {}
};

// Отладочная информация (опционально)
struct BydaoDebugInfo {
    quint32 instructionIndex;  // индекс инструкции в коде
    qint16 line;
    qint16 column;
    qint16 fileIndex;          // индекс имени файла в таблице строк
};

// Основной класс для работы с байткодом
class BydaoBytecode {
public:
    // Версии формата
    static const quint32 MAGIC = 0x42594453;  // "BYDS"
    static const quint32 VERSION = 0x00020000; // версия 2.0
    
    // Преобразование опкода в строку (для дизассемблера)
    static QString opcodeToString(BydaoOpCode op);
    
    // Сохранение
    static bool save(const QVector<BydaoConstant>& constants,
                     const QVector<QString>& stringTable,
                     const QVector<BydaoInstruction>& code,
                     const QString& filename,
                     const QVector<BydaoDebugInfo>* debugInfo = nullptr);
    
    // Загрузка
    static bool load(const QString& filename,
                     QVector<BydaoConstant>& constants,
                     QVector<QString>& stringTable,
                     QVector<BydaoInstruction>& code,
                     QVector<BydaoDebugInfo>* debugInfo = nullptr,
                     QString* error = nullptr);
    
    // Валидация
    static bool validate(const QVector<BydaoInstruction>& code,
                         const QVector<BydaoConstant>& constants,
                         const QVector<QString>& stringTable,
                         QString* error = nullptr);
};

#define SPECIAL_VAR     "__SelfData"

} // namespace BydaoScript

Q_DECLARE_METATYPE(BydaoScript::BydaoInstruction)
