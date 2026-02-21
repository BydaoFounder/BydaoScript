#pragma once

#include "BydaoIterator.h"
#include "BydaoArray.h"

namespace BydaoScript {

class BydaoArrayIterator : public BydaoIterator {
    Q_OBJECT

public:
    explicit BydaoArrayIterator(BydaoArray* array, QObject* parent = nullptr);
    virtual ~BydaoArrayIterator();

    QString typeName() const override { return "ArrayIterator"; }

    // Реализация методов итератора
    bool next() override;
    bool isValid() const override;
    BydaoValue key() const override;
    BydaoValue value() const override;

private:
    BydaoArray* m_array;
    int m_index;
};

} // namespace BydaoScript