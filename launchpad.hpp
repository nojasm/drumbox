#pragma once

#include <vector>
#include "RtMidi.h"

using std::vector;

// The top row buttons of the launchpad.
enum class LaunchpadTopButton {
    UP = 0, DOWN, LEFT, RIGHT, SESSION, USER1, USER2, MIXER
};

// The state of a button on the grid. Can be pressed or not
struct LaunchpadGridButtonState {
    int x, y;
    bool pressed;
};

// The state of a top button on the launchpad. Can be pressed or not
struct LaunchpadTopButtonState {
    LaunchpadTopButton button;
    bool pressed;
};

// The state of a side row button on the launchpad. Can be pressed or not.
// 0 is the lowest button, 7 the highest
struct LaunchpadRowButtonState {
    int rowIndex;
    bool pressed;
};


class Launchpad {
private:
    RtMidiIn* midiIn;
    RtMidiOut* midiOut;

public:
    vector<LaunchpadGridButtonState> gridQueue;
    vector<LaunchpadTopButtonState> topQueue;
    vector<LaunchpadRowButtonState> rowQueue;

    int gridLightQueue[8 * 8];
    int gridCachedLightQueue[8 * 8];

    int topLightQueue[8];
    int topCachedLightQueue[8];

    int rowLightQueue[8];
    int rowCachedLightQueue[8];

    Launchpad(int);
    
    void resetGridLights();
    void resetTopLights();
    void resetRowLights();

    void setGridLight(int, int, unsigned char);
    void setTopLight(LaunchpadTopButton, int);
    void setRowLight(int, int);
    void updateLights(bool);
};

/**
 * This helper function gets called by RtMidi and processes any incoming events
*/
void launchpadEventHandler(double deltaTime, vector<unsigned char>* message, void *userData) {
    Launchpad* lp = (Launchpad*)userData;
    
    vector<unsigned char> data = *message;
    //printf("%d, %d, %d\n", (int)data[0], (int)data[1], (int)data[2]);

    // Grid or row button
    if (data[0] == 144) {

        // Row button
        if (data[1] % 10 == 9) {
            LaunchpadRowButtonState rbs;
            rbs.rowIndex = (data[1] - 19) / 10;
            rbs.pressed = data[2] == 127;
            
            
            lp->rowQueue.push_back(rbs);

        // Grid button
        } else {
            int x = data[1] % 10 - 1;
            int y = (data[1] - 11) / 10;

            LaunchpadGridButtonState gbs;
            gbs.x = x;
            gbs.y = 7 - y;  // Flip origin to be at the bottom left instead of the top left
            gbs.pressed = data[2] == 127;

            lp->gridQueue.push_back(gbs);
        }

    // Top row button
    } else if (data[0] == 176) {
        LaunchpadTopButton tb = LaunchpadTopButton(data[1] - 104);
        LaunchpadTopButtonState tbs;
        tbs.button = tb;
        tbs.pressed = data[2] == 127;

        lp->topQueue.push_back(tbs);
    }
}

Launchpad::Launchpad(int portNumber) {
    midiIn = new RtMidiIn();
    midiIn->openPort(portNumber);
    midiIn->ignoreTypes(false, false, false);
    midiIn->setCallback(&launchpadEventHandler, this);

    midiOut = new RtMidiOut();
    midiOut->openPort(portNumber);

    for (int i = 0; i < (8 * 8); i++)
        this->gridLightQueue[i] = -1;
    
    for (int i = 0; i < 8; i++)
        this->topLightQueue[i] = -1;
    
    for (int i = 0; i < 8; i++)
        this->rowLightQueue[i] = -1;
}

void Launchpad::setGridLight(int x, int y, unsigned char color) {
    this->gridLightQueue[y * 8 + x] = color;
}

void Launchpad::setRowLight(int rowIndex, int color) {
    this->rowLightQueue[rowIndex] = color;
}

void Launchpad::setTopLight(LaunchpadTopButton btn, int color) {
    this->topLightQueue[(int)btn] = color;
}

/*
 * Uploads the light queue to the launchpad
*/
void Launchpad::updateLights(bool updateAll = false) {
    // Grid lights
    for (int i = 0; i < (8 * 8); i++) {
        //printf("%d\n", i);
        if (this->gridCachedLightQueue[i] != this->gridLightQueue[i] || updateAll) {
            int x = i % 8;
            int y = 7 - i / 8;  // Flip origin to be at the bottom left instead of the top left

            vector<unsigned char> data;
            data.push_back((unsigned char)144);
            data.push_back((unsigned char)(x + (y * 10) + 11));
            data.push_back((unsigned char)this->gridLightQueue[i]);
            
            this->midiOut->sendMessage(&data);
            this->gridCachedLightQueue[i] = this->gridLightQueue[i];
        }
    }

    // Row lights
    for (int i = 0; i < 8; i++) {
        if (this->rowCachedLightQueue[i] != this->rowLightQueue[i] || updateAll) {
            vector<unsigned char>* data = new vector<unsigned char>();
            data->push_back((unsigned char)144);
            data->push_back((unsigned char)19 + (10 * i));
            data->push_back((unsigned char)this->rowLightQueue[i]);
            
            this->midiOut->sendMessage(data);
            this->rowCachedLightQueue[i] = this->rowLightQueue[i];
        }
    }

    // Top lights
    for (int i = 0; i < 8; i++) {
        if (this->topCachedLightQueue[i] != this->topLightQueue[i] || updateAll) {
            //printf("  %d, %d = %d\n", x, y, this->gridLightQueue[i]);

            vector<unsigned char>* data = new vector<unsigned char>();
            data->push_back((unsigned char)176);
            data->push_back((unsigned char)104 + i);
            data->push_back((unsigned char)this->topLightQueue[i]);
            
            this->midiOut->sendMessage(data);
            this->topCachedLightQueue[i] = this->topLightQueue[i];
        }
    }
}

void Launchpad::resetGridLights() {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            this->setGridLight(x, y, 0);
        }
    }
}

void Launchpad::resetTopLights() {
    for (int i = 0; i < 8; i++) {
        this->setRowLight(i, 0);
    }
}

void Launchpad::resetRowLights() {
    for (int i = 0; i < 8; i++) {
        this->setTopLight((LaunchpadTopButton)i, 0);
    }
}