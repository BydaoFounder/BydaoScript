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
}

VarMetaData::VarMetaData( const VarMetaData& var ) {
    type = var.type;
    isConst = var.isConst;
}

VarMetaData::VarMetaData( const QString& type, bool isConst ) {
    this->type = type;
    this->isConst = isConst;
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
    funcs = funcArg.funcs;
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
    funcs = funcArg.funcs;
}

/**
 * Свойства функции модуля/класса.
 *
 * FuncArgMetaDataList argList;
 * QString             retType;
 * bool                isImmutable;
 */
FuncMetaData::FuncMetaData(){
    retType = "Void";
    isStatic = false;
    isPublic = false;
    isImmutable = false;
    index = -1;
}

FuncMetaData::FuncMetaData( const FuncMetaData& func ){
    argList = func.argList;
    retType = func.retType;
    isImmutable = func.isImmutable;
    isStatic = func.isStatic;
    isPublic = func.isPublic;
    index = func.index;
}

FuncMetaData::FuncMetaData( const QString& retType, bool isImmutable ) {
    this->retType = retType;
    this->isImmutable = isImmutable;
    this->isStatic = false;
    this->isPublic = false;
    this->index = -1;
}

FuncMetaData::FuncMetaData( int index, const QString& retType, bool isImmutable ) {
    this->retType = retType;
    this->isImmutable = isImmutable;
    this->isStatic = false;
    this->isPublic = false;
    this->index = index;
}

FuncMetaData::FuncMetaData( const FuncArgMetaDataList& argList, const QString& retType, bool isImmutable ){
    this->argList = argList;
    this->retType = retType;
    this->isImmutable = isImmutable;
    this->isStatic = false;
    this->isPublic = false;
    this->index = -1;
}

void    FuncMetaData::operator=( const FuncMetaData& func ){
    this->argList = func.argList;
    this->retType = func.retType;
    this->isImmutable = func.isImmutable;
    this->isStatic = func.isStatic;
    this->isPublic = func.isPublic;
    this->index = func.index;
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
    type.vars = data.type.vars;
    type.funcs = data.type.funcs;
    type.opers = data.type.opers;
    object.vars = data.object.vars;
    object.funcs = data.object.funcs;
    object.opers = data.object.opers;
}

MetaData::MetaData( MetaData* data ){
    if ( data != nullptr ) {
        external = data->external;
        extend = data->extend;
        type.vars = data->type.vars;
        type.funcs = data->type.funcs;
        type.opers = data->type.opers;
        object.vars = data->object.vars;
        object.funcs = data->object.funcs;
        object.opers = data->object.opers;
    }
}

MetaData&   MetaData::operator=( MetaData* data ) {
    if ( data != nullptr ) {
        external = data->external;
        extend = data->extend;
        type.vars = data->type.vars;
        type.funcs = data->type.funcs;
        type.opers = data->type.opers;
        object.vars = data->object.vars;
        object.funcs = data->object.funcs;
        object.opers = data->object.opers;
    }
    return *this;
}

/**
 * Добавить свойства из указанных метаданных.
 */
void        MetaData::append( const MetaData* md ) {
    for (auto it = md->type.vars.begin(); it != md->type.vars.end(); ++it) {
        type.vars.insert( it.key(), it.value() );
    }
    for (auto it = md->type.funcs.begin(); it != md->type.funcs.end(); ++it) {
        type.funcs.insert( it.key(), it.value() );
    }
    for (auto it = md->type.opers.begin(); it != md->type.opers.end(); ++it) {
        type.opers.insert( it.key(), it.value() );
    }
    for (auto it = md->object.vars.begin(); it != md->object.vars.end(); ++it) {
        object.vars.insert( it.key(), it.value() );
    }
    for (auto it = md->object.funcs.begin(); it != md->object.funcs.end(); ++it) {
        object.funcs.insert( it.key(), it.value() );
    }
    for (auto it = md->object.opers.begin(); it != md->object.opers.end(); ++it) {
        object.opers.insert( it.key(), it.value() );
    }
}

MetaData&   MetaData::appendType( const QString& varName, const VarMetaData& var ){
    type.vars.insert( varName, var );
    return *this;
}

MetaData&   MetaData::appendType( const QString& funcName, const FuncMetaData& func ){
    type.funcs.insert( funcName, func );
    return *this;
}

MetaData&   MetaData::appendType( const QString& operName, const OperMetaData& oper ) {
    type.opers.insert( operName, oper );
    return *this;
}

MetaData&   MetaData::appendObj( const QString& varName, const VarMetaData& var ){
    object.vars.insert( varName, var );
    return *this;
}

MetaData&   MetaData::appendObj( const QString& funcName, const FuncMetaData& func ){
    object.funcs.insert( funcName, func );
    return *this;
}

MetaData&   MetaData::appendObj( const QString& operName, const OperMetaData& oper ) {
    object.opers.insert( operName, oper );
    return *this;
}

bool        MetaData::hasTypeVar( const QString& name ) const {
    return type.vars.contains( name );
}

const VarMetaData  MetaData::typeVar( const QString& name ) const {
    return type.vars[ name ];
}

bool        MetaData::hasObjVar( const QString& name ) const {
    return object.vars.contains( name );
}

const VarMetaData  MetaData::objVar( const QString& name ) const {
    return object.vars[ name ];
}

bool        MetaData::hasTypeFunc( const QString& name ) const {
    return type.funcs.contains( name );
}

const FuncMetaData  MetaData::typeFunc( const QString& name ) const {
    return type.funcs[ name ];
}

bool        MetaData::hasObjFunc( const QString& name ) const {
    return object.funcs.contains( name );
}

const FuncMetaData  MetaData::objFunc( const QString& name ) const {
    return object.funcs[ name ];
}

bool        MetaData::hasTypeOper( const QString& name ) const {
    return type.opers.contains( name );
}

const OperMetaData  MetaData::typeOper( const QString& name ) const {
    return type.opers[ name ];
}

bool        MetaData::hasObjOper( const QString& name ) const {
    return object.opers.contains( name );
}

const OperMetaData  MetaData::objOper( const QString& name ) const {
    return object.opers[ name ];
}

}   // namespace
