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

#include <QList>

#include "BydaoObject.h"
#include "BydaoMetaData.h"

namespace BydaoScript {

class BydaoArray : public BydaoObject {

public:
    explicit BydaoArray();
    virtual ~BydaoArray() override = default;

    // Получить мета-данные
    static MetaData*   metaData();

    // Получить список используемых мета-данных
    static UsedMetaDataList usedMetaData();

    QString typeName() const override { return "Array"; }

    bool callMethod(const QString& name,
                    const BydaoValueList& args,
                    BydaoValue* result) override;

    virtual int         size() const { return m_elements.size(); }
    virtual BydaoValue* at(qint64 index) const;
    virtual void        set(qint64 index, BydaoValue* value);
    virtual BydaoValue* get(const BydaoValue* index);
    virtual void        append(BydaoValue* value);
    virtual void        insert(qint64 index, BydaoValue* value);
    virtual void        removeAt(qint64 index);
    virtual void        clear();

    BydaoValue*  iter() override;  // создаёт итератор для массива

private:

    bool method_toString(const BydaoValueList& args, BydaoValue* result);
    bool method_length(const BydaoValueList& args, BydaoValue* result);
    bool method_get(const BydaoValueList& args, BydaoValue* result);
    bool method_set(const BydaoValueList& args, BydaoValue* result);
    bool method_push(const BydaoValueList& args, BydaoValue* result);
    bool method_pop(const BydaoValueList& args, BydaoValue* result);
    bool method_shift(const BydaoValueList& args, BydaoValue* result);
    bool method_unshift(const BydaoValueList& args, BydaoValue* result);
    bool method_slice(const BydaoValueList& args, BydaoValue* result);
    bool method_join(const BydaoValueList& args, BydaoValue* result);

    bool method_iter(const BydaoValueList& args, BydaoValue* result);

    using MethodPtr = bool (BydaoArray::*)(const BydaoValueList&, BydaoValue*);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;

    BydaoValueList m_elements;
};

} // namespace BydaoScript
