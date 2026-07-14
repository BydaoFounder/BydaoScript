#pragma once

#include "../../../include/BydaoScript/BydaoModule.h"
#include "../../../include/BydaoScript/BydaoMetaData.h"

#ifdef BYDAOWEB_LIBRARY
#include "BydaoWeb_global.h"
#else
#define BYDAOWEB_EXPORT
#endif

namespace BydaoScript {
namespace Modules {


/**
 * Класс запроса от веб-сервера.
 *
 * Методы объекта:
 *
 * headers(): Dict
 * header( name: String ): String
 */

class BYDAOWEB_EXPORT WebRequest: public BydaoObject {

public:
    explicit WebRequest();
    virtual ~WebRequest() override;

    // Получить мета-данные
    static MetaData*   metaData();

    // Получить список используемых мета-данных
    static UsedMetaDataList usedMetaData();

    QString typeName() const override { return "WebRequest"; }

    bool    getVar( const QString& varName, BydaoValue& value ) override;

    bool    callMethod(const QString& name, const QVector<BydaoValue>& args, BydaoValue& result) override;

    void            setRuntime(BydaoRuntime* runtime) {
        m_runtime = runtime;
    };
    BydaoRuntime*   runtime() { return m_runtime; };

private:

    void    parseHeaders();

    bool    method_headers(const QVector<BydaoValue>& args, BydaoValue& result);
    bool    method_header(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (WebRequest::*)(const QVector<BydaoValue>&, BydaoValue&);
    void    registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;

    using GetVarPtr = bool (WebRequest::*)(BydaoValue&);
    using SetVarPtr = bool (WebRequest::*)(const BydaoValue&);
    struct VarMethod {
        GetVarPtr   getter;
        SetVarPtr   setter;
    };
    QHash<QString,VarMethod> m_vars;

    void    registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter = nullptr );

    static bool headersImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        return ( static_cast<WebRequest*>(self) )->method_headers( args, result );
    }

    static bool headerImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        return ( static_cast<WebRequest*>(self) )->method_header( args, result );
    }

    BydaoRuntime*   m_runtime;

    QHash< QString, QString > m_headers;
};

/**
 * Класс ответа веб-серверу.
 *
 * Методы объекта:
 *
 * out( data: String )
 * status( val )
 * header( name: String, value: String )
 * headers( list: Dict )
 * redirect( url: String )
 * json( data: Array | Dict )
 */

class BYDAOWEB_EXPORT WebResponse: public BydaoObject {

public:
    explicit WebResponse();
    virtual ~WebResponse() override;

    // Получить мета-данные
    static MetaData*   metaData();

    // Получить список используемых мета-данных
    static UsedMetaDataList usedMetaData();

    QString typeName() const override { return "WebResponse"; }

    bool    getVar( const QString& varName, BydaoValue& value ) override;

    bool    callMethod(const QString& name, const QVector<BydaoValue>& args, BydaoValue& result) override;

    void            setRuntime(BydaoRuntime* runtime) {
        m_runtime = runtime;
    };
    BydaoRuntime*   runtime() { return m_runtime; };

private:

    bool method_out(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_status(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_header(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (WebResponse::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;

    using GetVarPtr = bool (WebResponse::*)(BydaoValue&);
    using SetVarPtr = bool (WebResponse::*)(const BydaoValue&);
    struct VarMethod {
        GetVarPtr   getter;
        SetVarPtr   setter;
    };
    QHash<QString,VarMethod> m_vars;

    void registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter = nullptr );

    static bool outImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        return ( static_cast<WebResponse*>(self) )->method_out( args, result );
    }

    static bool statusImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        return ( static_cast<WebResponse*>(self) )->method_status( args, result );
    }

    static bool headerImpl( BydaoObject* self, const QVector<BydaoValue>& args, BydaoValue& result ) {
        return ( static_cast<WebResponse*>(self) )->method_header( args, result );
    }

    BydaoRuntime*   m_runtime;

    int             m_statusCode;
    QString         m_statusText;
    bool            m_statusSet;

    bool            m_headersSent;
    QHash< QString, QString > m_headers;

};

//==============================================================================

class BYDAOWEB_EXPORT BydaoWebModule : public BydaoModule {

public:
    explicit BydaoWebModule();
    ~BydaoWebModule();

    // Обязательные методы от BydaoObject
    QString typeName() const override { return "WebModule"; }

    // Обязательные методы от BydaoModule
    QString name() const override { return "Web"; }
    QString version() const override { return "1.0.0"; }

    // Метаданные модуля
    MetaData*   metaData() override;

    // Получить список используемых мета-данных
    UsedMetaDataList usedMetaData() override;

    bool    callMethod(const QString& name, const QVector<BydaoValue>& args, BydaoValue& result) override;

    bool    getVar( const QString& varName, BydaoValue& value ) override;

    bool    getServerParams( BydaoValue& result );

protected:
    bool initialize() override;
    bool shutdown() override;

private:

    // Методы модуля
    bool method_server(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_request(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_response(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoWebModule::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    using GetVarPtr = bool (BydaoWebModule::*)(BydaoValue&);
    using SetVarPtr = bool (BydaoWebModule::*)(const BydaoValue&);
    struct VarMethod {
        GetVarPtr   getter;
        SetVarPtr   setter;
    };
    QHash<QString,VarMethod> m_vars;

    void registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter = nullptr );

    bool getvar_server( BydaoValue& value ) {
        return getServerParams( value );
    };

    bool getvar_request( BydaoValue& value ) {
        if ( ! m_request ) {
            m_request = new WebRequest();
            m_request->ref();
            m_request->setRuntime( m_runtime );
        }
        value = BydaoValue( (BydaoObject*)m_request, BydaoTypeId::TYPE_OBJECT );
        return true;
    };

    bool getvar_response( BydaoValue& value ) {
        if ( ! m_response ) {
            m_response = new WebResponse();
            m_response->ref();
            m_response->setRuntime( m_runtime );
        }
        value = BydaoValue( (BydaoObject*)m_response, BydaoTypeId::TYPE_OBJECT );
        return true;
    };

    WebRequest*     m_request;      // объект запроса с веб-сервера
    WebResponse*    m_response;     // объект ответа веб-серверу

};

//==============================================================================

} // namespace Modules
} // namespace BydaoScript
