#pragma once

#include "BydaoBytecode.h"
#include <QString>
#include <QTextStream>

namespace BydaoScript {

class BydaoDisassembler {
public:
    BydaoDisassembler();
    
    // Дизассемблировать в строку
    QString disassemble(const QVector<BydaoInstruction>& code);
    
    // Дизассемблировать в поток (файл, stdout)
    void disassemble(const QVector<BydaoInstruction>& code, QTextStream& out);
    
    // Настройки
    void setShowLineNumbers(bool show) { m_showLineNumbers = show; }
    void setShowOffsets(bool show) { m_showOffsets = show; }
    void setColorOutput(bool enable) { m_colorOutput = enable; }

private:
    QString formatInstruction(int index, const BydaoInstruction& instr);
    QString formatArg(const BydaoInstruction& instr);
    
    bool m_showLineNumbers = true;
    bool m_showOffsets = true;
    bool m_colorOutput = false;
    
    // Для цветного вывода (ANSI)
    static const QString RESET;
    static const QString RED;
    static const QString GREEN;
    static const QString YELLOW;
    static const QString BLUE;
    static const QString MAGENTA;
    static const QString CYAN;
};

} // namespace BydaoScript
