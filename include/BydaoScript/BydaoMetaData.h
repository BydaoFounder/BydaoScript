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

#ifndef BYDAOMETADATA_H
#define BYDAOMETADATA_H

#pragma once

#include <QString>
#include <QList>
#include <QMap>

namespace BydaoScript {

/**
 * Свойства переменной модуля/класса.
 */
struct VarMetaData {

    /**
     * Тип переменной.
     */
    QString     type;

    /**
     * Флаг переменной, определенной оператором "const".
     */
    bool        isConst;

    /**
     * Флаг статической переменной.
     * В модуле все переменные статические.
     * Статическая переменная в классе принадлежит только объекту класса.
     * Не статическая переменная в классе - это переменная экземпляра класса.
     */
    bool        isStatic;

    VarMetaData();
    VarMetaData( const VarMetaData& var );
    VarMetaData( const QString& type, bool isConst, bool isStatic );

};
typedef QMap< QString, VarMetaData > VarMetaDataDict;

/**
 * Свойства аргумента функции.
 */
struct FuncArgMetaData {

    QString         name;
    QStringList     types;
    bool            isOut;
    QString         defVal;

    FuncArgMetaData();
    FuncArgMetaData( const FuncArgMetaData& funcArg );
    FuncArgMetaData( const QString& name, const QString& type, bool isOut = false );
    FuncArgMetaData( const QString& name, const QStringList& typeList, bool isOut = false );
    FuncArgMetaData( const QString& name, const QString& type, bool isOut, const QString& defVal );

    void    operator=( const FuncArgMetaData& funcArg );
};
typedef QList< FuncArgMetaData >    FuncArgMetaDataList;

/**
 * Свойства функции модуля/класса.
 */
struct FuncMetaData {

    /**
     * Список аргументов функции.
     */
    FuncArgMetaDataList argList;

    /**
     * Тип возвращаемого значения.
     */
    QString             retType;

    /**
     * Флаг статической функции.
     * В модуле все функции статические.
     * Статическая функция в классе (с признакком "static") применима к объекту класса.
     * Не статическая функция в классе применима к экземпляру класса.
     */
    bool                isStatic;

    /**
     * Флаг функции, которая не изменяет значение объекта, для которого она вызвана.
     */
    bool                isImmutable;

    FuncMetaData();
    FuncMetaData( const FuncMetaData& func );
    FuncMetaData( const QString& retType, bool isStatic, bool isImmutable );
    FuncMetaData( const FuncArgMetaDataList& argList, const QString& retType, bool isStatic, bool isImmutable );

    FuncMetaData&   append( const FuncArgMetaData& arg );

    FuncMetaData&   operator<<( const FuncArgMetaData& arg );

    int             argCount() const;
};
typedef QMap< QString, FuncMetaData > FuncMetaDataDict;

/**
 * Свойства операции.
 *
 * Для типа операнда может быть использован тип "Any", которые означает,
 * что операнд может быть любого типа, если он имеет функцию преобразования
 * в тип левого операнда.
 */
struct OperMetaData {

    /** Список результатов операции для разных типов операндов */
    QMap< QString, QString > resultList;

    OperMetaData();
    OperMetaData( const QString& operandType, const QString& resultType );

    OperMetaData&   append( const QString& operandType, const QString& resultType );

    bool    hasOperandType( const QString& operandType ) {
        return resultList.contains( operandType );
    }

    QString resultType( const QString& operandType ) {
        return resultList[ operandType ];
    }
};
typedef QMap< QString, OperMetaData > OperMetaDataDict;

/**
 * Мета-данные.
 */
struct MetaData {

    bool                external;   // флаг внешнего модуля, загружаемого оператором use
    QString             extend;     // название расширяемого типа (класса)
    VarMetaDataDict     vars;       // общедоступные (публичные) переменные типа
    FuncMetaDataDict    funcs;      // общедоступные (публичные) функции типа
    OperMetaDataDict    opers;      // операции, поддерживаемые типом

    MetaData();
    MetaData( const MetaData& data );
    MetaData( MetaData* data );

    MetaData&   operator=( MetaData* data );

    MetaData&   append( const QString& varName, const VarMetaData& var );
    MetaData&   append( const QString& funcName, const FuncMetaData& func );
    MetaData&   append( const QString& operName, const OperMetaData& oper );

    bool        hasVar( const QString& name ) const;
    const VarMetaData   var( const QString& name ) const;

    bool        hasFunc( const QString& name ) const;
    const FuncMetaData  func( const QString& name ) const;

    bool        hasOper( const QString& name ) const;
    const OperMetaData  oper( const QString& name ) const;
};

/**
 * Список используемых мета-данных.
 */
struct UsedMetaData {

    QString     type;
    MetaData*   metaData;

    UsedMetaData( const QString& type, MetaData* metaData ) {
        this->type = type;
        this->metaData = metaData;
    };
};

typedef QList< UsedMetaData >   UsedMetaDataList;

}   // namespace

#endif // BYDAOMETADATA_H
