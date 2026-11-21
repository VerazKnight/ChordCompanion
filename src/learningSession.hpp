#pragma once

// learningSession.hpp
//
// The "teaching brain" of the UI, with no ImGui or GLFW. It remembers the
// chords the player has played, guesses the key, decides when to offer ideas,
// warns (gently) about chords outside the key, and checks a practice chord
// chosen with a "Show" button. Time is passed in as a number, so tests can
// use a fake clock instead of the real one.
//
// Nothing in here ever stops the player. Every answer is a friendly hint.

#include "chordTheory.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace Learning {

namespace CT = ChordTheory;

// ---------------------------------------------------------------- practice

enum class PracticeResult { Correct, WrongInversion, DifferentChord };

struct PracticeOutcome {
    PracticeResult result = PracticeResult::DifferentChord;
    std::string message;
};

// The player chose a chord to practice (for example C Major, 1st inversion)
// and then played a full chord. Answer like a friend.
inline PracticeOutcome judgePractice(int targetRoot, int targetType, int targetInversion,
                                     const CT::IdentifiedChord& played) {
    PracticeOutcome out;
    std::string target = CT::chordName(targetRoot, targetType);
    std::string playedName = CT::chordName(played.rootPc, played.typeIndex);

    if (played.rootPc == targetRoot && played.typeIndex == targetType) {
        if (played.inversion == targetInversion) {
            out.result = PracticeResult::Correct;
            out.message = "Yes! " + target + ", " + CT::inversionName(targetInversion) +
                          ". Nailed it! Pick another one to keep going.";
        } else {
            std::vector<int> want = CT::chordNotes(targetRoot, targetType, targetInversion);
            out.result = PracticeResult::WrongInversion;
            out.message = "So close! That is " + target + ", but in " +
                          CT::inversionName(played.inversion) + ". For " +
                          CT::inversionName(targetInversion) + " put " +
                          CT::pitchClassName(CT::pitchClass(want.front())) + " at the bottom.";
        }
    } else {
        out.result = PracticeResult::DifferentChord;
        out.message = "Oh, you went with " + playedName + " instead of " + target +
                      ". No problem! Press Show again whenever you want to try it.";
    }
    return out;
}

// ------------------------------------------------------------- small rules

// Is this chord outside the key the player has been playing in? Only counts
// in Learner mode and after at least 3 chords, so the key is not just a guess.
inline bool isOutsideKey(bool learnerMode, int keyPc, size_t chordsPlayed,
                         const CT::IdentifiedChord& chord) {
    return learnerMode && keyPc >= 0 && chordsPlayed >= 3 &&
           !CT::isDiatonic(keyPc, chord.rootPc, chord.typeIndex);
}

// The key the ideas are given in. In the key: that key. Outside the key: start
// fresh from the chord itself (a major chord is "home", a minor chord is the
// sixth chord of its relative major, a diminished chord leads up a semitone).
inline int ideasKeyFor(int keyPc, const CT::PlayedChord& last) {
    if (CT::isDiatonic(keyPc, last.rootPc, last.typeIndex)) {
        return keyPc;
    }
    if (last.typeIndex == CT::MINOR) {
        return CT::pitchClass(last.rootPc + 3);
    }
    if (last.typeIndex == CT::DIMINISHED) {
        return CT::pitchClass(last.rootPc + 1);
    }
    return last.rootPc;
}

// Four friendly chords to start with when nothing has been played yet.
inline std::vector<CT::Suggestion> starterChords() {
    return {
        CT::suggestionFor(0, 0, "A friendly place to start"),
        CT::suggestionFor(0, 4, "Bright and open"),
        CT::suggestionFor(0, 5, "Soft and a little sad"),
        CT::suggestionFor(0, 3, "Warm, used in many songs"),
    };
}

// ----------------------------------------------------------------- session

class Session {
public:
    // ---- what the player has played ----
    CT::IdentifiedChord currentChord;
    std::vector<CT::PlayedChord> history;   // last 8 chords
    int keyPc = -1;                         // guessed key, -1 = none
    std::unordered_set<int> lastPressed;
    double lastChangeTime = 0.0;
    double pausedFor = 0.0;
    bool hasPlayed = false;

