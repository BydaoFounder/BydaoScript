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

#include <QObject>
#include <QHash>
#include <QSet>
#include <functional>

#include "BydaoObject.h"
#include "BydaoValue.h"

namespace BydaoScript {

struct BydaoPropertyInfo {
    enum Access { ReadOnly, ReadWrite };
    enum Visibility { Public, Internal };

    Access access = ReadWrite;
    Visibility visibility = Public;
    QString doc;

    BydaoPropertyInfo() = default;
    BydaoPropertyInfo(Access a, Visibility v = Public)
        : access(a), visibility(v) {
    }
};

class BydaoNative : public QObject, public BydaoObject {
    Q_OBJECT

public:
    explicit BydaoNative(QObject* parent = nullptr);
    virtual ~BydaoNative();

    // ========== Методы ==========

    // Абстрактный метод — каждый класс реализует сам
    virtual bool callMethod(const QString& name,
                            const QVector<BydaoValue>& args,
                            BydaoValue& result) override = 0;

    // ========== Свойства ==========

    void registerProperty(const QString& name,
                          std::function<BydaoValue()> getter,
                          std::function<bool(const BydaoValue&)> setter = nullptr,
                          const BydaoPropertyInfo& info = BydaoPropertyInfo());

    virtual bool hasProperty(const QString& name) const;
    virtual bool canGetProperty(const QString& name) const;
    virtual bool canSetProperty(const QString& name) const;
    virtual BydaoValue getProperty(const QString& name);
    virtual bool setProperty(const QString& name, const BydaoValue& value);

protected:

    struct PropertyEntry {
        BydaoPropertyInfo info;
        std::function<BydaoValue()> getter;
        std::function<bool(const BydaoValue&)> setter;
    };

    QHash<QString, PropertyEntry> m_properties;
};

} // namespace BydaoScript
