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