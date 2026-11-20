#pragma once

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>
#include <span>
#include <cstdint>


struct ChordMatch {
    int root = 0; // chord root
    int bass = 0; // lowest note
    std::string suffix; // "", "m", "maj7", "dim7", etc.
    std::string typeName; // "Major Triad", "Minor Seventh", etc.
    std::string name; // "C", "Cmaj7", "Cdim7", etc.
    bool inversion = false; // true when lowest note is not the root
};

struct ChordAnalysis {
    enum class Status {
        Empty, // nothing held
        SingleNote, // one pitch
        Recognized, // matches at least one named chord
        Unnamed, // 2-4 notes, but no match
        TooManyNotes // more than 4 notes
    };

    Status status = Status::Empty;
    int distinctNotes = 0;
    std::vector<ChordMatch> matches; // best reading first
    std::string label; // ready to display
};


class ChordLogic {
public:
    static constexpr int MAX_CHORD_NOTES = 4;

    ChordLogic(); // builds the chord table

    std::string identifyChord(const std::unordered_set<int>& pressedKeys) const;

    ChordAnalysis analyze(std::span<const int> midiKeys) const;
    ChordAnalysis analyze(const std::unordered_set<int>& midiKeys) const;

    //Pitch classes of a chord given its root note
    std::vector<int> chordPitchClasses(int rootPitchClass, const std::string& suffix) const;

    //Pitch class name (0-11); black keys use flats except F#
    static std::string noteName(int pitchClass);
    //MIDI key name with octave
    static std::string midiNoteName(int midiKey);
private:
    struct ChordShape {
        std::string suffix;
        std::string name;
        std::uint16_t mask;
        std::size_t priority; // lower wins when two are equal
    };

    void setup();

    // Chord shape
    std::unordered_map<std::uint16_t, std::vector<ChordShape>> chords;
};


