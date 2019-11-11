#include "library/aoide/domain/marker.h"

#include "track/keyutils.h"
#include "util/assert.h"
#include "util/logger.h"

namespace {

const mixxx::Logger kLogger("aoide Marker");

const int kMarkerStateReadWrite = 0;
const int kMarkerStateReadOnly = 1;

const int kPositionMarkerCustom = 0; // fallback for unsupported types
const int kPositionMarkerLoad = 1;   // = Cue::Type::MainCue
const int kPositionMarkerMain = 2;   // = Cue::Type::AudibleSound
const int kPositionMarkerIntro = 3;
const int kPositionMarkerOutro = 4;
const int kPositionMarkerJump = 5; // = Cue::Type::HotCue
const int kPositionMarkerLoop = 6;
//const int kPositionMarkerSample = 7; // unused/unsupported

class CueConverter {
  public:
    explicit CueConverter(
            double cuePositionToMillis)
            : m_cuePositionToMillis(cuePositionToMillis) {
    }

    double convertPositionToMillis(int offset) const {
        return offset * m_cuePositionToMillis;
    }

  private:
    const double m_cuePositionToMillis;
};

} // anonymous namespace

//static
AoidePositionMarker AoidePositionMarker::fromCue(
        const Cue& cue,
        double cuePositionToMillis) {
    AoidePositionMarker pm;
    const CueConverter cueConv(cuePositionToMillis);
    switch (cue.getType()) {
    case Cue::Type::HotCue:
    case Cue::Type::MainCue:
        pm.setStart(cueConv.convertPositionToMillis(cue.getPosition()));
        break;
    case Cue::Type::Intro:
    case Cue::Type::Outro:
        if (cue.getPosition() == -1.0) {
            // No start position
            pm.setEnd(cueConv.convertPositionToMillis(cue.getLength()));
        } else {
            pm.setStart(cueConv.convertPositionToMillis(cue.getPosition()));
            if (cue.getLength() >= 0.0) {
                // Both start and end position
                pm.setEnd(cueConv.convertPositionToMillis(cue.getPosition() + cue.getLength()));
            }
            // else no end position
        }
        break;
    case Cue::Type::Loop:
    case Cue::Type::AudibleSound:
        if (cue.getLength() <= 0.0) {
            kLogger.warning()
                    << "Range has invalid length"
                    << cue.getLength();
            return pm;
        }
        pm.setStart(cueConv.convertPositionToMillis(cue.getPosition()));
        pm.setEnd(cueConv.convertPositionToMillis(cue.getPosition() + cue.getLength()));
        break;
    default:
        kLogger.warning()
                << "Unsupported cue type"
                << static_cast<int>(cue.getType());
        return pm;
    }
    pm.setType(cue.getType());
    pm.setNumber(cue.getHotCue());
    pm.setLabel(cue.getLabel());
    if (cue.getColor()) {
        pm.setColor(cue.getColor()->m_defaultRgba);
    }
    return pm;
}

Cue::Type AoidePositionMarker::type() const {
    int markerType = m_jsonObject.value("m").toInt(kPositionMarkerCustom);
    switch (markerType) {
    case kPositionMarkerLoad:
        return Cue::Type::MainCue;
    case kPositionMarkerMain:
        return Cue::Type::AudibleSound;
    case kPositionMarkerIntro:
        return Cue::Type::Intro;
    case kPositionMarkerOutro:
        return Cue::Type::Outro;
    case kPositionMarkerJump:
        return Cue::Type::HotCue;
    case kPositionMarkerLoop:
        return Cue::Type::Loop;
    default:
        kLogger.warning()
                << "Unsupported position marker type"
                << m_jsonObject.value("m");
        return Cue::Type::Invalid;
    }
}

void AoidePositionMarker::setType(Cue::Type type) {
    int markerType;
    switch (type) {
    case Cue::Type::HotCue:
        markerType = kPositionMarkerJump;
        break;
    case Cue::Type::MainCue:
        markerType = kPositionMarkerLoad;
        break;
    case Cue::Type::Intro:
        markerType = kPositionMarkerIntro;
        break;
    case Cue::Type::Outro:
        markerType = kPositionMarkerOutro;
        break;
    case Cue::Type::Loop:
        markerType = kPositionMarkerLoop;
        break;
    case Cue::Type::AudibleSound:
        markerType = kPositionMarkerMain;
        break;
    default:
        kLogger.warning()
                << "Unsupported cue type"
                << static_cast<int>(type);
        markerType = kPositionMarkerCustom;
        return;
    }
    m_jsonObject.insert("m", markerType);
}

double AoidePositionMarker::start() const {
    return m_jsonObject.value("s").toDouble(std::nan("s"));
}

void AoidePositionMarker::setStart(double start) {
    putOptional("s", start);
}

double AoidePositionMarker::end() const {
    return m_jsonObject.value("e").toDouble(std::nan("e"));
}

void AoidePositionMarker::setEnd(double end) {
    putOptional("e", end);
}

int AoidePositionMarker::number() const {
    return m_jsonObject.value("n").toInt(-1);
}

void AoidePositionMarker::setNumber(int number) {
    if (number >= 0) {
        m_jsonObject.insert("n", number);
    }
}

QString AoidePositionMarker::label() const {
    return m_jsonObject.value("l").toString();
}

void AoidePositionMarker::setLabel(QString label) {
    putOptionalNonEmpty("l", std::move(label));
}

QColor AoidePositionMarker::color() const {
    return QColor(m_jsonObject.value("c").toString());
}

void AoidePositionMarker::setColor(QColor color) {
    putOptionalNonEmpty("c", formatColor(color));
}

