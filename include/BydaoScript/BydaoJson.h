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

#ifndef BYDAOJSON_H
#define BYDAOJSON_H

#include <QVector>

#include "BydaoObject.h"
#include "BydaoValue.h"
#include "BydaoRuntime.h"
#include "BydaoMetaData.h"
#include "BydaoLexer.h"

namespace BydaoScript {

class BydaoJson : public BydaoObject {

public:
    explicit BydaoJson();
    virtual ~BydaoJson() override = default;

    enum JsonStyle {
        COMPACT     = 0,    // сжатый формат (например, для передачи по запросу)
        PRETTY      = 1     // форматирование для наглядности
    };

    // Получить мета-данные
    static MetaData*   metaData();

    // Получить список используемых мета-данных
    static UsedMetaDataList usedMetaData();

    QString      toString( const BydaoValue& val, JsonStyle style );

    struct ParseContext {
        BydaoLexer*         lexer;
        QVector<BydaoToken> tokens;
        int                 pos;
    };
    BydaoValue   fromString( const QString& str );

    QString     typeName() const override { return "Json"; }

    bool        getVar( const QString& varName, BydaoValue& value ) override;

    bool        callMethod(const QString& name, const QVector<BydaoValue>& args, BydaoValue& result) override;

    void        setRuntime(BydaoRuntime* runtime) {
        m_runtime = runtime;
    };

private:

    BydaoValue   parseValue( ParseContext* ctx );
    BydaoValue   parseArray( ParseContext* ctx );
    BydaoValue   parseObject( ParseContext* ctx );

    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_fromString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_error(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoJson::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;

    using GetVarPtr = bool (BydaoJson::*)(BydaoValue&);
    using SetVarPtr = bool (BydaoJson::*)(const BydaoValue&);
    struct VarMethod {
        GetVarPtr   getter;
        SetVarPtr   setter;
    };
    QHash<QString,VarMethod> m_vars;

    void registerVar(const QString& name, GetVarPtr getter, SetVarPtr setter = nullptr );

    bool getvar_COMPACT( BydaoValue& value ) {
        value = BydaoValue::fromInt( COMPACT );
        return true;
    };

    bool getvar_PRETTY( BydaoValue& value ) {
        value = BydaoValue::fromInt( PRETTY );
        return true;
    };

    BydaoRuntime*   m_runtime;
};

} // namespace BydaoScript

#endif // BYDAOJSON_H
