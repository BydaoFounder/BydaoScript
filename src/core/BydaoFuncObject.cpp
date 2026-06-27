#include "BydaoScript/BydaoFuncObject.h"
#include "BydaoScript/BydaoString.h"
#include "BydaoScript/BydaoBool.h"

namespace BydaoScript {

BydaoFuncObject::BydaoFuncObject()
    : BydaoObject()
{
    entryPc = 0;
    arity = 0;
    bytecode.clear();
    code = nullptr;
    codeSize = 0;
    registerMethod("toString", &BydaoFuncObject::method_toString);
    registerMethod("isNull", &BydaoFuncObject::method_isNull);
}

BydaoFuncObject::~BydaoFuncObject() {
    if ( code ) {
        delete[] code;
        code = nullptr;
    }
    bytecode.clear();
}

void BydaoFuncObject::registerMethod(const QString& name, MethodPtr method) {
    m_methods[name] = method;
}

bool BydaoFuncObject::callMethod(const QString& name,
                                 const QVector<BydaoValue>& args,
                                 BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (this->*(it.value()))(args, result);
    }
    return false;
}

bool BydaoFuncObject::method_toString(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    QString str = QString("Func(%1)").arg(name.isEmpty() ? "lambda" : name);
    result = BydaoValue( BydaoString::create(str), TYPE_STRING );
    return true;
}

bool BydaoFuncObject::method_isNull(const QVector<BydaoValue>& args, BydaoValue& result) {
    Q_UNUSED(args);
    result = BydaoValue( BydaoBool::create(false), TYPE_BOOL );
    return true;
}

// Получить мета-данные для всех объектов типа "Func"
MetaData* BydaoFuncObject::metaData() {
    static MetaData* metaData = nullptr;
    if (!metaData) {
        metaData = new MetaData();
        metaData
            ->appendType("toString", FuncMetaData("String", FMD_IMMUTABLE))
            .appendType("isNull", FuncMetaData("Bool", FMD_IMMUTABLE));
    }
    return metaData;
}

} // namespace BydaoScript
