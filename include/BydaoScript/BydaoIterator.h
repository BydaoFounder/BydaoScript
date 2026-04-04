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

namespace BydaoScript {

class BydaoIterator : public BydaoObject {

public:
    explicit BydaoIterator();
    virtual ~BydaoIterator();

    QString typeName() const override { return "Iter"; }

    bool callMethod(const QString& name, const BydaoValueList& args, BydaoValue* result) override;

    // Виртуальные методы, которые должны реализовать конкретные итераторы
    virtual bool        next() = 0;
    virtual bool        isValid() const = 0;
    virtual BydaoValue* key() const = 0;
    virtual BydaoValue* value() const = 0;

    // Свойства для доступа через точку
    BydaoValue* getKey() const { return key(); }
    BydaoValue* getValue() const { return value(); }

private:

    // Реализация методов для вызова из VM
    bool method_next(const BydaoValueList& args, BydaoValue* result);
    bool method_isValid(const BydaoValueList& args, BydaoValue* result);
    bool method_key(const BydaoValueList& args, BydaoValue* result);
    bool method_value(const BydaoValueList& args, BydaoValue* result);

    using MethodPtr = bool (BydaoIterator::*)(const BydaoValueList&, BydaoValue*);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;
};

} // namespace BydaoScript