bool AoidePositionMarkers::locked() const {
    return m_jsonObject.value("z").toInt(kMarkerStateReadWrite) == kMarkerStateReadOnly;
}

void AoidePositionMarkers::setLocked(bool locked) {
    if (locked) {
        m_jsonObject.insert("z", kMarkerStateReadOnly);
    } else {
        m_jsonObject.remove("z");
    }
}

QVector<AoidePositionMarker> AoidePositionMarkers::markers() const {
    auto array = m_jsonObject.value("m").toArray();
    QVector<AoidePositionMarker> markers;
    markers.reserve(array.size());
    for (auto elem : array) {
        markers.append(AoidePositionMarker(elem.toObject()));
    }
    return markers;
}

void AoidePositionMarkers::setMarkers(QVector<AoidePositionMarker> markers) {
    if (markers.isEmpty()) {
        m_jsonObject.remove("m");
        return;
    }
    QJsonArray array;
    for (auto marker : markers) {
        array.append(marker.intoJsonValue());
    }
    m_jsonObject.insert("m", array);
}

bool AoideBeatMarkers::locked() const {
    return m_jsonObject.value("z").toInt(kMarkerStateReadWrite) == kMarkerStateReadOnly;
}

void AoideBeatMarkers::setLocked(bool locked) {
    if (locked) {
        m_jsonObject.insert("z", kMarkerStateReadOnly);
    } else {
        m_jsonObject.remove("z");
    }
}

QVector<AoideBeatMarker> AoideBeatMarkers::markers() const {
    auto array = m_jsonObject.value("m").toArray();
    QVector<AoideBeatMarker> markers;
    markers.reserve(array.size());
    for (auto elem : array) {
        markers.append(AoideBeatMarker(elem.toObject()));
    }
    return markers;
}

void AoideBeatMarkers::setMarkers(QVector<AoideBeatMarker> markers) {
    if (markers.isEmpty()) {
        m_jsonObject.remove("m");
        return;
    }
    QJsonArray array;
    for (auto marker : markers) {
        array.append(marker.intoJsonValue());
    }
    m_jsonObject.insert("m", array);
}

double AoideBeatMarker::start() const {
    return m_jsonObject.value("s").toDouble(std::nan("s"));
}

void AoideBeatMarker::setStart(double start) {
    putOptional("s", start);
}

double AoideBeatMarker::end() const {
    return m_jsonObject.value("e").toDouble(std::nan("e"));
}

void AoideBeatMarker::setEnd(double end) {
    putOptional("e", end);
}

double AoideBeatMarker::tempoBpm(double defaultBpm) const {
    return m_jsonObject.value("b").toDouble(defaultBpm);
}

void AoideBeatMarker::setTempoBpm(double tempoBpm) {
    if (tempoBpm > 0) {
        m_jsonObject.insert("b", tempoBpm);
    } else {
        m_jsonObject.remove("b");
    }
}

double AoideKeyMarker::start() const {
    return m_jsonObject.value("s").toDouble(std::nan("s"));
}

void AoideKeyMarker::setStart(double start) {
    putOptional("s", start);
}

double AoideKeyMarker::end() const {
    return m_jsonObject.value("e").toDouble(std::nan("e"));
}

void AoideKeyMarker::setEnd(double end) {
    putOptional("e", end);
}

mixxx::track::io::key::ChromaticKey AoideKeyMarker::key() const {
    const int keyCode = m_jsonObject.value("k").toInt(0);
    DEBUG_ASSERT(keyCode >= 0);
    DEBUG_ASSERT(keyCode <= 24);
    if ((keyCode <= 0) || (keyCode > 24)) {
        return mixxx::track::io::key::ChromaticKey::INVALID;
    }
    const int openKeyCode = 1 + (keyCode - 1) / 2;
    const bool major = (keyCode % 2) == 1;
    return KeyUtils::openKeyNumberToKey(openKeyCode, major);
}

void AoideKeyMarker::setKey(mixxx::track::io::key::ChromaticKey chromaticKey) {
    if (chromaticKey == mixxx::track::io::key::ChromaticKey::INVALID) {
        m_jsonObject.remove("k");
    } else {
        const int openKeyNumber = KeyUtils::keyToOpenKeyNumber(chromaticKey);
        DEBUG_ASSERT(openKeyNumber >= 1);
        DEBUG_ASSERT(openKeyNumber <= 12);
        const int keyCode = 2 * openKeyNumber - (KeyUtils::keyIsMajor(chromaticKey) ? 1 : 0);
        m_jsonObject.insert("k", keyCode);
    }
}

bool AoideKeyMarkers::locked() const {
    return m_jsonObject.value("z").toInt(kMarkerStateReadWrite) == kMarkerStateReadOnly;
}

void AoideKeyMarkers::setLocked(bool locked) {
    if (locked) {
        m_jsonObject.insert("z", kMarkerStateReadOnly);
    } else {
        m_jsonObject.remove("z");
    }
}

QVector<AoideKeyMarker> AoideKeyMarkers::markers() const {
    auto array = m_jsonObject.value("m").toArray();
    QVector<AoideKeyMarker> markers;
    markers.reserve(array.size());
    for (auto elem : array) {
        markers.append(AoideKeyMarker(elem.toObject()));
    }
    return markers;
}

void AoideKeyMarkers::setMarkers(QVector<AoideKeyMarker> markers) {
    if (markers.isEmpty()) {
        m_jsonObject.remove("m");
        return;
    }
    QJsonArray array;
    for (auto marker : markers) {
        array.append(marker.intoJsonValue());
    }
    m_jsonObject.insert("m", array);
}
