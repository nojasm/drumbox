#include <vector>
#include <string>
#include <iostream>
#include <cstdint>
#include <chrono>
#include <thread>
#include <map>

#include "launchpad.hpp"

#include "drumbox.hpp"
#include "teVirtualMIDI.h"
//#include "RtMidi.h"

using std::vector;
using std::string;

#define LIGHT_CURSOR 41
#define LIGHT_LOW 1
#define LIGHT_BRIGHT 115
#define LIGHT_SELECTED 110
#define LIGHT_INSTRUMENT_PLAY 99

void midiInCallback(LPVM_MIDI_PORT midiPort, LPBYTE midiDataBytes, DWORD length, DWORD_PTR dwCallbackInstance) {
    DrumBox* instance = (DrumBox*)dwCallbackInstance;

    if (midiDataBytes[0] == 248) {
        instance->quarterCounter++;
        return;
    }

    //printf("<--- [");
    vector<unsigned char> message;
    for (int i = 0; i < length; i++) {
        message.push_back(midiDataBytes[i]);

        //printf("%d", midiDataBytes[i]);

        //if (i != length - 1)
        //    printf(", ");
    }

    //printf("]\n");

    instance->midiEventCallback(message);
}

DrumBox::DrumBox() {
    // Create virtual midi device
    midi = virtualMIDICreatePortEx2(L"DRUMBOX", (LPVM_MIDI_DATA_CB)(&midiInCallback), (DWORD_PTR)this, 0, TE_VM_FLAGS_PARSE_RX | TE_VM_FLAGS_PARSE_TX);

    DWORD d = GetLastError();
    if (d != 0)
        printf("lastError: %d\n", d);    

    // Initialize sequencer
    sequenceSteps.resize(8 * 8 * 4);  // 4 sequencer pages with 8x8 cells

    // Initialize all velocities and key states
    for (int i = 0; i < 8; i++) {
        keyVelocities[i + this->keyOffsetC] = 1.0;
        keyStates[i + this->keyOffsetC] = KeyState::NONE;
        keyPresses[i + this->keyOffsetC] = -1;
    }

    // Examples for default sequencer patters
    /*Note kick;
    kick.key = this->keyOffsetC;
    kick.velocity = 1.0;

    Note hihat;
    hihat.key = this->keyOffsetC + 1;
    hihat.velocity = 1.0;*/

    // Place a simple pattern on the sequencer
    /*for (int i = 0; i < (8 * 8); i++) {
        if (i % 8 == 0)
            sequenceSteps[i].push_back(kick);
        //else if (i % 4 == 2)
        //    sequenceSteps[i].push_back(hihat);
    }*/
}

void DrumBox::midiEventCallback(vector<unsigned char> message) {
    if (message.size() == 1) {
        if (message[0] == 250) {
            // Play event
            isPlaying = true;
            playStartTime = std::chrono::high_resolution_clock::now();
            currentStep = 0;
            totalSteps = 0;
            printf("{PLAY}\n");
        
        } else if (message[0] == 248) {
            // Quarter note clock event
        
        } else if (message[0] == 252) {
            // Stop event
            isPlaying = false;
            printf("{STOP}\n");
        } else {
            printf("weird event %d\n", message[0]);
        }
    }
}

/**
 * Increment cursor by one. Reset the cursor if it is outside of the
 * active pages
*/
void DrumBox::gotoNextSequencerStep() {
    if (currentStep + 1 >= (8 * 8 * useSequencerPages)) currentStep = 0;
    else currentStep++;

    totalSteps++;
}

