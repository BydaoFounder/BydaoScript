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

    VarMetaData();
    VarMetaData( const VarMetaData& var );
    VarMetaData( const QString& type, bool isConst );

};
typedef QMap< QString, VarMetaData > VarMetaDataDict;

#define VMD_VARIABLE    false
#define VMD_CONST       true

/**
 * Свойства аргумента функции.
 */
struct FuncMetaData;
struct FuncArgMetaData {

    QString             name;
    QStringList         types;
    QList<FuncMetaData> funcs;
    bool                isOut;
    QString             defVal;

    FuncArgMetaData();
    FuncArgMetaData( const FuncArgMetaData& funcArg );
    FuncArgMetaData( const QString& name, const QString& type, bool isOut = false );
    FuncArgMetaData( const QString& name, const QStringList& typeList, bool isOut = false );
    FuncArgMetaData( const QString& name, const QString& type, bool isOut, const QString& defVal );

    void    operator=( const FuncArgMetaData& funcArg );
};
typedef QList< FuncArgMetaData >    FuncArgMetaDataList;

#define ARG_IN      false
#define ARG_OUT     true

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
     * Флаг функции, которая не изменяет значение объекта, для которого она вызвана.
     */
    bool                isImmutable;

    bool                isStatic;

    bool                isPublic;

    /**
     * Индекс функции для прямого вызова через таблицу функций.
     */
    int                 index;

    FuncMetaData();
    FuncMetaData( const FuncMetaData& func );
    FuncMetaData( const QString& retType, bool isImmutable );
    FuncMetaData( int index, const QString& retType, bool isImmutable );
    FuncMetaData( const FuncArgMetaDataList& argList, const QString& retType, bool isImmutable );

    FuncMetaData&   append( const FuncArgMetaData& arg );

    FuncMetaData&   operator<<( const FuncArgMetaData& arg );

    void            operator=( const FuncMetaData& func );

    int             argCount() const;
};
typedef QMap< QString, FuncMetaData > FuncMetaDataDict;

#define FMD_IMMUTABLE      true
#define FMD_ALTERABLE      false

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

struct MetaInfo {
    VarMetaDataDict     vars;       // общедоступные (публичные) переменные типа
    FuncMetaDataDict    funcs;      // общедоступные (публичные) функции типа
    OperMetaDataDict    opers;      // операции, поддерживаемые типом
};

/**
 * Мета-данные.
 */
struct MetaData {

    bool                external;   // флаг внешнего модуля, загружаемого оператором use
    QString             name;       // название типа/класса метаданных
    QString             extend;     // название расширяемого типа/класса
    MetaInfo            type;       // мета-информация типа (класса, модуля)
    MetaInfo            object;     // мета-информация объекта

    MetaData();
    MetaData( const MetaData& data );
    MetaData( MetaData* data );

    MetaData&   operator=( MetaData* data );

    /**
     * Добавить свойства из указанных метаданных.
     */
    void        append( const MetaData* md );

    MetaData&   appendType( const QString& varName, const VarMetaData& var );
    MetaData&   appendType( const QString& funcName, const FuncMetaData& func );
    MetaData&   appendType( const QString& operName, const OperMetaData& oper );

    MetaData&   appendObj( const QString& varName, const VarMetaData& var );
    MetaData&   appendObj( const QString& funcName, const FuncMetaData& func );
    MetaData&   appendObj( const QString& operName, const OperMetaData& oper );

    bool        hasTypeVar( const QString& name ) const;
    const VarMetaData   typeVar( const QString& name ) const;

    bool        hasTypeFunc( const QString& name ) const;
    const FuncMetaData  typeFunc( const QString& name ) const;

    bool        hasTypeOper( const QString& name ) const;
    const OperMetaData  typeOper( const QString& name ) const;

    bool        hasObjVar( const QString& name ) const;
    const VarMetaData   objVar( const QString& name ) const;

    bool        hasObjFunc( const QString& name ) const;
    const FuncMetaData  objFunc( const QString& name ) const;

    bool        hasObjOper( const QString& name ) const;
    const OperMetaData  objOper( const QString& name ) const;
};

/**
 * Список используемых мета-данных.
 */
struct UsedMetaData {

    QString     type;       //!< название типа
    MetaData*   metaData;   //!< объект мета-данных
    bool        append;     //!< флаг добавления мета-данных к ведущему классу

    UsedMetaData( const QString& type, MetaData* metaData, bool append = false ) {
        this->type = type;
        this->metaData = metaData;
        this->append = append;
    };
};

typedef QList< UsedMetaData >   UsedMetaDataList;

}   // namespace

#endif // BYDAOMETADATA_H
