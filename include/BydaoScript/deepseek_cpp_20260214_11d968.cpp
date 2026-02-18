#pragma once

#include <QString>
#include <QVector>

namespace BydaoScript {

class BydaoValue;

class BydaoObject {
public:
    virtual ~BydaoObject() = default;
    
    // Единственный метод вызова
    virtual bool callMethod(const QString& name,
                           const QVector<BydaoValue>& args,
                           BydaoValue& result) = 0;
    
    // Информация о типе
    virtual QString typeName() const = 0;
};

} // namespace BydaoScript