/**
 * Go through all samples of the current sequencer step. Send MIDI signals
 * to play a sample in a certain velocity. Also releases older keys again that
 * were pressed before.
*/
void DrumBox::playSequencerStep() {
    vector<Note>* stepNotes = &sequenceSteps[currentStep];

    // Check if any of the samples are solo'd
    bool anySampleSolo = false;
    for (int i = 0; i < 8; i++) {
        if (this->keyStates[i + this->keyOffsetC] == KeyState::SOLO) {
            anySampleSolo = true;
            break;
        }
    }

    //printf("---> [");
    for (int i = 0; i < stepNotes->size(); i++) {
        // If the note is currently played, release it first
        int key = stepNotes->at(i).key;
        if ((anySampleSolo && this->keyStates[key] == KeyState::SOLO) || (!anySampleSolo && this->keyStates[key] == KeyState::NONE)) {
            if (keyPressTimeout.find(key) != keyPressTimeout.end()) {
                sendReleaseKey(key);
                keyPressTimeout.erase(keyPressTimeout.find(key));
            }
            
            // Send midi signal to play the note
            this->sendNote(stepNotes->at(i));
            keyPressTimeout[key] = 0;
        }
        
        //Note n = stepNotes->at(i);
        //printf("Note(%d, %.2f)", n.key, n.velocity);
        //if (i != stepNotes->size() - 1)
        //    printf(", ");
    }
    //printf("]\n");

    // Check if any keys should be released
    for (std::pair<int, int> key : keyPressTimeout) {
        if (key.second >= keyPressSteps) {
            keyPressTimeout[key.first] = -1;
            sendReleaseKey(key.first);
        } else if (key.second != -1) {
            keyPressTimeout[key.first]++;
        }
    }
}
 
void DrumBox::sendMessageToHost(vector<unsigned char> message) {
    if (message.size() == 0)
        printf("sendMessageToHost received empty message parameter\n");
    
    unsigned char* data = (unsigned char*) malloc(sizeof(unsigned char) * message.size());
    for (int i = 0; i < message.size(); i++) {
        data[i] = message[i];
    }

    if (!virtualMIDISendData(midi, data, message.size())) {
        printf("Message could not be sent.\n");
    }
}

void DrumBox::sendNote(Note note) {
    vector<unsigned char> msg;
    msg.push_back(0x90 + this->channel);
    msg.push_back(note.key);
    msg.push_back((unsigned char)(note.velocity * 127));
    
    this->sendMessageToHost(msg);
}

void DrumBox::sendReleaseKey(unsigned char key) {
    vector<unsigned char> msg;
    msg.push_back(0x80 + this->channel);
    msg.push_back(key);
    msg.push_back(127);

    this->sendMessageToHost(msg);
}

// TODO
void DrumBox::close() {
    //midiIn->closePort();
    //midiOut->closePort();
}

// Some random conversion so keys like 50 and 51 have completely different colors
int keyToColorIndex(int key) {
    return (key * 25 + 17) % 128;
}

