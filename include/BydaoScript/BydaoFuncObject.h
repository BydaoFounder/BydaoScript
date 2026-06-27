#ifndef BYDAOFUNCOBJECT_H
#define BYDAOFUNCOBJECT_H

#pragma once

#include "BydaoObject.h"
#include "BydaoBytecode.h"
#include "BydaoRuntime.h"
#include "BydaoMetaData.h"

namespace BydaoScript {

class BydaoFuncObject : public BydaoObject {
public:
    BydaoFuncObject();
    virtual ~BydaoFuncObject();              // = default;

    // BydaoObject
    QString typeName() const override { return "Func"; }
    bool callMethod(const QString& name,
                    const QVector<BydaoValue>& args,
                    BydaoValue& result) override;

    BydaoObject* copy() override {
        // Функции копируются по ссылке (возвращаем this)
        this->ref();
        return this;
    }

    // Получить мета-данные для всех объектов типа "Func"
    MetaData*   metaData();

    // Данные функции
    QString                 name;
    QVector<BydaoInstruction> bytecode;
    BydaoInstruction*       code;
    int                     codeSize;
    int                     entryPc;        // индекс первой инструкции в пуле
    int                     arity;          // количество параметров
    FuncMetaData            funcMetaData;   // метаданные объекта функции (типы и т.д.)
    int                     scopeOffset;

private:
    bool method_toString(const QVector<BydaoValue>& args, BydaoValue& result);
    bool method_isNull(const QVector<BydaoValue>& args, BydaoValue& result);

    using MethodPtr = bool (BydaoFuncObject::*)(const QVector<BydaoValue>&, BydaoValue&);
    void registerMethod(const QString& name, MethodPtr method);

    QHash<QString, MethodPtr> m_methods;
};

} // namespace BydaoScript

#endif // BYDAOFUNCOBJECT_H
