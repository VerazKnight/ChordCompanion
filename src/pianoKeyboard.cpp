#include "pianoKeyboard.hpp"

void PianoKeyboard::handleMIDI(double deltatime, std::vector<unsigned char> *message, void *userData) {
    // Some messages contain less than 3 parts, and if they do, we can ignore them because they are not a keypress event
    if (!message || message->size() < 3) return;
    unsigned int nBytes = message->size();
    // The message has 3 parts:
    // The first byte appears to be either 144, signaling key pressed, or 128, signaling key release
    // The second byte is the midi key number
    // The third byte is the "velocity" v with witch the key was pressed, with v > 0 signaling a key press, and v = 0 signaling key release
    // We will be using the second and third bytes and ignoring the first
    std::string press_state = "";
    if ((int)message->at(2) != 0) {
        press_state = " Pressed";
    } else {
        press_state = " Released";
    }
    std::cout << "Midi Key: " << (int)message->at(1) << press_state << std::endl;
}

void PianoKeyboard::setup() {
    PianoKeyboard::pressedKeys.empty();

    try {
        PianoKeyboard::midiDevice = new RtMidiIn();
    }
    catch ( RtMidiError &error ) {
        error.printMessage();
        exit( EXIT_FAILURE );
    }
    
    // Check available ports.
    unsigned int nPorts = PianoKeyboard::midiDevice->getPortCount();
    std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
    if (nPorts == 0) {
        std::cerr << "No MIDI ports available" << std::endl;
    }
    
    for ( unsigned int i=0; i<nPorts; i++ ) {
        try {
            std::cout << "  Input Port #" << i + 1 << ": " << PianoKeyboard::midiDevice->getPortName(i) << '\n';;
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

    PianoKeyboard::midiDevice->openPort(port);
        
    // Set our callback function.  This should be done immediately after
    // opening the port to avoid having incoming messages written to the queue.
    PianoKeyboard::midiDevice->setCallback(&PianoKeyboard::handleMIDI);
    
    // Don't ignore sysex, timing, or active sensing messages.
    PianoKeyboard::midiDevice->ignoreTypes(false, false, false);
}

void PianoKeyboard::setPressedKeys(std::span<const int> keys) {
    for (int i = 0; i < keys.size(); i++) {
        PianoKeyboard::pressedKeys.insert(keys[i]);
    }
}

void PianoKeyboard::releasedPressedKeys(std::span<const int> keys) {
    for (int i = 0; i < keys.size(); i++) {
        PianoKeyboard::pressedKeys.erase(keys[i]);
    }
}

std::unordered_set<int> PianoKeyboard::getPressedKeys() {
    return PianoKeyboard::pressedKeys;
}