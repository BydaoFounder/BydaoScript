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

class BydaoIntClass : public BydaoObject {

public:
    explicit BydaoIntClass();
    virtual ~BydaoIntClass() = default;

    // Получить мета-данные
    static MetaData*   metaData();

    // Получить список используемых мета-данных
    static UsedMetaDataList usedMetaData();

    QString typeName() const override { return "Int"; }

    bool callMethod(const QString& name,
                    const BydaoValueList& args,
                    BydaoValue* result) override;

private:
    // Статические методы класса Int
    bool method_range(const BydaoValueList& args, BydaoValue* result);
    bool method_parse(const BydaoValueList& args, BydaoValue* result);
    bool method_max(const BydaoValueList& args, BydaoValue* result);
    bool method_min(const BydaoValueList& args, BydaoValue* result);
    bool method_random(const BydaoValueList& args, BydaoValue* result);

    using MethodPtr = bool (BydaoIntClass::*)(const BydaoValueList&, BydaoValue*);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;
};

} // namespace BydaoScript
