#include "BydaoStringModule.h"

namespace BydaoScript {
namespace Modules {

BydaoModuleInfoImpl* BydaoStringModule::s_info = nullptr;

BydaoStringModule::BydaoStringModule() {
    REGISTER_MODULE_METHOD(fromCodePoint);
    REGISTER_MODULE_METHOD(join);
    REGISTER_MODULE_METHOD(format);
}

BydaoModuleInfoImpl* BydaoStringModule::createInfo() {
    auto* info = new BydaoModuleInfoImpl();
    info->m_name = "String";
    info->m_version = "1.0.0";
    info->m_methods = {
        {"fromCodePoint", {"code"}},
        {"join",          {"separator", "array"}},
        {"format",        {"format", "args..."}}
    };
    return info;
}

BydaoModuleInfo* BydaoStringModule::info() const {
    if (!s_info) s_info = createInfo();
    return s_info;
}

bool BydaoStringModule::callMethod(const QString& name,
                                   const QVector<BydaoValue>& args,
                                   BydaoValue& result) {
    auto it = m_methods.find(name);
    if (it != m_methods.end()) {
        return (it.value())(args, result);
    }
    return false;
}

MODULE_METHOD(fromCodePoint) {
    if (args.size() != 1) return false;
    QChar ch(args[0].toInt());
    result = BydaoValue(new BydaoString(QString(ch)));
    return true;
}

MODULE_METHOD(join) {
    if (args.size() != 2) return false;
    QString sep = args[0].toString();
    // TODO: получить массив из args[1]
    result = BydaoValue(new BydaoString(""));
    return true;
}

MODULE_METHOD(format) {
    // TODO: реализовать форматирование
    result = BydaoValue(new BydaoString(""));
    return true;
}

} // namespace Modules
} // namespace BydaoScript