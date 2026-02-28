#pragma once

#include "BydaoBytecode.h"
#include <QString>
#include <QTextStream>

namespace BydaoScript {

class BydaoDisassembler {
public:
    BydaoDisassembler();

    // Основные методы дизассемблирования
    QString disassemble(const QVector<BydaoConstant>& constants,
                        const QVector<QString>& stringTable,
                        const QVector<BydaoInstruction>& code,
                        const QVector<BydaoDebugInfo>* debugInfo = nullptr);

    void disassemble(QTextStream& out,
                     const QVector<BydaoConstant>& constants,
                     const QVector<QString>& stringTable,
                     const QVector<BydaoInstruction>& code,
                     const QVector<BydaoDebugInfo>* debugInfo = nullptr);

    // Дизассемблирование отдельных секций
    QString disassembleConstants(const QVector<BydaoConstant>& constants,
                                 const QVector<QString>& stringTable);

    QString disassembleStringTable(const QVector<QString>& stringTable);

    QString disassembleCode(const QVector<BydaoInstruction>& code,
                            const QVector<QString>& stringTable,
                            const QVector<BydaoDebugInfo>* debugInfo = nullptr);

    // Настройки вывода
    void setShowLineNumbers(bool show) { m_showLineNumbers = show; }
    void setShowOffsets(bool show) { m_showOffsets = show; }
    void setShowConstants(bool show) { m_showConstants = show; }
    void setShowStringTable(bool show) { m_showStringTable = show; }
    void setColorOutput(bool enable) { m_colorOutput = enable; }
    void setShowDebugInfo(bool show) { m_showDebugInfo = show; }

    // Форматирование
    void setIndent(int spaces) { m_indent = spaces; }
    void setMaxStringLength(int len) { m_maxStringLength = len; }

private:
    // Вспомогательные методы
    QString formatInstruction(int index, const BydaoInstruction& instr,
                              const QVector<QString>& stringTable,
                              const QVector<BydaoDebugInfo>* debugInfo = nullptr);

    QString formatArg(const BydaoInstruction& instr, const QVector<QString>& stringTable);
    QString formatConstant(const BydaoConstant& c, const QVector<QString>& stringTable);
    QString formatString(const QString& str);

    // Поиск отладочной информации
    const BydaoDebugInfo* findDebugInfo(int instructionIndex,
                                        const QVector<BydaoDebugInfo>* debugInfo) const;

    // Настройки
    bool m_showLineNumbers = true;
    bool m_showOffsets = true;
    bool m_showConstants = true;
    bool m_showStringTable = true;
    bool m_colorOutput = false;
    bool m_showDebugInfo = true;
    int m_indent = 2;
    int m_maxStringLength = 50;

    // ANSI color codes
    static const QString RESET;
    static const QString RED;
    static const QString GREEN;
    static const QString YELLOW;
    static const QString BLUE;
    static const QString MAGENTA;
    static const QString CYAN;
    static const QString GRAY;
};

} // namespace BydaoScript
