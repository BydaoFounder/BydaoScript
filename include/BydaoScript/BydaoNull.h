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
#pragma once

#include "BydaoNative.h"
#include "BydaoMetaData.h"

namespace BydaoScript {

class BydaoNull : public BydaoNative {
public:
    static BydaoNull* instance();

    // Получить мета-данные
    static MetaData*   metaData();

    QString typeName() const override { return "Null"; }

    BydaoObject* copy() override {
        return BydaoNull::instance();
    }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

    void    assign( BydaoObject* obj ) override {
        Q_UNUSED( obj );
    }
    BydaoValue eq(const BydaoValue& other) override;
    BydaoValue neq(const BydaoValue& other) override;

private:

    BydaoNull();
    
    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_toBool(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoNull::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;

    static BydaoNull* s_instance;
};

} // namespace BydaoScript
