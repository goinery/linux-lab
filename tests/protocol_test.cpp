#include "protocol.h"

#include <QCoreApplication>
#include <QJsonObject>

#include <cstdlib>

namespace {

void require(bool condition, const char *message) {
    if (!condition) {
        qCritical() << "FAILED:" << message;
        std::exit(EXIT_FAILURE);
    }
}

QByteArray rawFrame(const QByteArray &body) {
    const quint32 size = quint32(body.size());
    QByteArray frame(4, '\0');
    frame[0] = char((size >> 24) & 0xFF);
    frame[1] = char((size >> 16) & 0xFF);
    frame[2] = char((size >> 8) & 0xFF);
    frame[3] = char(size & 0xFF);
    frame.append(body);
    return frame;
}

} // namespace

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    const QJsonObject source{{"type", "system"}, {"content", "测试消息"}};
    const QByteArray frame = Protocol::serializeMessage(source);
    require(!frame.isEmpty(), "valid message should serialize");

    QByteArray fragmented = frame.left(3);
    QJsonObject decoded;
    require(Protocol::deserializeMessage(fragmented, decoded)
                == Protocol::DecodeStatus::Incomplete,
            "partial header should be incomplete");
    fragmented.append(frame.mid(3));
    require(Protocol::deserializeMessage(fragmented, decoded)
                == Protocol::DecodeStatus::Complete,
            "complete frame should decode");
    require(decoded == source, "decoded JSON should match source");

    QByteArray combined = frame + frame;
    require(Protocol::deserializeMessage(combined, decoded)
                == Protocol::DecodeStatus::Complete,
            "first sticky frame should decode");
    require(Protocol::deserializeMessage(combined, decoded)
                == Protocol::DecodeStatus::Complete,
            "second sticky frame should decode");
    require(combined.isEmpty(), "all sticky data should be consumed");

    const quint32 oversized = Protocol::MAX_PAYLOAD_SIZE + 1;
    QByteArray invalidLength(4, '\0');
    invalidLength[0] = char((oversized >> 24) & 0xFF);
    invalidLength[1] = char((oversized >> 16) & 0xFF);
    invalidLength[2] = char((oversized >> 8) & 0xFF);
    invalidLength[3] = char(oversized & 0xFF);
    QString error;
    require(Protocol::deserializeMessage(invalidLength, decoded, &error)
                == Protocol::DecodeStatus::Invalid,
            "oversized frame should be rejected");
    require(!error.isEmpty(), "invalid frame should provide an error");

    QByteArray arrayFrame = rawFrame("[]");
    require(Protocol::deserializeMessage(arrayFrame, decoded, &error)
                == Protocol::DecodeStatus::Invalid,
            "non-object JSON should be rejected");

    const QJsonObject huge{{"type", "system"},
                           {"content", QString(Protocol::MAX_PAYLOAD_SIZE + 1, 'x')}};
    require(Protocol::serializeMessage(huge).isEmpty(),
            "oversized outgoing message should be rejected");

    qInfo() << "PASS: protocol framing, fragmentation, sticky packets and limits";
    return EXIT_SUCCESS;
}

