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
#include <QVector>

namespace BydaoScript {

class BydaoArray : public BydaoNative {
    Q_OBJECT

public:
    explicit BydaoArray(QObject* parent = nullptr);
    virtual ~BydaoArray() override = default;

    QString typeName() const override { return "Array"; }

    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

    int         size() const { return m_elements.size(); }
    BydaoValue  at(qint64 index) const;
    void        set(qint64 index, const BydaoValue& value);
    BydaoValue  get(const BydaoValue& index);
    void        append(const BydaoValue& value);
    void        insert(qint64 index, const BydaoValue& value);
    void        removeAt(qint64 index);
    void        clear();

    BydaoValue  iter();  // создаёт итератор для массива

private:

    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_length(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_get(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_set(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_push(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_pop(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_shift(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_unshift(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_slice(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_join(const QVector<BydaoValue>& args, BydaoValue& result);

    bool method_iter(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoArray::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;

    QVector<BydaoValue> m_elements;
};

} // namespace BydaoScript
