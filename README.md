| Supported Targets | ESP32 | ESP32-S3 |
| ----------------- | ----- | -------- |

# CCK_Recorder

## Info
This is for **speed cubing/sport stacks** record upload, which can be used in competition, festival.  
It would makes judges **easier, faster, more accurate** in working.

## Main functions
### Upload/Adjust record
- Adjust record penalty, DNF with several buttons
- Upload record, judge info to player's attempt data
- Integrity about player, judge's info

### Stackmat synchronization
- Just connect stackmat with AUX cable, then automatically detect, synchronize stackmat data.

### Inspection
- Just push a OK button, then start the inspection section
- Every alert events, audio announcement triggers(8s, 12s, 15s)
- Over 15s and less then 17s, penalty automatically added
- Over 17s, inspection section stop, give player DNF

### WiFi management
- If not connected, automatically goes to AP mode, and you can connect and go to configuration page.

### OTA(Over The Air) update
- Automatically detect new version, and update via WiFi



## Example folder contents

The project **sample_project** contains one source file in C language [main.c](main/main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
