#include "BydaoScript/BydaoBytecode.h"
#include <QFile>
#include <QDataStream>

namespace BydaoScript {

static const quint32 BYDAO_MAGIC = 0x42594453;
static const quint32 BYDAO_VERSION = 0x00010000;

bool BydaoBytecode::save(const QVector<BydaoInstruction>& code, const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QDataStream ds(&file);
    ds.setVersion(QDataStream::Qt_6_0);
    ds << BYDAO_MAGIC;
    ds << BYDAO_VERSION;
    ds << (quint32)code.size();

    for (const auto& i : code) {
        ds << (quint8)i.op;
        ds << i.arg;
        ds << (qint32)i.line;
        ds << (qint32)i.column;
    }
    return true;
}

QVector<BydaoInstruction> BydaoBytecode::load(const QString& filename, QString* error) {
    QVector<BydaoInstruction> result;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = "Cannot open file";
        return result;
    }

    QDataStream ds(&file);
    ds.setVersion(QDataStream::Qt_6_0);
    quint32 magic, version, count;
    ds >> magic >> version >> count;

    if (magic != BYDAO_MAGIC) {
        if (error) *error = "Invalid bytecode format";
        return result;
    }

    for (quint32 i = 0; i < count; ++i) {
        quint8 op; QString arg; qint32 line, col;
        ds >> op >> arg >> line >> col;
        result.append(BydaoInstruction((BydaoOpCode)op, arg, line, col));
    }
    return result;
}

} // namespace BydaoScript