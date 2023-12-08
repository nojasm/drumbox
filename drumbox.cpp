#include <vector>
#include <string>
#include "launchpad.hpp"
#include "RtMidi.h"

using std::vector;
using std::string;

void midiInCallback(double deltaTime, vector<unsigned char>* message, void* userData) {
    printf("=== Midi Message ===\n");
    for (int i = 0; i < message->size(); i++) {
        printf("Byte #%d = %d\n", i, (*message)[i]);
    }
}

enum Mode {
    SEQUENCER,
    INSTRUMENT
};

class Sample {
public:
    string path;
    Sample(const char* path) {
        this->path = path;
    }
};

struct Note {
    int key;
    double velocity;
};

class DrumBox {
public:
    RtMidiIn midiIn;
    RtMidiOut midiOut;

    int currentStep = 0;
    int currentSequencerPage = 0;
    int nSequencerPages = 1;
    int currentSampleIndex = 0;

    Mode currentMode = Mode::SEQUENCER;

    vector<Sample> samplePack;
    vector<vector<Note>> sequenceSteps;  // step[samples[sample]]
    int stepsPerBeat = 4;
    
    DrumBox() {
        try
        {
            midiIn.openVirtualPort("DrumBox Virtual Device In");
            midiOut.openVirtualPort("DrumBox Virtual Device Out");

            midiIn.setCallback(midiInCallback, this);
            midiIn.ignoreTypes(false, false, false);
        } catch(const std::exception& e) {
            std::cerr << e.what() << '\n';
        }
    }

    void nextSequencerStep() {
        if (currentStep + 1 >= (8 * 8)) currentStep = 0;
        else currentStep++;

        vector<Note>* stepNotes = &sequenceSteps[currentStep];
        for (int i = 0; i < stepNotes->size(); i++) {
            this->sendNote(&stepNotes->at(i));
        }
    }

    void sendNote(Note* note) {
        // TOOD
    }

    void close() {
        midiIn.closePort();
        midiOut.closePort();
    }
};

int main() {
    DrumBox* db = new DrumBox();
    while (true) {
        break;
    }

    return 0;
}

/*

    TODOs:
    - autofill option, so that you can set 4 hihats and hit autofill and it automatically fills the rest of the pattern

    - Use the SESSION button as a shift button
    
    USER 1
        Switch between instrument and sequencer
    
    SESSION
        acts as a shift button
    
    UP, DOWN, LEFT, RIGHT
        Select one of four pages for the sequencer.
        With SHIFT, you can set how many pages should be used.
    
    MIXER
        - The arrow buttons light up. If you have 1 or 2 pages selected,
          you can select 2 or 4 to select more pages and also copy the existing
          ones to that new pages
        - You can press on one of the side buttons to select a velocity
            - You will then place all new samples of the current sample
              in that velocity
            - If you also press shift, you can 
        - On the grid, four lights per row/sample light up
            - On the left of each row that has a sample, there are two buttons for soloing.
                -> Press the first for soloing it
                -> Hold the second to solo it while holding
            - Same goes for the right with muting
    Side buttons 1 - 8
        Switch which sample you are currently editing

    When pressing SHIFT, :
        
*/