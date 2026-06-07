#ifndef BYDAORUNTIME_H
#define BYDAORUNTIME_H

#include <QtCore>

#include "BydaoValue.h"

namespace BydaoScript {

// Информация о переменной в runtime
struct RuntimeVar {
    QString name;        // имя переменной (для отладки)
    BydaoValue value;    // значение
};

typedef QList<RuntimeVar>   VarScope;

} // namespace BydaoScript

#endif // BYDAORUNTIME_H
