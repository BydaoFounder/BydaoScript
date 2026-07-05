#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QThread>
#include <QProcessEnvironment>

#include "BydaoWeb.h"
#include "../include/BydaoScript/BydaoString.h"
#include "../include/BydaoScript/BydaoInt.h"
#include "../include/BydaoScript/BydaoBool.h"
#include "../include/BydaoScript/BydaoNull.h"
#include "../include/BydaoScript/BydaoDict.h"
#include "../include/BydaoScript/BydaoRuntime.h"

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace BydaoScript {
namespace Modules {

// ========== Экспорт модуля ==========

extern "C" {
    MODULE_EXPORT BydaoScript::BydaoModule* createModule() {
        return new BydaoWebModule();
    }

    MODULE_EXPORT void destroyModule(BydaoScript::BydaoModule* module) {
        delete module;
    }
}

// ========== BydaoWebModule ==========

MetaData*   BydaoWebModule::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData->external = true;
        metaData->name = "Web";
        metaData
            // переменные модуля (типа данных)
            ->appendType( "server",     VarMetaData("Dict",VMD_CONST) )
//            .appendType( "request",     VarMetaData("WebRequest",VMD_CONST) )
            .appendType( "response",    VarMetaData("WebResponse",VMD_CONST) )
            ;
        metaData
            ->appendType( "server",     FuncMetaData("Dict",   FMD_IMMUTABLE) )
            ;
    }
    return metaData;
}

/**
 * Вернуть список используемых типов.
 */
UsedMetaDataList    BydaoWebModule::usedMetaData() {
    static UsedMetaDataList list;

    if ( list.isEmpty() ) {
        // list << UsedMetaData( "WebRequest",  BydaoWebRequest::metaData() );
        list << UsedMetaData( "WebResponse", WebResponse::metaData() );
    }

    return list;
}

BydaoWebModule::BydaoWebModule()
    : BydaoModule()
{
    m_request = nullptr;      // объект запроса с веб-сервера
    m_response = nullptr;     // объект ответа веб-серверу

    // Методы модуля
    registerMethod("server",    &BydaoWebModule::method_server);

    // Переменные модуля
    registerVar( "server",      &BydaoWebModule::getvar_server );
    registerVar( "request",     &BydaoWebModule::getvar_request );
    registerVar( "response",    &BydaoWebModule::getvar_response );
}

BydaoWebModule::~BydaoWebModule() {
/*
    if ( m_request ) {
        delete m_request;
        m_request = nullptr;
    }
*/
}

bool BydaoWebModule::initialize() {
    m_request = nullptr;      // объект запроса с веб-сервера
    m_response = nullptr;     // объект ответа веб-серверу
    return true;
}

bool BydaoWebModule::shutdown() {
    if ( m_response ) {
        delete m_response;
        m_response = nullptr;
    }
    return true;
}

void BydaoWebModule::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoWebModule::callMethod(const QString& name,
                                 const QVector<BydaoValue>& args,
                                 BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

void BydaoWebModule::registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter ) {
    m_vars[ name ] = { getter, setter };
}

bool    BydaoWebModule::getVar( const QString& varName, BydaoValue& value ) {
    auto it = m_vars.find( varName );
    if ( it == m_vars.end() ) {
        return BydaoObject::getVar( varName, value );
    }
    GetVarPtr getter = it.value().getter;
    return ( this->*( getter) )( value );
}

bool    BydaoWebModule::getServerParams( BydaoValue& result ) {
    BydaoDict* dict = new BydaoDict();
    if ( m_runtime ) {

        BydaoRuntime::Environment* env = m_runtime->getEnvironment();
        if ( env ) {

            for ( auto it = env->begin(); it != env->end(); ++it ) {
                dict->set( BydaoValue::fromString( it.key() ), BydaoValue::fromString( it.value() ) );
                // QString key = it.key();
                // if ( ! key.startsWith("HTTP_")
                //     && key != QStringLiteral("CONTENT_TYPE")
                //     && key != QStringLiteral("CONTENT_LENGTH") ) {
                //     dict->set( BydaoValue::fromString( key ), BydaoValue::fromString( it.value() ) );
                // }
            }
        }
    }
    result = BydaoValue( dict, BydaoTypeId::TYPE_DICT );
    return true;
}