    // ---- ideas ----
    bool suggestionsVisible = false;
    bool suggestRequested = false;
    bool suggestedByPause = false;
    std::vector<CT::Suggestion> suggestions;
    std::string bridgeText;
    std::string feedback;
    int selectedSuggestion = -1;
    int ideasKeyPc = 0;

    // ---- "outside the key" note ----
    bool noticeVisible = false;
    CT::PlayedChord noticeChord;
    int noticeKeyPc = -1;
    double noticeTime = 0.0;

    // ---- practice chord (the "Show" buttons) ----
    // highlightType -1 = nothing chosen.
    int highlightType = -1;
    int highlightInversion = 0;
    int highlightRootPc = -1;
    std::string practiceMsg;
    int practiceMood = 0;                   // 0 hint, 1 happy, 2 oops (friendly)
    double practiceTime = 0.0;
    bool movedOn = false;                   // the UI should follow the played chord again

    // Called every frame with everything that is pressed right now.
    void update(const std::unordered_set<int>& pressed, double now, bool learnerMode,
                bool autoSuggest, double idleSeconds) {
        if (pressed != lastPressed) {
            lastPressed = pressed;
            lastChangeTime = now;
            if (!pressed.empty()) {
                hasPlayed = true;
            }

            CT::IdentifiedChord identified = CT::identifyChord(pressed);
            bool isNewChord = identified.found &&
                (!currentChord.found ||
                 identified.rootPc != currentChord.rootPc ||
                 identified.typeIndex != currentChord.typeIndex);

            if (isNewChord) {
                int oldKeyPc = keyPc;
                feedback = suggestionsVisible ? makeFeedback(oldKeyPc, identified) : "";

                bool outside = isOutsideKey(learnerMode, oldKeyPc, history.size(), identified);

                history.push_back(CT::PlayedChord{ identified.rootPc, identified.typeIndex });
                if (history.size() > 8) {
                    history.erase(history.begin());
                }

                // Old ideas are done as soon as the player plays something.
                suggestionsVisible = false;
                suggestRequested = false;
                selectedSuggestion = -1;

                if (outside) {
                    noticeVisible = true;
                    noticeChord = CT::PlayedChord{ identified.rootPc, identified.typeIndex };
                    noticeKeyPc = oldKeyPc;
                    noticeTime = now;
                } else if (noticeVisible && oldKeyPc >= 0 &&
                           CT::isDiatonic(oldKeyPc, identified.rootPc, identified.typeIndex)) {
                    noticeVisible = false;  // back in the key
                }
            }
            if (identified.found && learnerMode) {
                checkPractice(identified, now);
            }
            currentChord = identified;
            keyPc = CT::detectKey(history);
        }

        pausedFor = now - lastChangeTime;

        if (!practiceMsg.empty() && now - practiceTime > 10.0) {
            practiceMsg.clear();
        }
        if (noticeVisible && now - noticeTime > 14.0) {
            noticeVisible = false;  // the note fades by itself
        }

        // Learner mode only: ideas after a pause (if on) or when asked for.
        if (learnerMode && !suggestionsVisible) {
            bool byPause = autoSuggest && hasPlayed && pausedFor >= idleSeconds;
            if (byPause || suggestRequested) {
                buildSuggestions();
                suggestedByPause = byPause && !suggestRequested;
            }
        }
    }

    void requestIdeas() { suggestRequested = true; }
    void dismissNotice() { noticeVisible = false; }

    // "Start fresh here": forget the old chords, work out the key from this one.
    void startFreshHere() {
        history.clear();
        history.push_back(noticeChord);
        keyPc = CT::detectKey(history);
        noticeVisible = false;
    }

    // The player pressed a "Show" button.
    void startPractice(int rootPc, int typeIndex, int inversion, double now) {
        highlightType = typeIndex;
        highlightRootPc = rootPc;
        highlightInversion = inversion;
        practiceMsg = "Go ahead and play " + CT::chordName(rootPc, typeIndex) + ", " +
                      CT::inversionName(inversion) + ". Take your time!";
        practiceMood = 0;
        practiceTime = now;
    }

    void clearPractice() {
        highlightType = -1;
        highlightRootPc = -1;
        practiceMsg.clear();
    }

