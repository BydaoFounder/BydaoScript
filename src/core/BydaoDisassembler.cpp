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
#include "BydaoScript/BydaoDisassembler.h"
#include <QHash>
#include <cmath>

namespace BydaoScript {

// ANSI color codes
const QString BydaoDisassembler::RESET = "\033[0m";
const QString BydaoDisassembler::RED = "\033[31m";
const QString BydaoDisassembler::GREEN = "\033[32m";
const QString BydaoDisassembler::YELLOW = "\033[33m";
const QString BydaoDisassembler::BLUE = "\033[34m";
const QString BydaoDisassembler::MAGENTA = "\033[35m";
const QString BydaoDisassembler::CYAN = "\033[36m";
const QString BydaoDisassembler::GRAY = "\033[90m";

BydaoDisassembler::BydaoDisassembler() {}

// ========== Основные методы ==========

QString BydaoDisassembler::disassemble(const QVector<BydaoConstant>& constants,
                                       const QVector<QString>& stringTable,
                                       const QVector<BydaoInstruction>& code,
                                       const QVector<BydaoDebugInfo>* debugInfo) {
    QString result;
    QTextStream out(&result);
    disassemble(out, constants, stringTable, code, debugInfo);
    return result;
}

void BydaoDisassembler::disassemble(QTextStream& out,
                                    const QVector<BydaoConstant>& constants,
                                    const QVector<QString>& stringTable,
                                    const QVector<BydaoInstruction>& code,
                                    const QVector<BydaoDebugInfo>* debugInfo) {

    if (m_showConstants && !constants.isEmpty()) {
        out << "\n" << (m_colorOutput ? CYAN : "") << "=== CONSTANTS TABLE ==="
            << (m_colorOutput ? RESET : "") << "\n";
        out << disassembleConstants(constants, stringTable);
    }

    if (m_showStringTable && !stringTable.isEmpty()) {
        out << "\n" << (m_colorOutput ? CYAN : "") << "=== STRING TABLE ==="
            << (m_colorOutput ? RESET : "") << "\n";
        out << disassembleStringTable(stringTable);
    }

    out << "\n" << (m_colorOutput ? CYAN : "") << "=== BYTECODE ==="
        << (m_colorOutput ? RESET : "") << "\n";
    out << disassembleCode(code, stringTable, debugInfo);
}

// ========== Дизассемблирование таблиц ==========

QString BydaoDisassembler::disassembleConstants(const QVector<BydaoConstant>& constants,
                                                const QVector<QString>& stringTable) {
    QString result;
    QTextStream out(&result);

    for (int i = 0; i < constants.size(); i++) {
        const auto& c = constants[i];

        // Индекс
        if (m_colorOutput)
            out << GREEN << QString("%1").arg(i, 4) << RESET << ": ";
        else
            out << QString("%1: ").arg(i, 4);

        // Значение
        out << formatConstant(c, stringTable);

        // Тип (серым цветом)
        if (m_colorOutput) {
            out << GRAY;
        }
        switch (c.type) {
        case CONST_INT: out << " (int)"; break;
        case CONST_REAL: out << " (real)"; break;
        case CONST_STRING: out << " (string)"; break;
        case CONST_BOOL: out << " (bool)"; break;
        case CONST_NULL: out << " (null)"; break;
        }
        if (m_colorOutput) {
            out << RESET;
        }

        out << "\n";
    }

    return result;
}

QString BydaoDisassembler::disassembleStringTable(const QVector<QString>& stringTable) {
    QString result;
    QTextStream out(&result);

    for (int i = 0; i < stringTable.size(); i++) {
        if (stringTable[i].isEmpty())
            continue;

        if (m_colorOutput)
            out << GREEN << QString("%1").arg(i, 4) << RESET << ": ";
        else
            out << QString("%1: ").arg(i, 4);

        out << "'" << formatString(stringTable[i]) << "'\n";
    }

    return result;
}

QString BydaoDisassembler::disassembleCode(const QVector<BydaoInstruction>& code,
                                           const QVector<QString>& stringTable,
                                           const QVector<BydaoDebugInfo>* debugInfo) {
    QString result;
    QTextStream out(&result);

    // Собираем метки для переходов
    QHash<int, QString> labels;
    for (int i = 0; i < code.size(); i++) {
        const auto& instr = code[i];
        if (instr.op == BydaoOpCode::Jump ||
            instr.op == BydaoOpCode::JumpIfFalse ||
            instr.op == BydaoOpCode::JumpIfTrue) {
            int target = instr.arg1;
            if (!labels.contains(target)) {
                labels[target] = QString("L%1").arg(labels.size());
            }
        }
    }

    // Выводим инструкции
    for (int i = 0; i < code.size(); i++) {
        const auto& instr = code[i];

        // Метка, если есть
        if (labels.contains(i)) {
            if (m_colorOutput)
                out << YELLOW << labels[i] << ":" << RESET << "\n";
            else
                out << labels[i] << ":\n";
        }

        out << formatInstruction(i, instr, stringTable, debugInfo) << "\n";
    }

    return result;
}

// ========== Форматирование инструкции ==========

QString BydaoDisassembler::formatInstruction(int index,
                                             const BydaoInstruction& instr,
                                             const QVector<QString>& stringTable,
                                             const QVector<BydaoDebugInfo>* debugInfo) {
    QString result;

    // Отступ
    result += QString(m_indent, ' ');

    // Смещение (индекс инструкции)
    if (m_showOffsets) {
        if (m_colorOutput)
            result += CYAN + QString("[%1]").arg(index, 4) + RESET + " ";
        else
            result += QString("[%1] ").arg(index, 4);
    }

    // Отладочная информация (строка и колонка)
    if (m_showLineNumbers || m_showDebugInfo) {
        const BydaoDebugInfo* di = findDebugInfo(index, debugInfo);
        if (di) {
            if (m_colorOutput)
                result += GRAY + QString("(%1:%2)").arg(di->line).arg(di->column) + RESET + " ";
            else
                result += QString("(%1:%2) ").arg(di->line).arg(di->column);
        } else if (instr.line != 0 && m_showLineNumbers) {
            if (m_colorOutput)
                result += GRAY + QString("(%1:%2)").arg(instr.line).arg(instr.column) + RESET + " ";
            else
                result += QString("(%1:%2) ").arg(instr.line).arg(instr.column);
        }
    }

    // Имя инструкции
    QString opName = BydaoBytecode::opcodeToString(instr.op);
    if (m_colorOutput)
        result += GREEN + opName + RESET;
    else
        result += opName;

    // Аргументы
    QString args = formatArg(instr, stringTable);
    if (!args.isEmpty()) {
        if (m_colorOutput)
            result += " " + BLUE + args + RESET;
        else
            result += " " + args;
    }

    return result;
}

QString BydaoDisassembler::formatArg(const BydaoInstruction& instr,
                                     const QVector<QString>& stringTable) {
    QStringList args;

    switch (instr.op) {
    case BydaoOpCode::PushConst:
//        if (instr.arg1 != 0)
            args << QString("c%1").arg(instr.arg1);
        break;

    case BydaoOpCode::Load:
    case BydaoOpCode::Store:
    case BydaoOpCode::Drop:
//        if (instr.arg1 != 0 || instr.arg2 != 0)
            args << QString("s%1").arg(instr.arg1) << QString("v%1").arg(instr.arg2);
        break;

    case BydaoOpCode::VarDecl:
        if (instr.arg1 >= 0 && instr.arg1 < stringTable.size() && !stringTable[instr.arg1].isEmpty())
            args << "'" + formatString(stringTable[instr.arg1]) + "'";
        else if (instr.arg1 != 0)
            args << QString("n%1").arg(instr.arg1);
        break;

    case BydaoOpCode::Member:
    case BydaoOpCode::TypeClass:
        if (instr.arg1 >= 0 && instr.arg1 < stringTable.size() && !stringTable[instr.arg1].isEmpty())
            args << "'" + formatString(stringTable[instr.arg1]) + "'";
        else if (instr.arg1 != 0)
            args << QString("idx%1").arg(instr.arg1);
        break;

    case BydaoOpCode::UseModule:
        if (instr.arg1 >= 0 && instr.arg1 < stringTable.size())
            args << "'" + formatString(stringTable[instr.arg1]) + "'";
        if (instr.arg2 >= 0 && instr.arg2 < stringTable.size() && instr.arg2 != instr.arg1)
            args << "as '" + formatString(stringTable[instr.arg2]) + "'";
        break;

    case BydaoOpCode::Call:
        args << QString("%1").arg(instr.arg1);
        if (instr.arg2 != 0)
            args << QString("m%1").arg(instr.arg2);
        break;

    case BydaoOpCode::PushArray:
        args << QString("%1").arg(instr.arg1);
        break;

    case BydaoOpCode::Jump:
    case BydaoOpCode::JumpIfFalse:
    case BydaoOpCode::JumpIfTrue:
        args << QString("%1").arg(instr.arg1);
        break;

    default:
        if (instr.arg1 != 0)
            args << QString("%1").arg(instr.arg1);
        if (instr.arg2 != 0)
            args << QString("%1").arg(instr.arg2);
        break;
    }

    return args.join(", ");
}

QString BydaoDisassembler::formatConstant(const BydaoConstant& c,
                                          const QVector<QString>& stringTable) {
    switch (c.type) {
    case CONST_INT:
        return QString::number(c.intValue);

    case CONST_REAL: {
        // Убираем лишние нули
        QString str = QString::number(c.realValue, 'g', 16);
        if (!str.contains('.') && !str.contains('e'))
            str += ".0";
        return str;
    }

    case CONST_STRING:
        if (c.stringIndex < (quint32)stringTable.size()) {
            return "'" + formatString(stringTable[c.stringIndex]) + "'";
        }
        return "<invalid string>";

    case CONST_BOOL:
        return c.boolValue ? "true" : "false";

    case CONST_NULL:
        return "null";

    default:
        return "<?>";
    }
}

QString BydaoDisassembler::formatString(const QString& str) {
    if (str.length() > m_maxStringLength) {
        return str.left(m_maxStringLength - 3) + "...";
    }

    // Экранирование спецсимволов
    QString result;
    for (QChar ch : str) {
        if (ch == '\n') result += "\\n";
        else if (ch == '\t') result += "\\t";
        else if (ch == '\r') result += "\\r";
        else if (ch == '\\') result += "\\\\";
        else if (ch == '\'') result += "\\'";
        else if (ch == '"') result += "\\\"";
        else if (ch.unicode() < 32) {
            result += QString("\\x%1").arg((quint32)ch.unicode(), 2, 16, QChar('0'));
        } else {
            result += ch;
        }
    }
    return result;
}

// ========== Поиск отладочной информации ==========

const BydaoDebugInfo* BydaoDisassembler::findDebugInfo(int instructionIndex,
                                                       const QVector<BydaoDebugInfo>* debugInfo) const {
    if (!debugInfo) return nullptr;

    // Бинарный поиск (debugInfo отсортирован по instructionIndex)
    int left = 0;
    int right = debugInfo->size() - 1;

    while (left <= right) {
        int mid = (left + right) / 2;
        const auto& di = (*debugInfo)[mid];

        if (di.instructionIndex == (quint32)instructionIndex) {
            return &di;
        } else if (di.instructionIndex < (quint32)instructionIndex) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return nullptr;
}

} // namespace BydaoScript
