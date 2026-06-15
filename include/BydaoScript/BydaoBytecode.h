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
#include <QHash>
// #include <QMetaType>

#include "BydaoValue.h"

namespace BydaoScript {

// Инструкции байткода (1 байт)
enum BydaoOpCode : quint8 {
    // Управление
    Nop = 0, Ignore,

    // Переходы
    Jump,           // arg1 = смещение (индекс инструкции)
    JumpFalse,      // arg1 = смещение
    JumpTrue,       // arg1 = смещение
    JumpFalsePeek,  // arg1 = смещение
    JumpTruePeek,   // arg1 = смещение

    // Переменные и области видимости
    ConstDecl,      // определение константы: arg1 - индекс имени, arg2 - индекс константы
    VarDecl,        // arg1 = индекс в таблице строк (имя переменной)
    Drop,           // arg1 = индекс переменной
    Load,           // arg1 = индекс переменной
    Store,          // arg1 = индекс переменной
    AddStore,       // arg1 = индекс переменной, arg2 > 0 - индекс второй переменной
    SubStore,       // arg1 = индекс переменной
    MulStore,       // arg1 = индекс переменной
    DivStore,       // arg1 = индекс переменной
    ModStore,       // arg1 = индекс переменной

    // Константы
    PushConst,      // arg1 = индекс в таблице констант

    // Арифметика
    Add, Sub, Neg,
    Mul, Div, Mod,

    VarAdd,         // сложение переменной:
    // arg1 > 0 - индекс левой переменной
    // arg2 > 0 - индекс правой переменной
    // arg2 < 0 - индекс константы
    // arg2 = 0 - сложение переменной со значением на стеке

    VarSub,        // вычитание переменной:
    // arg1 > 0 - индекс левой переменной
    // arg2 > 0 - индекс правой переменной
    // arg2 < 0 - индекс константы
    // arg2 = 0 - сложение переменной со значением на стеке

    VarDiv,         // деление переменной:
    // arg1 > 0 - индекс левой переменной
    // arg2 > 0 - индекс правой переменной
    // arg2 < 0 - индекс константы
    // arg2 = 0 - сложение переменной со значением на стеке

    // Сравнение
    Eq, Neq, Lt, Gt, Le, Ge,
    IsType, NotType,

    // Сравнение переменных
    VarEq,
    VarNeq,
    VarLt,          // arg1 > 0 - индекс левой переменной, arg2 > 0 - индекс правой переменной
                    // arg1 > 0 - индекс левой переменной, arg2 < 0 - индекс константы
                    // arg1 > 0 - индекс левой переменной, arg2 = 0 - сравнение со значением на стеке
    VarLe,          // arg1 > 0 - индекс левой переменной, arg2 > 0 - индекс правой переменной
                    // arg1 > 0 - индекс левой переменной, arg2 < 0 - индекс константы
                    // arg1 > 0 - индекс левой переменной, arg2 = 0 - сравнение со значением на стеке

    // Логические
    And, Or, Not,

    // Битовые
    BitAnd, BitXor, BitOr,

    // Итераторы
    GetIter,     // obj.iter() - получение итератора, arg1 - индекс переменной для итератора
    ItNext,      // вызов iter.next() -> bool, arg1 - индекс переменной итератора
    ItValue,     // iter.value -> значение, arg1 - индекс переменной итератора, arg2 - индекс переменной значения
    ItKey,       // НЕ РЕАЛИЗОВАНО ! iter.key -> ключ, arg1 - индекс переменной итератора, arg2 - индекс переменной ключа

    // Доступ к членам
    Member,         // arg1 = индекс имени свойства, arg2 > 0 - индекс переменной
    SetMember,      // сохранить значение со стека в переменной объекта, arg1 = индекс имени свойства
    Method,         // arg1 = индекс имени метода (подготовка к вызову)
    Index,          // arg1 = не используется (индекс на стеке)
    Call,           // arg1 = количество аргументов
    CallVoid,       // arg1 = количество аргументов

    // Создание объектов

    NewObj,         // arg1 - количество аргументов
                    // arg2 = -1 - вызов метода "new" по имени
                    // arg2 >= 0 - вызов метода по индексу

    // Области видимости
    ScopeDrop,      // сброс до заданного размера, arg1 - размер списка переменных

    // Модули и встроенные типы
    UseModule,      // arg1 = индекс имени модуля в таблице строк
    UseBuiltin,     // arg1 = индекс имени типа в таблице строк

    // Массивы
    PushArray,      // arg1 = количество элементов

    // Функции (не методы объекта)
    FuncDecl,       // определение: arg1 - индекс названия, arg2 - индекс объекта функции
    CallFunc,       // вызов функции: arg1 = кол-во аргументов, arg2 - индекс переменной типа фнукция
    CallFuncVoid,   // вызов void-функции: arg1 = кол-во аргументов, arg2 - индекс переменной типа фнукция
    Return,         // возврат: arg1 = 1 если есть значение, 0 если void
    LoadScope,      // загрузка scope-переменной: arg1 = индекс
    StoreScope,     // сохранение в scope-переменную (для out?)
    PushAddr,       // адрес переменной (для out аргументов)

    // Управление стеком значений
    StkPop,          // сброс вершины стека

    // Сравнение с null

    EqNull,         // сравнение на равенство с null
                    // arg1 == 0 - значение на стеке
                    // arg1 > 0 - индекс переменной
    NeqNull,        // сравнение на неравенство с null
                    // arg1 == 0 - значение на стеке
                    // arg1 > 0 - индекс переменной

    // Стоп
    Halt
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
        qint64 intValue;        // 8 байт
        double realValue;       // 8 байт
        quint32 stringIndex;    // 4 байта (индекс в таблице строк)
        quint8 boolValue;       // 1 байт
        void*  pointer;         // 8 байт - указатель на объект
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


// Информация о аргументе функции для сериализации
struct FunctionArgInfo {
    QString name;
    QStringList types;
    bool isOut;
    bool hasDefault;
    BydaoValue defaultValue;

    FunctionArgInfo() : isOut(false), hasDefault(false) {}
};

// Информация о функции для сериализации
struct FunctionInfo {
    QString name;
    int entryPc = 0;
    int arity = 0;
    bool isPublic = false;
    bool isImmutable = false;
    bool isStatic = true;
    int scopeOffset = 0;
    QString retType;

    QVector<FunctionArgInfo> args;
    QVector<BydaoInstruction> code;
    QVector<BydaoConstant> constants;      // константы, используемые только этой функцией
    QVector<QString> stringTable;          // строки, используемые только этой функцией
    QHash<QString, int> selfRefs;          // имя переменной модуля -> индекс в глобальном фрейме

    // Локальные переменные функции
    struct LocalVar {
        QString name;
        QString type;
    };
    QVector<LocalVar> localVars;
};

// Информация о модуле для сериализации
struct ModuleInfo {
    QString name;
    QVector<FunctionInfo> functions;
    QVector<QString> globalStringTable;
    QVector<BydaoConstant> globalConstants;
    QVector<BydaoInstruction> globalCode;

    // Публичные переменные модуля
    struct PubVar {
        QString name;
        QString type;
        bool isConst;
        BydaoValue initValue;
    };
    QVector<PubVar> pubVars;
};

} // namespace BydaoScript

// Q_DECLARE_METATYPE(BydaoScript::BydaoInstruction)