// ========== Методы модуля (С КВАЛИФИКАТОРОМ КЛАССА) ==========

bool BydaoWebModule::method_server(const QVector<BydaoValue>&, BydaoValue& result) {
    return getServerParams( result );
}

//==============================================================================


// Получить мета-данные
MetaData*   WebResponse::metaData() {
    static MetaData* metaData = nullptr;
    if ( ! metaData ) {
        metaData = new MetaData();
        metaData->name = "WebResponse";
        metaData
            // методы объекта
            ->appendObj( "out",     FuncMetaData(0,"Void", FMD_ALTERABLE) << FuncArgMetaData("str","String",ARG_IN) )
            .appendObj( "status",   FuncMetaData(1,"Void", FMD_ALTERABLE) << FuncArgMetaData("code","Int",ARG_IN) << FuncArgMetaData("str","String",ARG_IN) )
            .appendObj( "header",   FuncMetaData(2,"Void", FMD_ALTERABLE) << FuncArgMetaData("name","String",ARG_IN) << FuncArgMetaData("value","String",ARG_IN) )
            ;
    }
    return metaData;
}

/**
 * Вернуть список используемых типов.
 */
UsedMetaDataList    WebResponse::usedMetaData() {
    static UsedMetaDataList list;

    if ( list.isEmpty() ) {
    }

    return list;
}

WebResponse::WebResponse()
    : BydaoObject()
{
    m_statusCode = 200;
    m_statusSet = false;
    m_headersSent = false;

    // Методы объекта
    registerMethod("out",       &WebResponse::method_out);
    registerMethod("status",    &WebResponse::method_status);
    registerMethod("header",    &WebResponse::method_header);

    // Регистрация функций для вызова по индексу

    m_stdMethodTable.resize(3);
    m_stdMethodTable[0] = &WebResponse::outImpl;
    m_stdMethodTable[1] = &WebResponse::statusImpl;
    m_stdMethodTable[2] = &WebResponse::headerImpl;
}

WebResponse::~WebResponse() {
}

void WebResponse::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

void WebResponse::registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter ) {
    m_vars[ name ] = { getter, setter };
}

bool    WebResponse::getVar( const QString& varName, BydaoValue& value ) {
    auto it = m_vars.find( varName );
    if ( it == m_vars.end() ) {
        return BydaoObject::getVar( varName, value );
    }
    GetVarPtr getter = it.value().getter;
    return ( this->*( getter) )( value );
}

bool WebResponse::callMethod(const QString& name,
                            const QVector<BydaoValue>& args,
                            BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

bool WebResponse::method_status(const QVector<BydaoValue>& args, BydaoValue&) {
    if (args.size() != 2) return false;
    if ( m_headersSent ) {
        // TODO: Ошибка - заголовки уже отправлены
    }
    m_statusCode = args[0].toInt();
    m_statusText = args[1].toString();
    m_statusSet = true;
    return true;
}

bool WebResponse::method_header(const QVector<BydaoValue>& args, BydaoValue&) {
    if (args.size() != 2) return false;
    if ( m_headersSent ) {
        // TODO: Ошибка - заголовки уже отправлены
    }
    m_headers[ args[0].toString() ] = args[1].toString();
    return true;
}

bool WebResponse::method_out(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED( result );
    if (args.size() != 1) return false;
    if ( m_runtime ) {
        QTextStream& out = *m_runtime->outStream();

        if ( ! m_headersSent ) {

            // Выводим статус (если установлен)
            if (m_statusSet) {
                out << QString("Status: %1 %2\r\n").arg(m_statusCode).arg(m_statusText);
            }
            else {
                out << "Status: 200 OK\r\n";
            }

            if ( m_headers.isEmpty() ) {
                // Заголовки по умолчанию
                out << "Content-Type: text/html; charset=utf-8\r\n";
            }
            else {
                // Выводим все установленные заголовки
                for (auto it = m_headers.begin(); it != m_headers.end(); ++it) {
                    out << QString("%1: %2\r\n").arg( it.key(), it.value() );
                }
            }
            out << "\r\n";  // разделитель заголовков и тела
            m_headersSent = true;
        }

        out << args[0].toString();
        out.flush();
    }
    return true;
}


} // namespace Modules
} // namespace BydaoScript
