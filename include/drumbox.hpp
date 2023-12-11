#include <vector>
#include <string>
#include <chrono>
#include <map>
#include "teVirtualMIDI.h"

using std::vector;
using std::string;

struct Note {
    int key;
    double velocity;
};

enum KeyState {
    NONE,
    SOLO,
    MUTE
};

enum Mode {
    SEQUENCER,
    INSTRUMENT
};

class DrumBox {
public:
    LPVM_MIDI_PORT midi;

    int currentStep = 0;
    int totalSteps = 0;
    int currentSequencerPage = 0;
    int useSequencerPages = 1;  // 1/2/4 - Number of sequencer pages to actually play
    int keyOffsetC = 36;  // Ableton index for c2
    int channel = 3;
    int bpm = 120;

    Mode currentMode = Mode::INSTRUMENT;

    int currentNoteKey = -1;
    std::map<int, double> keyVelocities;  // Remember velocity for every sample
    std::map<int, int> keyPresses;  // Keys that are currently pressed in instrument mode. Second int can be -1 or from 0 to 7 indicating the y position of the button that was pressed
    std::map<int, KeyState> keyStates;  // true if a key should be muted or solo'd
    bool shift = false;
    bool mixerButtonDown = false;

    int keyPressSteps = 1;
    // This map contains all keys that were pressed plus a counter. Every step, the counter increases.
    // If the counter is > keyPressSteps, the NOTE-OFF signal is sent and the key is removed from the map.
    std::map<int, int> keyPressTimeout;

    bool isPlaying = false;
    unsigned long quarterCounter = 0;
    std::chrono::high_resolution_clock::time_point playStartTime = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::duration latency = std::chrono::milliseconds(20);

    vector<vector<Note>> sequenceSteps;  // step[samples[sample]]
    int stepsPerBeat = 4;
    
    DrumBox();
    void midiEventCallback(vector<unsigned char> message);
    void sendMessageToHost(vector<unsigned char> message);
    void gotoNextSequencerStep();
    void playSequencerStep();
    void sendNote(Note note);
    void sendReleaseKey(unsigned char key);
    void close();
};
