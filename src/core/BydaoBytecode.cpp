#include "BydaoScript/BydaoBytecode.h"
#include <QFile>
#include <QDataStream>
#include <QHash>

namespace BydaoScript {

// ========== Преобразование опкода в строку ==========

QString BydaoBytecode::opcodeToString(BydaoOpCode op) {
    static QHash<BydaoOpCode, QString> names;
    if (names.isEmpty()) {
        names[BydaoOpCode::Nop] = "NOP";
        names[BydaoOpCode::Halt] = "HALT";
        names[BydaoOpCode::VarDecl] = "VARDECL";
        names[BydaoOpCode::Drop] = "DROP";
        names[BydaoOpCode::Load] = "LOAD";
        names[BydaoOpCode::Store] = "STORE";
        names[BydaoOpCode::PushConst] = "PUSHCONST";
        names[BydaoOpCode::Add] = "ADD";
        names[BydaoOpCode::Sub] = "SUB";
        names[BydaoOpCode::Mul] = "MUL";
        names[BydaoOpCode::Div] = "DIV";
        names[BydaoOpCode::Mod] = "MOD";
        names[BydaoOpCode::Neg] = "NEG";
        names[BydaoOpCode::Eq] = "EQ";
        names[BydaoOpCode::Neq] = "NEQ";
        names[BydaoOpCode::Lt] = "LT";
        names[BydaoOpCode::Gt] = "GT";
        names[BydaoOpCode::Le] = "LE";
        names[BydaoOpCode::Ge] = "GE";
        names[BydaoOpCode::And] = "AND";
        names[BydaoOpCode::Or] = "OR";
        names[BydaoOpCode::Not] = "NOT";
        names[BydaoOpCode::GetIter] = "GETITER";
        names[BydaoOpCode::ItNext] = "ITNEXT";
        names[BydaoOpCode::ItKey] = "ITKEY";
        names[BydaoOpCode::ItValue] = "ITVALUE";
        names[BydaoOpCode::Member] = "MEMBER";
        names[BydaoOpCode::Method] = "METHOD";  // ← новая строка
        names[BydaoOpCode::Index] = "INDEX";
        names[BydaoOpCode::Call] = "CALL";
        names[BydaoOpCode::Jump] = "JUMP";
        names[BydaoOpCode::JumpIfFalse] = "JMPF";
        names[BydaoOpCode::JumpIfTrue] = "JMPT";
        names[BydaoOpCode::ScopeBegin] = "SCOPEBEG";
        names[BydaoOpCode::ScopeEnd] = "SCOPEEND";
        names[BydaoOpCode::ScopePush] = "SCOPEPUSH";
        names[BydaoOpCode::ScopePop] = "SCOPEPOP";
        names[BydaoOpCode::UseModule] = "USE";
        names[BydaoOpCode::TypeClass] = "TYPECLASS";
        names[BydaoOpCode::PushArray] = "PUSHARRAY";
    }
    return names.value(op, "???");
}

// ========== Сохранение байткода ==========

bool BydaoBytecode::save(const QVector<BydaoConstant>& constants,
                         const QVector<QString>& stringTable,
                         const QVector<BydaoInstruction>& code,
                         const QString& filename,
                         const QVector<BydaoDebugInfo>* debugInfo) {
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setVersion(QDataStream::Qt_6_0);
    
    // Заголовок
    ds << MAGIC;
    ds << VERSION;
    
    // ===== Таблица строк =====
    quint32 stringCount = stringTable.size();
    ds << stringCount;
    
    for (const QString& str : stringTable) {
        QByteArray utf8 = str.toUtf8();
        quint32 size = utf8.size();
        ds << size;
        ds.writeRawData(utf8.constData(), size);
    }
    
    // ===== Таблица констант =====
    quint32 constCount = constants.size();
    ds << constCount;
    
    for (const BydaoConstant& c : constants) {
        ds << (quint8)c.type;
        
        switch (c.type) {
        case CONST_INT:
            ds << c.intValue;
            break;
        case CONST_REAL:
            ds << c.realValue;
            break;
        case CONST_STRING:
            ds << c.stringIndex;
            break;
        case CONST_BOOL:
            ds << (quint8)c.boolValue;
            break;
        case CONST_NULL:
            // ничего не пишем
            break;
        }
    }
    
    // ===== Код =====
    quint32 codeSize = code.size();
    ds << codeSize;
    
    for (const BydaoInstruction& i : code) {
        ds << (quint8)i.op;
        ds << i.arg1;
        ds << i.arg2;
        ds << i.line;
        ds << i.column;
    }
    
    // ===== Отладочная информация (опционально) =====
    if (debugInfo && !debugInfo->isEmpty()) {
        ds << (quint32)1;  // флаг "есть отладочная информация"
        quint32 debugSize = debugInfo->size();
        ds << debugSize;
        
        for (const BydaoDebugInfo& di : *debugInfo) {
            ds << di.instructionIndex;
            ds << di.line;
            ds << di.column;
            ds << di.fileIndex;
        }
    } else {
        ds << (quint32)0;  // флаг "нет отладочной информации"
    }
    
    file.close();
    return true;
}

// ========== Загрузка байткода ==========

bool BydaoBytecode::load(const QString& filename,
                         QVector<BydaoConstant>& constants,
                         QVector<QString>& stringTable,
                         QVector<BydaoInstruction>& code,
                         QVector<BydaoDebugInfo>* debugInfo,
                         QString* error) {
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = "Cannot open file: " + filename;
        return false;
    }
    
    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setVersion(QDataStream::Qt_6_0);
    
