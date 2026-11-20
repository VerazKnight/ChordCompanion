#include "pianoKeyboard.hpp"

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
        midiDevice = new RtMidiIn();
    }
    catch (RtMidiError &error) {
        error.printMessage();
        exit(EXIT_FAILURE);
    }
    
    // Check available ports.
    unsigned int nPorts = midiDevice->getPortCount();
    std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
    if (nPorts == 0) {
        std::cerr << "No MIDI ports available" << std::endl;
        return;
    }
    
    for ( unsigned int i=0; i<nPorts; i++ ) {
        try {
            std::cout << "  Input Port #" << i + 1 << ": " << midiDevice->getPortName(i) << '\n';;
        }
        catch ( RtMidiError &error ) {
            error.printMessage();
            return;
        }
    }

    unsigned int port = 0;

    if (nPorts > 1) {
        while (true) {
            std::cout << "Select MIDI Device [1-" << (nPorts) << "]: ";
            std::cin >> port;
            if (std::cin.eof()) {
                return;
            }

            if (port >= 1 && port <= nPorts) {
                port -= 1;
                break;
            }
            
            std::cout << "Please select a valid device" << std::endl;
            std::cin.clear();
            std::cin.ignore(100000, '\n');
        }
    }

    midiDevice->openPort(port);
        
    // Set our callback function.  This should be done immediately after
    // opening the port to avoid having incoming messages written to the queue.
    midiDevice->setCallback(&PianoKeyboard::handleMIDI, this);
    
    // Don't ignore sysex, timing, or active sensing messages.
    midiDevice->ignoreTypes(false, false, false);
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
