//
//  ofxMidiFile.cpp
//  JALC_AudioAnalyser
//
//  Created by Andreas Borg on 19/05/2015.
//
//

#include "ofxMidiFile.h"


ofEvent<MidiFileEvent>	MidiFileEvent::NOTE_ON;
ofEvent<MidiFileEvent>	MidiFileEvent::NOTE_OFF;
ofEvent<MidiFileEvent>	MidiFileEvent::PLAY;
ofEvent<MidiFileEvent>	MidiFileEvent::STOP;
ofEvent<MidiFileEvent>	MidiFileEvent::BEAT;
ofEvent<MidiFileEvent>	MidiFileEvent::MTC;
ofEvent<MidiFileEvent>	MidiFileEvent::NOTES_ALL_OFF;