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

#include "BydaoScript/BydaoMetaData.h"

namespace BydaoScript {

/**
 * Свойства переменной модуля/класса.
 */

VarMetaData::VarMetaData(){
    isConst = false;
    isStatic = false;
}

VarMetaData::VarMetaData( const VarMetaData& var ) {
    type = var.type;
    isConst = var.isConst;
    isStatic = var.isStatic;
}

VarMetaData::VarMetaData( const QString& type, bool isConst, bool isStatic ) {
    this->type = type;
    this->isConst = isConst;
    this->isStatic = isStatic;
}

/**
 * Свойства аргумента функции.
 *
 *  QString     name;
    QString     type;
    bool        isOut;
    QString     defVal;

 */
FuncArgMetaData::FuncArgMetaData(){
    isOut = false;
}

FuncArgMetaData::FuncArgMetaData( const FuncArgMetaData& funcArg ) {
    name = funcArg.name;
    types = funcArg.types;
    isOut = funcArg.isOut;
    defVal = funcArg.defVal;
}

FuncArgMetaData::FuncArgMetaData( const QString& name, const QString& type, bool isOut ) {
    this->name = name;
    this->types.append(type);
    this->isOut = isOut;
}

FuncArgMetaData::FuncArgMetaData( const QString& name, const QString& type, bool isOut, const QString& defVal ) {
    this->name = name;
    this->types.append( type );
    this->isOut = isOut;
    this->defVal = defVal;
}

FuncArgMetaData::FuncArgMetaData( const QString& name, const QStringList& typeList, bool isOut ) {
    this->name = name;
    this->types = typeList;
    this->isOut = isOut;
}

void    FuncArgMetaData::operator=( const FuncArgMetaData& funcArg ) {
    name = funcArg.name;
    types = funcArg.types;
    isOut = funcArg.isOut;
    defVal = funcArg.defVal;
}

/**
 * Свойства функции модуля/класса.
 *
 * FuncArgMetaDataList argList;
 * QString             retType;
 * bool                isStatic;
 * bool                isImmutable;
 */
FuncMetaData::FuncMetaData(){
    retType = "Void";
    isStatic = false;
    isImmutable = false;
    index = -1;
}

FuncMetaData::FuncMetaData( const FuncMetaData& func ){
    argList = func.argList;
    retType = func.retType;
    isStatic = func.isStatic;
    isImmutable = func.isImmutable;
    index = func.index;
}

FuncMetaData::FuncMetaData( const QString& retType, bool isStatic, bool isImmutable ) {
    this->retType = retType;
    this->isStatic = isStatic;
    this->isImmutable = isImmutable;
    this->index = -1;
}

FuncMetaData::FuncMetaData( int index, const QString& retType, bool isStatic, bool isImmutable ) {
    this->retType = retType;
    this->isStatic = isStatic;
    this->isImmutable = isImmutable;
    this->index = index;
}

FuncMetaData::FuncMetaData( const FuncArgMetaDataList& argList, const QString& retType,
                            bool isStatic, bool isImmutable ){
    this->argList = argList;
    this->retType = retType;
    this->isStatic = isStatic;
    this->isImmutable = isImmutable;
    this->index = -1;
}

FuncMetaData&   FuncMetaData::append( const FuncArgMetaData& arg ) {
    argList.append( arg );
    return *this;
}

FuncMetaData&   FuncMetaData::operator<<( const FuncArgMetaData& arg ){
    argList << arg;
    return *this;
}

int             FuncMetaData::argCount() const {
    return argList.count();
}

/**
 * Свойства операции.
 */
OperMetaData::OperMetaData() {
}

OperMetaData::OperMetaData( const QString& operandType, const QString& resultType ) {
    resultList[ operandType ] = resultType;
}

OperMetaData&   OperMetaData::append( const QString& operandType, const QString& resultType ){
    resultList[ operandType ] = resultType;
    return *this;
}

/**
 * Мета-данные.
 *
 * VarMetaDataDict     vars;
 * FuncMetaDataDict    funcs;
 * OperMetaDataDict    opers;
 */

MetaData::MetaData(){
    external = false;
}

MetaData::MetaData( const MetaData& data ){
    external = false;
    extend = data.extend;
    vars = data.vars;
    funcs = data.funcs;
    opers = data.opers;
}

MetaData::MetaData( MetaData* data ){
    if ( data != nullptr ) {
        external = data->external;
        extend = data->extend;
        vars = data->vars;
        funcs = data->funcs;
        opers = data->opers;
    }
}

MetaData&   MetaData::operator=( MetaData* data ) {
    if ( data != nullptr ) {
        external = data->external;
        extend = data->extend;
        vars = data->vars;
        funcs = data->funcs;
        opers = data->opers;
    }
    return *this;
}

MetaData&   MetaData::append( const QString& varName, const VarMetaData& var ){
    vars.insert( varName, var );
    return *this;
}

MetaData&   MetaData::append( const QString& funcName, const FuncMetaData& func ){
    funcs.insert( funcName, func );
    return *this;
}

MetaData&   MetaData::append( const QString& operName, const OperMetaData& oper ) {
    opers.insert( operName, oper );
    return *this;
}

bool        MetaData::hasVar( const QString& name ) const {
    return vars.contains( name );
}

const VarMetaData  MetaData::var( const QString& name ) const {
    return vars[ name ];
}

bool        MetaData::hasFunc( const QString& name ) const {
    return funcs.contains( name );
}

const FuncMetaData  MetaData::func( const QString& name ) const {
    return funcs[ name ];
}

bool        MetaData::hasOper( const QString& name ) const {
    return opers.contains( name );
}

const OperMetaData  MetaData::oper( const QString& name ) const {
    return opers[ name ];
}

}   // namespace
