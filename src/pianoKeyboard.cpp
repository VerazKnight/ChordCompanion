#include "pianoKeyboard.hpp"

#include <memory>

PianoKeyboard::PianoKeyboard() {
    setup();
}

PianoKeyboard::~PianoKeyboard() {
    delete midiDevice;
}

void PianoKeyboard::handleMIDI(double deltatime, std::vector<unsigned char> *message, void *userData) {
    // Some messages contain less than 3 parts, and if they do, we can ignore them because they are not a keypress event
    if (!message || message->size() < 3 || !userData) return;
    // The message has 3 parts:
    // The first byte appears to be either 144(0x90), signaling key pressed, or 128(0x80), signaling key release
    // The second byte is the midi key number
    // The third byte is the "velocity" v with witch the key was pressed, with v > 0 signaling a key press, and v = 0 signaling key release
    
    auto* keyboard = static_cast<PianoKeyboard*>(userData);
    const int status = (*message)[0] & 0xF0;
    const int channel = (*message)[0] & 0x0F;
    const int note = (*message)[1];
    const int velocity = (*message)[2];
    if (status != 0x90 && status != 0x80) return;

    // RtMidi invokes this on its own thread. Serialize state and audio calls.
    try {
        std::lock_guard lock(keyboard->stateMutex);
        keyboard->playEvents(keyboard->notes.setMidi(
            channel, note, status == 0x80 ? 0 : velocity));
    } catch (const std::exception& error) {
        // Exceptions must not escape the MIDI callback thread.
        std::cerr << "MIDI audio error: " << error.what() << '\n';
    }
}

void PianoKeyboard::setup() {
    try {
        // Probe locally so every failed or skipped setup leaves midiDevice null.
        auto device = std::make_unique<RtMidiIn>();
        const unsigned int nPorts = device->getPortCount();
        std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
        if (nPorts == 0) {
            std::cerr << "No MIDI ports available; using mouse/QWERTY input.\n";
            return;
        }

        for (unsigned int i = 0; i < nPorts; ++i) {
            std::cout << "  Input Port #" << i + 1 << ": "
                      << device->getPortName(i) << '\n';
        }

        unsigned int port = 0;
        if (nPorts > 1) {
            while (true) {
                std::cout << "Select MIDI Device [1-" << nPorts << "]: ";
                std::cin >> port;
                if (std::cin.eof()) {
                    return;
                }
                if (!std::cin.fail() && port >= 1 && port <= nPorts) {
                    --port;
                    break;
                }
                std::cout << "Please select a valid device\n";
                std::cin.clear();
                std::cin.ignore(100000, '\n');
            }
        }

        device->openPort(port);
        device->setCallback(&PianoKeyboard::handleMIDI, this);
        device->ignoreTypes(false, false, false);

        // Only retain the device after setup completes successfully.
        midiDevice = device.release();
    } catch (const RtMidiError& error) {
        error.printMessage();
        std::cerr << "MIDI input unavailable; using mouse/QWERTY input.\n";
    }
}

void PianoKeyboard::playEvents(const std::vector<NoteState::Event>& events) {
    for (const auto& event : events) {
        if (event.velocity != 0) {
            sound.noteOn(event.channel, event.note, event.velocity);
        } else {
            sound.noteOff(event.channel, event.note);
        }
    }
}

void PianoKeyboard::setUiPressedKeys(const std::unordered_set<int>& keys) {
    std::lock_guard lock(stateMutex);
    playEvents(notes.setUi(keys));
}

std::unordered_set<int> PianoKeyboard::getPressedKeys() {
    std::lock_guard lock(stateMutex);
    return notes.pressedKeys();
}
