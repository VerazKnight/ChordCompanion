#include "chordLogic.hpp"

#include <algorithm>
#include <bit>
#include <initializer_list>


namespace {
// build a 12-bit mask from semittone intervals above the root
constexpr std::uint16_t mask(std::initializer_list<int> intervals) {
    std::uint16_t m = 0;
    for (int i : intervals) {
        m = static_cast<std::uint16_t>(m | (1u << i));
    }
    return m;
}

struct ChordType {
    const char* suffix;
    const char* name;
    std::uint16_t mask;
}


const ChordType CHORD_TYPES[] = {
    {"", "Major triad", mask({0, 4, 7})},
};

const char* const NOTE_NAMES[12] = {
    "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};

int pitchClass(int midiKey) {
    return ((midiKey % 12) + 12) %12;
}
} // namespace

ChordLogic::ChordLogic() {
    setup();
}

void ChordLogic::setup() {
    // Populate the chord dictionary
    chords.clear();
    std::size_t priority = 0;
    for (const ChordType& type : CHORD_TYPES) {
        chords[type.mask].push_back({type.suffix, type.name, type.mask, priority++});
    }
}

std::string ChordLogic::noteName(int pc) {
    return NOTE_NAMES[pitchClass(pc)];
}

std::string ChordLogic::midiNoteName(int midiKey) {
    // MIDI 60 is middle C (C4)
    int octave = (midiKey >= 0) ? (midiKey / 12) - 1 : -2;
    return noteName(midiKey) + std::to_string(octave);
}

std::string ChordLogic::identifyChord(const std::unordered_set<int>& pressedKeys) const {
    return analyze(pressedKeys).label;
}

ChordAnalysis ChordLogic::analyze(std::span<const int> midiKeys) const {
    ChordAnalysis result;
    if (midiKeys.empty()) return result;

    const int lowestKey = *std::min_element(midiKeys.begin(), midiKeys.end());
    const int bass = pitchClass(lowestKey);

    unsigned pcMask = 0;
    for (int key : midiKeys) {
        pcMask |= (1u << pitchClass(key));
    }

    const int count = std::popcount(pcMask);
    result.distinctNotes = count;

    if (count == 1) {
        result.status = ChordAnalysis::Status::SingleNote;
        result.label = noteName(bass);
        return result;
    }

    if (count > MAX_CHORD_NOTES) {
        result.status = ChordAnalysis::Status::TooManyNotes;
        result.label = "Too many notes";
        return result;
    }

    // Try every held note as the root
    struct Candidate {
        int root;
        const ChordShape* shape;
    };
    std::vector<Candidate> candidates;
    for (int root = 0; root < 12; ++root) {
        if (!(pcMask & (1u << root))) continue;

        const std::uint16_t rel = static_cast<std::uint16_t> (((pcMask >> root) | (pcMask << 12 - root))) & 0xFFF);
        const auto found = chords.find(rel);

        if (found == chords.end()) continue;

        for (const ChordShape& shape : found->second) {
            candidates.push_back({root, &shape});
        }
    }

    // Two note sets: only lowest note is root
    if (count == 2) {
        std::erase_if(candidates, [bass](const Candidate& c) { return c.root != bass; });
    }

    // Root position first
    std::sort(candidates.begin(), candidates.end(), [bass](const Candidate& a, const candidate&b) {
        const bool aInv = a.root != bass;
        const bool bInv = b.root != bass;
        if (aInv != bInv) return !aInv;
        return a.shape->priority < b.shape->priority;
    });

    if (candidates.empty()) {
        // Valid 2-4 note combiation with no name, describe it by semitone distances above lowest note
        result.status = ChordAnalysis::Status::Unnamed;
        result.label = noteName(bass) + " (unnamed:";
        for (int step = 1; step < 12; ++step) {
            if (pcMask & (1u << ((bass + step) % 12))) {
                result.label += " +" + std::to_string(step);
            }
        }
        result.label += ")";
        return result;
    }

    for (const Candidate& c : candidates) {
        ChordMatch m;
        m.root = c.root;
        m.bass = bass;
        m.suffix = c.shape->suffix;
        m.typeName = c.shape->name;
        m.inversion = (c.root != bass);
        m.name = noteName(c.root) + m.suffix;
        if (m.inversion) {
            m.name += "/" + noteName(bass);
        }
        result.matches.push_back(std::move(m));
    }

    result.status = ChordAnalysis::Status::Recognized;
    result.label = result.matches.front().name;
    return result;
}


ChordAnalysis ChordLogic::analyze(const std::unordered_set<int>& midiKeys) const {
    const std::vector<int> keys(midiKeys.begin(), midiKeys.end());
    return analyze(std::span<const int>(keys));
}

std::vector<int> ChordLogic::chordPitchClasses(int rootPitchClass, const std::string& suffix) const {
    for (const auto& [shapeMask, shapes] : chords) {
        for (const ChordShape& shape : shapes) {
            if (shape.suffix != suffix) continue;
            std::vector<int> notes;
            for (int interval = 0; interval < 12; ++interval) {
                if (shapeMask & (1u << interval)) {
                    notes.push_back(pitchClass(rootPitchClass + interval));
                }
            }
            return notes;
        }
    }
    return {};
}