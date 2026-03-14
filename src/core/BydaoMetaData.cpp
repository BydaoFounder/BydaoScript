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
    isPublic = false;
    isStatic = false;
}

VarMetaData::VarMetaData( const VarMetaData& var ) {
    type = var.type;
    isConst = var.isConst;
    isPublic = var.isPublic;
    isStatic = var.isStatic;
}

VarMetaData::VarMetaData( const QString& type, bool isConst, bool isPublic, bool isStatic ) {
    this->type = type;
    this->isConst = isConst;
    this->isPublic = isPublic;
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
    type = funcArg.type;
    isOut = funcArg.isOut;
    defVal = funcArg.defVal;
}

FuncArgMetaData::FuncArgMetaData( const QString& name, const QString& type, bool isOut ) {
    this->name = name;
    this->type = type;
    this->isOut = isOut;
}

FuncArgMetaData::FuncArgMetaData( const QString& name, const QString& type, bool isOut, const QString& defVal ) {
    this->name = name;
    this->type = type;
    this->isOut = isOut;
    this->defVal = defVal;
}

void    FuncArgMetaData::operator=( const FuncArgMetaData& funcArg ) {
    name = funcArg.name;
    type = funcArg.type;
    isOut = funcArg.isOut;
    defVal = funcArg.defVal;
}

/**
 * Свойства функции модуля/класса.
 *
 * FuncArgMetaDataList argList;
 * QString             retType;
 * bool                isPublic;
 * bool                isStatic;
 * bool                isImmutable;
 */
FuncMetaData::FuncMetaData(){
    retType = "Void";
    isPublic = false;
    isStatic = false;
    isImmutable = false;
}

FuncMetaData::FuncMetaData( const FuncMetaData& func ){
    argList = func.argList;
    retType = func.retType;
    isPublic = func.isPublic;
    isStatic = func.isStatic;
    isImmutable = func.isImmutable;
}

FuncMetaData::FuncMetaData( const QString& retType, bool isPublic, bool isStatic, bool isImmutable ) {
    this->retType = retType;
    this->isPublic = isPublic;
    this->isStatic = isStatic;
    this->isImmutable = isImmutable;
}

FuncMetaData::FuncMetaData( const FuncArgMetaDataList& argList, const QString& retType,
                            bool isPublic, bool isStatic, bool isImmutable ){
    this->argList = argList;
    this->retType = retType;
    this->isPublic = isPublic;
    this->isStatic = isStatic;
    this->isImmutable = isImmutable;
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
 * Мета-данные.
 *
 * VarMetaDataDict     vars;
 * FuncMetaDataDict    funcs;
 */
MetaData::MetaData(){

}

MetaData::MetaData( const MetaData& data ){
    vars = data.vars;
    funcs = data.funcs;
}

MetaData::MetaData( MetaData* data ){
    if ( data != nullptr ) {
        vars = data->vars;
        funcs = data->funcs;
    }
}

MetaData&   MetaData::append( const QString& varName, const VarMetaData& var ){
    vars.insert( varName, var );
    return *this;
}

MetaData&   MetaData::append( const QString& funcName, const FuncMetaData& func ){
    funcs.insert( funcName, func );
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

}   // namespace