int main(int argc, char** argv) {
    // ignore this
    std::string logo =
    #include "logo.hpp"
    ;
    
    printf(logo.c_str());

    printf("https://github.com/nojasm/drumbox\n");


    DrumBox* db = new DrumBox();
    
    printf("Enter BPM: ");
    db->bpm = -1;
    scanf("%d", &db->bpm);

    if (db->bpm <= 0 || db->bpm > 200) {
        printf("failed.\n");
        exit(-1);
    }

    printf("-----\n");
    
    Launchpad* lp = new Launchpad();
    printf("e after lp %d\n", GetLastError());
    //printf("LaunchPad connected at #%d: %s / %s\n", lpPort, lp->midiIn->getPortName(lpPort).c_str(), lp->midiOut->getPortName(lpPort).c_str());
    if (!lp->midiIn->isPortOpen())
        printf("  jk its not connected, lol (the in one)\n");
    if (!lp->midiOut->isPortOpen())
        printf("  jk its not connected, lol (the out one)\n");

    lp->resetGridLights();
    lp->updateLights(true);

    db->currentNoteKey = db->keyOffsetC;
    std::chrono::high_resolution_clock::time_point tnow;
    std::chrono::high_resolution_clock::time_point lastStepTime;
    std::chrono::high_resolution_clock::duration timePerStep = std::chrono::microseconds((int)(60.0 / db->bpm / 4.0 * 1000 * 1000));
    while (true) {
        tnow = std::chrono::high_resolution_clock::now();

        if (db->isPlaying) {
            // Check if it is time for the NEXT STEP!
            if (db->totalSteps == 0 || tnow > (lastStepTime + timePerStep)) {
                lastStepTime = tnow;

                // Play next sample step
                db->playSequencerStep();
                db->gotoNextSequencerStep();
            }
        }

        lp->resetTopLights();
        lp->resetRowLights();
        lp->resetGridLights();

        // Top button events
        while (lp->topQueue.size()) {
            LaunchpadTopButtonState topButton = lp->topQueue.back();
            lp->topQueue.pop_back();

            // Shift button
            if (topButton.button == LaunchpadTopButton::SESSION)
                db->shift = topButton.pressed;
            
            // Select page
            if (topButton.button == LaunchpadTopButton::UP || topButton.button == LaunchpadTopButton::DOWN || topButton.button == LaunchpadTopButton::LEFT || topButton.button == LaunchpadTopButton::RIGHT) {
                int index = (int)topButton.button;
                if (db->shift) {
                    // Select how many pages should be used
                    // 3 sequencer pages does not make sense in a looping drum computer
                    if (index == 0) db->useSequencerPages = 1;
                    else if (index == 1) db->useSequencerPages = 2;
                    else if (index == 3) db->useSequencerPages = 4;
                } else {
                    // Switch pages
                    db->currentSequencerPage = index;
                }
            } else if (topButton.button == LaunchpadTopButton::MIXER) {
                if (db->shift) {
                    // TODO: If a sample is selected, only copy that one to all other pages
                    // Copy current page to all other pages
                    for (int page = 0; page < 4; page++) {
                        if (page == db->currentSequencerPage) continue;

                        for (int i = 0; i < (8 * 8); i++) {
                            db->sequenceSteps[(page * 8 * 8) + i] = db->sequenceSteps[(db->currentSequencerPage * 8 * 8) + i];
                        }
                    }
                } else if (topButton.pressed) {
                    // Toggle mixer page. This can also be changed to
                    // | db->mixerButtonDown = topButton.pressed |
                    // to open the page while MIXER is pressed, instead of toggling it
                    // TODO: Put this inside some kind of configuration file
                    db->mixerButtonDown = !db->mixerButtonDown;
                }
            } else if (topButton.button == LaunchpadTopButton::USER1) {
                // Change current mode to sequencer
                db->currentMode = Mode::SEQUENCER;
            } else if (topButton.button == LaunchpadTopButton::USER2) {
                // Change current mode to instrument
                db->currentMode = Mode::INSTRUMENT;
            }
        }

        // Grid lights
        if (db->mixerButtonDown) {
            // Mixer page
            for (int i = 0; i < 8; i++) {
                KeyState ks = db->keyStates[(7 - i) + db->keyOffsetC];
                // Mute buttons
                lp->setGridLight(0, i, ks == KeyState::MUTE ? 6 : 2);
                lp->setGridLight(1, i, ks == KeyState::MUTE ? 6 : 2);

                // Solo buttons
                lp->setGridLight(6, i, ks == KeyState::SOLO ? 6 : 2);
                lp->setGridLight(7, i, ks == KeyState::SOLO ? 6 : 2);
            }

            lp->setTopLight(LaunchpadTopButton::MIXER, LIGHT_SELECTED);

        } else if (db->currentMode == Mode::SEQUENCER) {
            // Sequencer page
            for (int i = 0; i < (8 * 8); i++) {
                int cellIndex = i + (db->currentSequencerPage * 8 * 8);
                if (db->currentNoteKey == -1) {
                    // No sample selected, so light up all (In color of first sample)
                    lp->setGridLight(i % 8, i / 8, db->sequenceSteps[cellIndex].size() ? keyToColorIndex(db->sequenceSteps[cellIndex][0].key) : 0);
                } else {
                    lp->setGridLight(i % 8, i / 8, 0);

                    // Check if the step has the selected sample in it
                    for (int sampleIndex = 0; sampleIndex < db->sequenceSteps[cellIndex].size(); sampleIndex++) {
                        // If so, light the cell up
                        if (db->sequenceSteps[cellIndex][sampleIndex].key == db->currentNoteKey) {
                            lp->setGridLight(i % 8, i / 8, keyToColorIndex(db->currentNoteKey));
                        }
                    }
                }
            }
        } else if (db->currentMode == Mode::INSTRUMENT) {
            // Instrument page
            for (int x = 0; x < 8; x++) {
                // Light up samples at the bottom. If a sample is pressed, light it
                // up where it is pressed instead.
                if (db->keyPresses[x + db->keyOffsetC] == -1)
                    lp->setGridLight(x, 7, LIGHT_BRIGHT);
                else
                    lp->setGridLight(x, db->keyPresses[x + db->keyOffsetC], LIGHT_INSTRUMENT_PLAY);
            }
        }
        
        // Current page lights
        for (int i = 0; i < 4; i++) {
            if (db->shift) {
                if (i != 2)  // Don't light up page 3 - looping 3 pages is stupid
                    lp->setTopLight((LaunchpadTopButton)i, 110);
                
            } else {
                int color = 0;
                // Page is currently playing
                if (db->isPlaying && (i * 8 * 8) <= db->currentStep && db->currentStep < ((i + 1) * 8 * 8)) color = LIGHT_CURSOR;

                // Page is selected
                else if (i == db->currentSequencerPage) color = LIGHT_SELECTED;

                // Page is activate and will be played
                else if (i < db->useSequencerPages) color = LIGHT_LOW;
                
                lp->setTopLight((LaunchpadTopButton)i, color);
            }
        }
        
        // Cursor light
        //if (db->isPlaying && (8 * 8 * db->currentSequencerPage) <= db->currentStep && db->currentStep < (8 * 8 * (db->currentSequencerPage + 1))) {
        if (db->currentMode == Mode::SEQUENCER && db->isPlaying && (db->currentStep / (8 * 8)) <= db->currentSequencerPage && db->currentSequencerPage < (db->currentStep / (8 * 8) + 1)) {
            lp->setGridLight(db->currentStep % 8, (db->currentStep % (8 * 8)) / 8, LIGHT_CURSOR);
        }

        // Row button events
        while (lp->rowQueue.size()) {
            LaunchpadRowButtonState rowButton = lp->rowQueue.back();
            lp->rowQueue.pop_back();
            
            // Ignore event if button was released
            if (!rowButton.pressed || db->currentMode != Mode::SEQUENCER)
                continue;

            if (db->currentNoteKey != -1 && db->mixerButtonDown) {
                // Select velocity for current sample
                db->keyVelocities[db->currentNoteKey] = ((rowButton.rowIndex + 1) / 8.0);
            } else {
                // If the pressed sample is already select, go back to all samples
                if (db->currentNoteKey == db->keyOffsetC + rowButton.rowIndex) {
                    db->currentNoteKey = -1;
                } else {
                    db->currentNoteKey = db->keyOffsetC + rowButton.rowIndex;
                }
            }
        }

        // Grid button events
        while (lp->gridQueue.size()) {
            // Toggle sample at the pressed position
            LaunchpadGridButtonState* button = &lp->gridQueue.back();
            //printf("Button %dx%d (%d) event\n", button->x, button->y, button->pressed);
            lp->gridQueue.pop_back();
            int cellIndex = button->y * 8 + button->x;

            if (db->mixerButtonDown) {
                // Mixer screen for sample muting and soloing
                int key = (7 - button->y) + db->keyOffsetC;
                if (button->x == 0 && button->pressed) {
                    db->keyStates[key] = db->keyStates[key] == KeyState::MUTE ? KeyState::NONE : KeyState::MUTE;
                } else if (button->x == 1) {
                    db->keyStates[key] = button->pressed ? KeyState::MUTE : KeyState::NONE;
                } else if (button->x == 6 && button->pressed) {
                    db->keyStates[key] = db->keyStates[key] == KeyState::SOLO ? KeyState::NONE : KeyState::SOLO;
                } else if (button->x == 7) {
                    db->keyStates[key] = button->pressed ? KeyState::SOLO : KeyState::NONE;
                }

            // Ignore event if button was released
            // You can only place and remove samples if you have sample selected
            } else if (button->pressed && db->currentNoteKey != -1 && db->currentMode == Mode::SEQUENCER) {
                // Get all notes that are in the current sequencer step
                vector<Note>* thisCellNotes = &db->sequenceSteps[cellIndex + (db->currentSequencerPage * 8 * 8)];

                Note note;
                note.key = db->currentNoteKey;
                note.velocity = db->keyVelocities[db->currentNoteKey];

                // Check if the step has a sample already
                bool alreadyAdded = false;
                for (int sampleIndex = 0; sampleIndex < thisCellNotes->size(); sampleIndex++) {
                    if (thisCellNotes->at(sampleIndex).key == db->currentNoteKey) {
                        // Found the sample, now remove it!
                        thisCellNotes->erase(thisCellNotes->begin() + sampleIndex);
                        alreadyAdded = true;
                        break;
                    }
                }

                // If the sample is not yet there, place it!
                if (!alreadyAdded) {
                    thisCellNotes->push_back(note);
                }
            } else if (db->currentMode == Mode::INSTRUMENT) {
                db->keyPresses[button->x + db->keyOffsetC] = button->pressed ? button->y : -1;
                if (button->pressed) {
                    // Play pressed key instantly
                    Note note;
                    note.key = button->x + db->keyOffsetC;
                    note.velocity = (button->y + 1) / 8.0;  // Calculate velocity by using the y value
                    db->sendNote(note);
                } else {
                    // TODO: What happens if you play a key, then switch to SEQUENCE-MODE. Will the key be released or sth like that?
                    db->sendReleaseKey(button->x + db->keyOffsetC);
                }
            }
        }

        // Side button lights
        for (int i = 0; i < 8; i++)
            lp->setRowLight(i, (db->currentNoteKey - db->keyOffsetC) == i ? LIGHT_SELECTED : 0);
        
        // Light up sample on row if they are currently playing
        if (db->isPlaying && db->currentNoteKey == -1) {
            std::map<int, int>::iterator it;
            for (it = db->keyPressTimeout.begin(); it != db->keyPressTimeout.end(); it++) {
                if (it->second != -1) {
                    lp->setRowLight(it->first - db->keyOffsetC, keyToColorIndex(it->first));
                }
            }
        }

        // Current mode lights
        if (!db->mixerButtonDown && !db->shift) {
            lp->setTopLight(db->currentMode == Mode::SEQUENCER ? LaunchpadTopButton::USER1 : LaunchpadTopButton::USER2, LIGHT_SELECTED);
        }

        // Overwrite side button lights if MIXER is pressed
        // MIXER + SIDEBUTTON sets the velocity for the current selected sample
        if (db->currentNoteKey != -1 && db->mixerButtonDown && db->currentMode == Mode::SEQUENCER) {
            double vel = db->keyVelocities[db->currentNoteKey];
            for (int i = 0; i < 8; i++) {
                lp->setRowLight(i, ((i) / 7.0) <= vel ? 39 : 0);
            }
        }

        // Send changed light values to launchpad
        lp->updateLights();
    }

    return 0;
}

/*
    TODOs:
    - autofill option, so that you can set 4 hihats and hit autofill and it automatically fills the rest of the pattern
    - Very important! Add a sleep mode that automatically does light shows randomly based on the beat. After pressing on a button, go into sequencer mode again with no sample selected
*/