    // Читаем заголовок
    quint32 magic, version;
    ds >> magic >> version;
    
    if (magic != MAGIC) {
        if (error) *error = "Invalid bytecode format (magic number mismatch)";
        return false;
    }
    
    if (version != VERSION) {
        if (error) *error = QString("Unsupported bytecode version: %1 (expected %2)")
                                 .arg(version, 0, 16)
                                 .arg(VERSION, 0, 16);
        return false;
    }
    
    // ===== Таблица строк =====
    quint32 stringCount;
    ds >> stringCount;
    
    stringTable.clear();
    stringTable.reserve(stringCount);
    
    for (quint32 i = 0; i < stringCount; ++i) {
        quint32 strSize;
        ds >> strSize;
        
        QByteArray utf8;
        utf8.resize(strSize);
        ds.readRawData(utf8.data(), strSize);
        
        stringTable.append(QString::fromUtf8(utf8));
    }
    
    // ===== Таблица констант =====
    quint32 constCount;
    ds >> constCount;
    
    constants.clear();
    constants.reserve(constCount);
    
    for (quint32 i = 0; i < constCount; ++i) {
        quint8 type;
        ds >> type;
        
        switch (type) {
        case CONST_INT: {
            qint64 value;
            ds >> value;
            constants.append(BydaoConstant(value));
            break;
        }
        case CONST_REAL: {
            double value;
            ds >> value;
            constants.append(BydaoConstant(value));
            break;
        }
        case CONST_STRING: {
            quint32 strIndex;
            ds >> strIndex;
            constants.append(BydaoConstant(strIndex));
            break;
        }
        case CONST_BOOL: {
            quint8 value;
            ds >> value;
            constants.append(BydaoConstant(value != 0));
            break;
        }
        case CONST_NULL:
            constants.append(BydaoConstant());
            break;
        default:
            if (error) *error = QString("Invalid constant type: %1").arg(type);
            return false;
        }
    }
    
    // ===== Код =====
    quint32 codeSize;
    ds >> codeSize;
    
    code.clear();
    code.reserve(codeSize);
    
    for (quint32 i = 0; i < codeSize; ++i) {
        quint8 op;
        qint16 arg1, arg2, line, col;
        
        ds >> op;
        ds >> arg1;
        ds >> arg2;
        ds >> line;
        ds >> col;
        
        code.append(BydaoInstruction((BydaoOpCode)op, arg1, arg2, line, col));
    }
    
    // ===== Отладочная информация =====
    quint32 hasDebug;
    ds >> hasDebug;
    
    if (debugInfo && hasDebug) {
        quint32 debugSize;
        ds >> debugSize;
        
        debugInfo->clear();
        debugInfo->reserve(debugSize);
        
        for (quint32 i = 0; i < debugSize; ++i) {
            BydaoDebugInfo di;
            ds >> di.instructionIndex;
            ds >> di.line;
            ds >> di.column;
            ds >> di.fileIndex;
            debugInfo->append(di);
        }
    }
    
    file.close();
    
    // Валидация загруженного кода
    if (!validate(code, constants, stringTable, error)) {
        return false;
    }
    
    return true;
}

// ========== Валидация ==========

bool BydaoBytecode::validate(const QVector<BydaoInstruction>& code,
                             const QVector<BydaoConstant>& constants,
                             const QVector<QString>& stringTable,
                             QString* error) {
    
    // Проверка границ констант
    for (const auto& instr : code) {
        if (instr.op == BydaoOpCode::PushConst) {
            if (instr.arg1 < 0 || instr.arg1 >= constants.size()) {
                if (error) *error = QString("Invalid constant index: %1").arg(instr.arg1);
                return false;
            }
        }
        
        // Проверка индексов в таблице строк для инструкций, которые их используют
        if (instr.op == BydaoOpCode::VarDecl ||
            instr.op == BydaoOpCode::Member ||
            instr.op == BydaoOpCode::UseModule ||
            instr.op == BydaoOpCode::TypeClass) {
            
            if (instr.arg1 < 0 || instr.arg1 >= stringTable.size()) {
                if (error) *error = QString("Invalid string index: %1").arg(instr.arg1);
                return false;
            }
        }
        
        // Проверка переходов
        if (instr.op == BydaoOpCode::Jump ||
            instr.op == BydaoOpCode::JumpIfFalse ||
            instr.op == BydaoOpCode::JumpIfTrue) {
            
            if (instr.arg1 < 0 || instr.arg1 >= code.size()) {
                if (error) *error = QString("Invalid jump target: %1").arg(instr.arg1);
                return false;
            }
        }
    }
    
    return true;
}

} // namespace BydaoScript
