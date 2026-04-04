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

#include "BydaoObject.h"
#include "BydaoMetaData.h"

namespace BydaoScript {

class BydaoBool : public BydaoObject {

    static QList<BydaoBool*> s_cache;
    static const int MAX_CACHE_SIZE = 1024;

protected:

    void release() override {
        if (s_cache.size() < MAX_CACHE_SIZE) {
            m_refCount = 0;  // сбрасываем для следующего использования
            s_cache.append(this);
        } else {
            delete this;
        }
    }

    explicit BydaoBool(bool value = false);

public:

    static BydaoBool* create(bool value) {
        if (!s_cache.isEmpty()) {
            BydaoBool* obj = s_cache.takeLast();
            obj->m_value = value;
            return obj;
        }
        return new BydaoBool(value);
    }

    // Получить мета-данные
    static MetaData*   metaData();

    QString typeName() const override { return "Bool"; }
    bool value() const { return m_value; }

    bool callMethod(const QString& name,
                    const BydaoValueList& args,
                    BydaoValue* result) override;

    BydaoObject* copy() override {
        return BydaoBool::create( m_value );
    }

    void    assign( BydaoObject* obj ) override {
        m_value = ((BydaoBool*)obj)->m_value;
    }
    BydaoValue* eq(const BydaoValue* other) override;
    BydaoValue* neq(const BydaoValue* other) override;

private:

    bool method_toString(const BydaoValueList& args, BydaoValue* result);
    bool method_toInt(const BydaoValueList& args, BydaoValue* result);
    bool method_toReal(const BydaoValueList& args, BydaoValue* result);
    bool method_toBool(const BydaoValueList& args, BydaoValue* result);
    bool method_isNull(const BydaoValueList& args, BydaoValue* result);

    using MethodPtr = bool (BydaoBool::*)(const BydaoValueList&, BydaoValue*);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;  // своя таблица методов

    bool m_value;
};

} // namespace BydaoScript
