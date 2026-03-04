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

#include "BydaoIterator.h"
#include "BydaoString.h"

namespace BydaoScript {

class BydaoStringIterator : public BydaoIterator {
    Q_OBJECT

public:
    explicit BydaoStringIterator(BydaoString* str, QObject* parent = nullptr);
    virtual ~BydaoStringIterator();

    QString typeName() const override { return "StringIterator"; }

    // Реализация методов итератора
    bool next() override;
    bool isValid() const override;
    BydaoValue key() const override;
    BydaoValue value() const override;

private:
    BydaoString* m_string;
    int m_index;
};

} // namespace BydaoScript