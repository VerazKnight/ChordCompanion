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
    const int keys[] = {(*message)[1]};
    const auto velocity = (*message)[2];

    if (velocity != 0 && status == 0x90) {
        keyboard->setPressedKeys(keys);
        keyboard->sound.noteOn(channel, keys[0], velocity);
    } else if ((velocity == 0 && status == 0x90) || status == 0x80) {
        keyboard->releasePressedKeys(keys);
        keyboard->sound.noteOff(channel, keys[0]);
    }
}

void PianoKeyboard::setup() {
    pressedKeys.clear();

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

void PianoKeyboard::setPressedKeys(std::span<const int> keys) {
    for (int key : keys) {
        pressedKeys.insert(key);
    }
}

void PianoKeyboard::releasePressedKeys(std::span<const int> keys) {
    for (int i = 0; i < keys.size(); i++) {
        pressedKeys.erase(keys[i]);
    }
}

std::unordered_set<int> PianoKeyboard::getPressedKeys() {
    return pressedKeys;
}