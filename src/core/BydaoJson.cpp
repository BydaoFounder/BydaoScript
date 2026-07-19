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
#include "BydaoScript/BydaoJson.h"
#include "BydaoScript/BydaoArray.h"
#include "BydaoScript/BydaoLexer.h"

namespace BydaoScript {

// Получить мета-данные
MetaData*   BydaoJson::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData->name = "Array";
        metaData
            // Константы типа
            ->appendType( "COMPACT",    VarMetaData("Int", VMD_CONST) )
            .appendType( "PRETTY",    VarMetaData("Int", VMD_CONST) )
            ;
        metaData
            // Функции типа
            ->appendType( "toString",   FuncMetaData("String",FMD_IMMUTABLE) << FuncArgMetaData("val","Any",ARG_IN) << FuncArgMetaData("style","Int",ARG_IN,"null") )
            .appendType( "fromString",  FuncMetaData("Any",FMD_IMMUTABLE) << FuncArgMetaData("str","String",ARG_IN) )
            .appendType( "error",       FuncMetaData("String",FMD_IMMUTABLE) )
            ;
    }
    return metaData;
}

/**
 * Вернуть список используемых типов.
 */
UsedMetaDataList    BydaoJson::usedMetaData() {
    static UsedMetaDataList list;
    return list;
}

BydaoJson::BydaoJson()
    : BydaoObject()
{
    // Методы типа
    registerMethod("toString",      &BydaoJson::method_toString);
    registerMethod("fromString",    &BydaoJson::method_fromString);
    registerMethod("error",         &BydaoJson::method_error);

    // Свойства типа
    registerVar( "COMPACT",         &BydaoJson::getvar_COMPACT );
    registerVar( "PRETTY",          &BydaoJson::getvar_PRETTY );

    // Регистрация функций для вызова по индексу

    // m_stdMethodTable.resize(3);
    // m_stdMethodTable[0] = &BydaoJson::sortImpl;
    // m_stdMethodTable[1] = &BydaoJson::ksortImpl;
    // m_stdMethodTable[2] = &BydaoJson::sizeImpl;
}

void BydaoJson::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

void BydaoJson::registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter ) {
    m_vars[ name ] = { getter, setter };
}

bool    BydaoJson::getVar( const QString& varName, BydaoValue& value ) {
    auto it = m_vars.find( varName );
    if ( it == m_vars.end() ) {
        return BydaoObject::getVar( varName, value );
    }
    GetVarPtr getter = it.value().getter;
    return ( this->*( getter) )( value );
}

bool BydaoJson::callMethod(const QString& name,
                            const QVector<BydaoValue>& args,
                            BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    m_runtime->logError( QString("Array does not have method '%1'").arg( name ) );
    return false;
}


QString      BydaoJson::toString( const BydaoValue& val, JsonStyle style ) {

    if ( val.isNull() ) {
        return QString("null");
    }
    if ( val.isArray() ) {
        return ((BydaoArray*) val.toObject() )->toJson( style, 0 );
    }
    if ( val.isString() ) {
        QString str = val.toString();
        if ( str.contains("\"") && ! str.contains("\\\"") ) {
            str.replace( "\"", "\\\"" );
        }
        return QString( "\"%1\"" ).arg( str );
    }
    return val.toString();
}

BydaoValue   BydaoJson::fromString( const QString& str ) {

    if ( str.isEmpty() ) {
        m_runtime->logError( "JSON syntax: empty string" );
        return BydaoValue();
    }

    BydaoLexer lexer( str );
    ParseContext ctx;
    ctx.lexer  = &lexer;
    ctx.tokens = lexer.tokenize();
    ctx.pos    = 0;
    if ( ! lexer.errorMessage().isEmpty() ) {
        m_runtime->logError( "JSON syntax: " + lexer.errorMessage() );
        return BydaoValue();
    }

    BydaoValue value = parseValue( &ctx );
    if ( value.isObject() ) {
        if ( ++ctx.pos < ctx.tokens.size() ) {
            BydaoToken token = ctx.tokens[ ctx.pos ];
            if ( token.type != BydaoTokenType::EndOfFile ) {
                m_runtime->logError( QString( "Invalid syntax at %1,%2" ).arg( token.line ).arg( token.column ) );
                value = BydaoValue();
            }
        }
    }
    return value;
}

/**
 * Разбор текущего токена как значения.
 *
 * Позиция разбора должна указывать на начало значения.
 *
 * @param ctx
 * @return
 */
BydaoValue   BydaoJson::parseValue( ParseContext* ctx ) {
    BydaoValue value;

    BydaoToken token = ctx->tokens[ ctx->pos ];
    BydaoTokenType tokenType = token.type;
    if ( tokenType == BydaoTokenType::LBracket ) {      // разбор массива [...]

        value = BydaoJson::parseArray( ctx );
    }
    else if ( tokenType == BydaoTokenType::LBrace) {    // разбор объекта {...}

        value = BydaoJson::parseObject( ctx );
    }
    else {                                              // разбор простого значения

        QString text = token.text;

        if ( tokenType == BydaoTokenType::Number ) {

            if ( text.contains('.') || text.contains('e') || text.contains('E') ) {
                value = BydaoValue::fromReal( text.toDouble() );
            }
            else if ( text.contains('x') || text.contains('X')) {
                value = BydaoValue::fromInt( text.toLongLong(nullptr,16) );
            }
            else if ( text.contains('b') || text.contains('B')) {
                value = BydaoValue::fromInt( text.toLongLong(nullptr,2) );
            }
            else {
                value = BydaoValue::fromInt( text.toLongLong() );
            }
        }
        else if ( tokenType == BydaoTokenType::String ) {
            value = BydaoValue::fromString( text );
        }
        else if ( tokenType == BydaoTokenType::False ) {
            value = BydaoValue::fromBool( false );
        }
        else if ( tokenType == BydaoTokenType::True ) {
            value = BydaoValue::fromBool( true );
        }
        else if ( tokenType == BydaoTokenType::Null ) {
            value = BydaoValue::fromNull();
        }
        else {
            m_runtime->logError( QString( "Invalid value at %1,%2" ).arg( token.line ).arg( token.column ) );
        }
    }

    return value;
}

