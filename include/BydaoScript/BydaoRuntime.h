#ifndef BYDAORUNTIME_H
#define BYDAORUNTIME_H

#include <QtCore>

#include "BydaoValue.h"

namespace BydaoScript {

// Информация о переменной в runtime

struct RuntimeVar {
    QString name;        // имя переменной (для отладки)
    BydaoValue value;    // значение
};

typedef QList<RuntimeVar>   VarScope;

class BydaoConfig;
class BydaoLogger;

class BydaoRuntime {
public:

    virtual ~BydaoRuntime() = default;

    typedef QHash< QString, QString > Environment;

    virtual QString         moduleName() = 0;

    virtual int             line() = 0;

    virtual void            setConfig( BydaoConfig* conf ) = 0;
    virtual BydaoConfig*    getConfig() = 0;

    virtual void            setLogger( BydaoLogger* log ) = 0;
    virtual BydaoLogger*    getLogger() = 0;

    virtual void            setEnvironment( Environment* env ) = 0;
    virtual Environment*    getEnvironment() = 0;

    virtual void            setInputData( const QByteArray* inputData ) = 0;
    virtual const QByteArray* getInputData() const = 0;

    virtual void            setTraceId( const QString& traceId ) = 0;
    virtual QString         getTraceId() const = 0;

    virtual QTextStream* outStream() = 0;
    virtual QTextStream* errStream() = 0;

    virtual void    logError(const QString& msg) = 0;
    virtual QString lastError() const = 0;

    virtual bool callFunction(BydaoValue func, const QVector<BydaoValue>& args, BydaoValue& result) = 0;
};

} // namespace BydaoScript

#endif // BYDAORUNTIME_H