    // Switching between Learner and Player mode starts the teaching afresh.
    void resetTeaching(double now) {
        suggestionsVisible = false;
        suggestRequested = false;
        selectedSuggestion = -1;
        feedback.clear();
        noticeVisible = false;
        highlightType = -1;
        highlightRootPc = -1;
        lastChangeTime = now;
    }

private:
    void buildSuggestions() {
        suggestions.clear();
        bridgeText.clear();
        if (!history.empty() && keyPc >= 0) {
            const CT::PlayedChord& last = history.back();
            ideasKeyPc = ideasKeyFor(keyPc, last);
            if (!CT::isDiatonic(keyPc, last.rootPc, last.typeIndex)) {
                bridgeText = CT::bridgeMessage(keyPc, last.rootPc, last.typeIndex);
            }
            suggestions = CT::suggestNext(ideasKeyPc, last.rootPc, last.typeIndex);
        } else {
            ideasKeyPc = 0;
            suggestions = starterChords();
        }
        suggestionsVisible = true;
        selectedSuggestion = -1;
        feedback.clear();
    }

    // A friendly reply when a chord is played while ideas were showing.
    std::string makeFeedback(int oldKeyPc, const CT::IdentifiedChord& played) const {
        std::string name = CT::chordName(played.rootPc, played.typeIndex);
        for (size_t i = 0; i < suggestions.size(); i++) {
            if (suggestions[i].rootPc == played.rootPc && suggestions[i].typeIndex == played.typeIndex) {
                return "Nice! " + name + " was idea " + std::to_string(i + 1) + ". You are getting it!";
            }
        }
        if (oldKeyPc >= 0 && CT::isDiatonic(oldKeyPc, played.rootPc, played.typeIndex)) {
            return "Oh, " + name + "! Not on the list, but it fits the key. Love the detour.";
        }
        if (oldKeyPc >= 0) {
            return "Ooh, " + name + " steps outside the key. Bold! That adds color, keep going.";
        }
        return name + " is a fine choice. Play on!";
    }

    void checkPractice(const CT::IdentifiedChord& played, double now) {
        if (highlightType < 0 || highlightRootPc < 0) {
            return;
        }
        PracticeOutcome out = judgePractice(highlightRootPc, highlightType, highlightInversion, played);
        practiceMsg = out.message;
        practiceTime = now;
        if (out.result == PracticeResult::Correct) {
            practiceMood = 1;
            highlightType = -1;
            highlightRootPc = -1;
        } else if (out.result == PracticeResult::WrongInversion) {
            practiceMood = 2;           // still trying this chord: keep the glow
        } else {
            practiceMood = 2;           // moved on: let the glow go
            highlightType = -1;
            highlightRootPc = -1;
            movedOn = true;
        }
    }
};

// ----------------------------------------------------------------- the glow

struct Glow {
    std::vector<int> notes;      // piano keys drawn with a yellow glow
    std::string caption;
};

// Which keys glow yellow. The chord the player chose with a "Show" button
// always wins. Otherwise the selected idea (or the first idea) glows.
// Player mode has no glow at all.
inline Glow glowFor(const Session& s, const std::unordered_set<int>& pressed, bool learnerMode) {
    Glow glow;
    if (!learnerMode) {
        return glow;
    }

    int shown = s.selectedSuggestion;
    if (shown < 0 && s.suggestionsVisible && !s.suggestions.empty()) {
        shown = 0;
    }

    if (s.highlightType < 0 && s.suggestionsVisible && shown >= 0 &&
        shown < static_cast<int>(s.suggestions.size())) {
        const CT::Suggestion& idea = s.suggestions[shown];
        glow.notes = CT::chordNotes(idea.rootPc, idea.typeIndex, 0);
        glow.caption = "Glowing = " + CT::chordName(idea.rootPc, idea.typeIndex) + " (an idea, not a rule)";
    } else if (s.highlightType >= 0) {
        int root = 0;
        if (s.highlightRootPc >= 0) {
            root = s.highlightRootPc;
        } else if (!pressed.empty()) {
            root = CT::pitchClass(CT::lowestNote(pressed));
        } else if (!s.history.empty()) {
            root = s.history.back().rootPc;
        }
        glow.notes = CT::chordNotes(root, s.highlightType, s.highlightInversion);
        glow.caption = "Glowing = " + CT::chordName(root, s.highlightType) + ", " +
                       CT::inversionName(s.highlightInversion) + " (your pick)";
    }
    return glow;
}

}  // namespace Learning