/**
 * Разбор массива значений.
 *
 * Левая скобка '[' уже разобрана. Разбор завершается, когда метод
 * получает токен ']'.
 *
 * @param ctx
 * @return
 */
BydaoValue   BydaoJson::parseArray( ParseContext* ctx ) {

    BydaoArray* array = new BydaoArray();
    array->setRuntime( m_runtime );

    while ( ++ctx->pos < ctx->tokens.size() ) {

        BydaoToken token = ctx->tokens[ ctx->pos ];
        BydaoTokenType tokenType = token.type;

        if ( tokenType != BydaoTokenType::RBracket ) {

            BydaoValue value = parseValue( ctx );
            if ( ! value.isObject() ) { // ошибка разбора
                delete array;
                array = nullptr;
                return BydaoValue();
            }
            array->append( value );

            token = ctx->tokens[ ++ctx->pos ];
            tokenType = token.type;
        }

        if ( tokenType == BydaoTokenType::RBracket ) {
            break;
        }

        if ( tokenType == BydaoTokenType::Comma ) {
            if ( array->size() == 0 ) {
                delete array;
                array = nullptr;
                m_runtime->logError( QString( "Missed value at %1,%2" ).arg( token.line ).arg( token.column ) );
            }
            token = ctx->tokens[ ctx->pos + 1 ];
            if ( token.type == BydaoTokenType::RBracket ) {
                ++ctx->pos;
                break;
            }
        }
    }
    return BydaoValue( array, TYPE_ARRAY );
}

/**
 * Разбор массива значений.
 *
 * Левая скобка '{' уже разобрана. Разбор завершается, когда метод
 * получает токен '}'.
 *
 * @param ctx
 * @return
 */
BydaoValue   BydaoJson::parseObject( ParseContext* ctx ) {

    BydaoArray* array = new BydaoArray();
    array->setRuntime( m_runtime );

    while ( ++ctx->pos < ctx->tokens.size() ) {

        BydaoToken token = ctx->tokens[ ctx->pos ];
        BydaoTokenType tokenType = token.type;

        if ( tokenType != BydaoTokenType::RBrace ) {

            // Должен быть ключ в формате строки

            if ( tokenType != BydaoTokenType::String ) {
                delete array;
                m_runtime->logError( QString( "Invalid format of the key at %1,%2" ).arg( token.line ).arg( token.column ) );
                return BydaoValue();
            }
            BydaoValue key = BydaoValue::fromString( token.text );

            // Проверяем ':'

            token = ctx->tokens[ ++ctx->pos ];
            tokenType = token.type;
            if ( tokenType != BydaoTokenType::Colon ) {
                delete array;
                m_runtime->logError( QString( "Missed ':' at %1,%2" ).arg( token.line ).arg( token.column ) );
                return BydaoValue();
            }

            // Пропустим ':'

            ++ctx->pos;

            // Разбор значения для ключа

            BydaoValue value = parseValue( ctx );
            if ( ! value.isObject() ) { // ошибка разбора
                delete array;
                return BydaoValue();
            }
            array->set( key, value );

            token = ctx->tokens[ ++ctx->pos ];
            tokenType = token.type;
        }

        if ( tokenType == BydaoTokenType::RBrace ) {
            break;
        }

        if ( tokenType == BydaoTokenType::Comma ) {
            token = ctx->tokens[ ctx->pos + 1 ];
            if ( token.type == BydaoTokenType::RBrace ) {
                ++ctx->pos;
                break;
            }
        }
    }
    return BydaoValue( array, TYPE_ARRAY );
}

bool BydaoJson::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    if ( args.size() != 2 ) return false;
    JsonStyle style = COMPACT;
    if ( args[1].isInt() ) {
        if ( args[1].toInt() == PRETTY ) {
            style = PRETTY;
        }
    }
    result = BydaoValue::fromString( BydaoJson::toString( args[0], style ) );
    return true;
}

bool BydaoJson::method_fromString(const QVector<BydaoValue>& args, BydaoValue& result) {
    if ( args.size() != 1 ) return false;
    if ( ! args[0].isString() ) {
        m_runtime->logError( QString("Invalid agrument type for method 'fromString'") );
        result = BydaoValue();
    }
    else {
        result = fromString( args[0].toString() );
    }
    return true;
}

bool BydaoJson::method_error(const QVector<BydaoValue>& args, BydaoValue& result) {
    if ( args.size() != 0 ) return false;
    result = BydaoValue::fromString( m_runtime->lastError() );
    return true;
}

} // namespace BydaoScript