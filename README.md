# drumbox
A program to use your Novation Launchpad as a drum sequencer and much more

# Usage
Currently there is no GUI. You can just double click drumbox.exe to start the program.

# Common errors
## Error: Launchpad not found on one or more ports
The software failed to connect to the launchpad. Check if Ableton has connected first. If so, remove your Launchpad from the MIDI tab in the settings. Also check if your launchpad is connected correctly.

## "failed." after entering BPM
The BPM you entered was probably not valid. Make sure you enter a valid BPM between 0 and 200.

# Buttons
| Button | Feature |
| - | - |
| UP | Page 1 |
| DOWN | Page 2 |
| LEFT | Page 3 |
| RIGHT | Page 4 |
| SESSION | Shift |
| USER 1 | Sequencer mode |
| USER 2 | Instrument mode |
| MIXER | Open mixer page |

Use the shift (session) button to use the secondary buttons:
| Button | Secondary feature |
| - | - |
| UP | Activate page 1 |
| DOWN | Activate page 2 |
| RIGHT | Activate page 4 |
| MIXER | Copy current page to all other pages |

**Note:** You can't activate page 3 as looping 3 pages is quite useless

# Samples
Using the buttons on the right, you can select which sample you are currently working with. As DRUMBOX just acts as a MIDI controller, sample #1 sends the midi signal for note 36 (C1). Sample #2 is note 37 and so on. To change this offset, edit "keyOffsetC" inside drumbox.hpp.

# Sequencer Mode
This is the mode you probably want. The sequencer has 4 pages. You can change the page you are currently working on using the arrow buttons UP, DOWN, LEFT and RIGHT. By default, only the first page is active. You can activate other pages by pressing SHIFT + *arrow button*. You can select the current sample using one of the buttons on the right. Selecting a sample that is already selected will get you to an overview page, where you can see all samples that are on the current page. Notice that the sample buttons light up when they are playing. In this mode you also can't place any samples.

To place samples in a different velocity, click MIXER and select the velocity using one of the buttons on the right. Whenever you place the current sample, it will be placed in that new velocity. Sadly, you can't yet see which placed sample has which velocity.

The sequencer only starts as soon as you click *play* in Ableton.

# Instrument Mode
In the instrument mode you can play samples live. On the bottom you see 8 samples lit up. To play in different velocities, you can press above them. The further you go up, the quieter the samples become.

# Customizing colors
Currently, customizing colors requires editing the source code and recompiling the whole project. In `drumbox.cpp` there are *#define LIGHT_...* statements that you can edit. Light values go from 0 to 127. Have fun figuring out the different colors.

# Compiling the project
To compile the source code, you can use the following command:
```sh
g++ -D __WINDOWS_MM__ -g -Iinclude src\RtMidi.cpp src\drumbox.cpp -lwinmm lib\teVirtualMIDI64.lib lib\librtmidi.lib -o build\drumbox.exe
```

# Libraries
DrumBox uses rtMidi to communicate with the launchpad and teVirtualMIDI to act as a virtual midi device that can be used in your DAW.