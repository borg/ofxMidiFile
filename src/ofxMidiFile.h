//
//  ofxMidiFile.h
//  JALC_AudioAnalyser
//
//  Created by Andreas Borg on 19/05/2015.
//



/*
TODO:

    // we change panorama in channels 0-2

    m.SetControlChange ( chan = 0, ctrl = 0xA, val = 0 ); // channel 0 panorama = 0 at the left
    tracks.GetTrack( trk )->PutEvent( m );

    m.SetControlChange ( chan = 1, ctrl, val = 64 ); // channel 1 panorama = 64 at the centre
    tracks.GetTrack( trk )->PutEvent( m );

    m.SetControlChange ( chan = 2, ctrl, val = 127 ); // channel 2 panorama = 127 at the right
    tracks.GetTrack( trk )->PutEvent( m );

    // we change musical instrument in channels 0-2

    m.SetProgramChange( chan = 0, val = 19 ); // channel 0 instrument 19 - Church Organ
    tracks.GetTrack( trk )->PutEvent( m );
    
    
    
    
    
        // after note(s) on before note(s) off: add words to music in the present situation
    tracks.GetTrack( trk )->PutTextEvent(t, META_LYRIC_TEXT, "Left");


MidiFiles exported from Ableton don't seem to contain BPM info from when exported.
Each bar in 4/4 is 384 clicks 
//384 ableton default
//480 logic default

*/



//

#ifndef __JALC_AudioAnalyser__ofxMidiFile__
#define __JALC_AudioAnalyser__ofxMidiFile__

#include "ofMain.h"
#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>
#include <jdksmidi/sequencer.h>

using namespace jdksmidi;

class MidiFileEvent : public ofEventArgs{
    
public:
    
    MidiFileEvent() {
        tempo = 0;
        note = 0;
        velocity = 0;
        channel = 0;
        beat = 0;
        time = 0;
        type = "NOTE_ON";
        trackId = "";//use to separate tracks
    }
    
    int beat=1;
    float time=0;
    float tempo=0;
    int note=0;
    int velocity=100;
    int channel=1;
    string trackId="";
    string type="";//specify NOTE_ON, NOTE_OFF etc
    
    static ofEvent<MidiFileEvent>	NOTE_ON;
    static ofEvent<MidiFileEvent>	NOTE_OFF;
    static ofEvent<MidiFileEvent>	PLAY;
    static ofEvent<MidiFileEvent>	STOP;
    static ofEvent<MidiFileEvent>	BEAT;
    static ofEvent<MidiFileEvent>	MTC;
    static ofEvent<MidiFileEvent>	NOTES_ALL_OFF;
};



inline ostream& operator<<(ostream& os, MidiFileEvent& n){
     os <<"[MidiFileEvent "<< n.type<<" note: "<<n.note<<", channel: "<<n.channel<<", velocity: "<<n.velocity <<", time: "<<n.time<<", tempo: "<<n.tempo<<"]";
    return os;
}
    


typedef enum MIDI_PLAYER_STATE{
    MIDI_PLAYER_STATE_IDLE,
    MIDI_PLAYER_STATE_LOADING,
    MIDI_PLAYER_STATE_LOADED,
    MIDI_PLAYER_STATE_PLAYING,
    MIDI_PLAYER_STATE_STOPPED
}MIDI_PLAYER_STATE;

class ofxMidiFilePlayer:public ofThread{
    public:
    ofxMidiFilePlayer(){
        count = 0;
		midiFileName = "";

        lastCheckedTime = 0;
		currentTime = 0.0;
		nextEventTime = 0.0;
		
		musicDurationInSeconds = 0;
		max_time = 0;
		myTime = 0;
		
		
        doLoop = true;
		isReady = true;
        numMidiTracks = 0;
        loadedOk = 0;
        shouldPlay = 0;
        setState(MIDI_PLAYER_STATE_IDLE);
        
        
        
        lastTimedBigMessage = new MIDITimedBigMessage();
        //lastTimedBigMessage = 0;
        tracks = new MIDIMultiTrack();
        sequencer = 0;
        
        trackLoader = 0;
        trackReader = 0;
        
        trackId = "";
 }
    
    
    ~ofxMidiFilePlayer(){
        clear();
        if(lastTimedBigMessage){
            delete lastTimedBigMessage;
            lastTimedBigMessage = 0;
        }
    }
    
    
    MIDI_PLAYER_STATE _state;
    
    void setState(MIDI_PLAYER_STATE s){
        _state = s;
    }
    
    int count;
	bool isReady;
	string midiFileName;

    float lastCheckedTime;
	float currentTime;
	float nextEventTime;
	
	double musicDurationInSeconds;
	float max_time;
	long myTime;
	bool doLoop;
    bool shouldPlay;
    
    string trackId;
    void setTrackId(string str){
        trackId =  str;
    }
    
    string getTrackId(){
        return trackId;
    }
    
	MIDITimedBigMessage *lastTimedBigMessage;
    MIDISequencer *sequencer;
    MIDIMultiTrack *tracks;
    
    MIDIFileReadMultiTrack *trackLoader;
    MIDIFileRead *trackReader;
    
    int numMidiTracks;
    bool loadedOk;
    
    void load(string file){
    
        if(isThreadRunning()){
            stopThread();
        }
        
        clear();
        tracks = new MIDIMultiTrack();
        
        midiFileName = file;
        string filePath = ofToDataPath(midiFileName, true);
        if(!ofFile::doesFileExist(filePath)){
            ofLogError()<<midiFileName <<" does not exist"<<endl;
            return;
        }
        
  
        setState(MIDI_PLAYER_STATE_LOADING);
        startThread();
        
    }
    
    
    void clear(){
        if(tracks){
            delete tracks;
            tracks = 0;
        }
        
        if(sequencer){
            delete sequencer;
            sequencer = 0;
        }
        
        if(trackLoader){
            delete trackLoader;
            trackLoader = 0;
        }
        if(trackReader){
            delete trackReader;
            trackReader = 0;
        }


    }
    
    
    
    
    void stop(){
        setState(MIDI_PLAYER_STATE_STOPPED);
        stopThread();
    }
    
    void play(){
    
        count = 0;


		currentTime = 0.0;
		nextEventTime = 0.0;
		
        
        shouldPlay = 1;
        
        if(loadedOk){
            setState(MIDI_PLAYER_STATE_PLAYING);
		}
        if(!isThreadRunning()){
            startThread();
        }
    
    }
    
    
    bool isPlaying(){
        return _state == MIDI_PLAYER_STATE_PLAYING;
    }
    
    
    
    //TODO: Extend this to include any events you might need, pitch etc
	void DumpMIDITimedBigMessage( const MIDITimedBigMessage *msg ){
    
		if ( msg ){
            MidiFileEvent e;
			parseMsgToEvent(e,msg,true);

		}
	}
    
    
    void parseMsgToEvent(MidiFileEvent &e,const MIDITimedBigMessage *msg, bool doNotify = true ){
        char msgbuf[1024];
        lastTimedBigMessage->Copy(*msg);

        e.time = msg->GetTime();
        e.tempo = msg->GetTempo();
        e.trackId = getTrackId();
        
        if ( msg->IsBeatMarker() ){
            e.type = "BEAT";
            if(doNotify){
                ofNotifyEvent(MidiFileEvent::BEAT, e);
            }
            //ofLog(OF_LOG_VERBOSE, "%8ld : %s <------------------>", msg->GetTime(), msg->MsgToText ( msgbuf ) );
        }else if(msg->IsNoteOn()){
            e.type = "NOTE_ON";
            e.note = msg->GetNote();
            e.velocity = msg->GetVelocity();
            e.channel = msg->GetChannel();
            if(doNotify){
                ofNotifyEvent(MidiFileEvent::NOTE_ON, e);
            }
            ofLog(OF_LOG_VERBOSE, "%f : %s", msg->GetTime()/(float)tracks->GetClksPerBeat(), msg->MsgToText ( msgbuf ) );
        }else if(msg->IsNoteOff()){
            e.type = "NOTE_OFF";
            e.note = msg->GetNote();
            e.velocity = msg->GetVelocity();
            e.channel = msg->GetChannel();
            if(doNotify){
                ofNotifyEvent(MidiFileEvent::NOTE_OFF, e);
            }
            
            ofLog(OF_LOG_VERBOSE, "%f : %s", msg->GetTime()/(float)tracks->GetClksPerBeat(), msg->MsgToText ( msgbuf ) );
        }else if(msg->IsAllNotesOff()){
            e.type = "NOTES_ALL_OFF";
            e.channel = msg->GetChannel();
            if(doNotify){
                ofNotifyEvent(MidiFileEvent::NOTES_ALL_OFF, e);
            }
            
            ofLog(OF_LOG_VERBOSE, "%f : %s", msg->GetTime()/(float)tracks->GetClksPerBeat(), msg->MsgToText ( msgbuf ) );
        }
        
        if ( msg->IsSystemExclusive() ){
            ofLog(OF_LOG_VERBOSE, "SYSEX length: %d", msg->GetSysEx()->GetLengthSE() );
        }
    };
    
    float getDurationInSeconds(){
    
        if(!loadedOk){
            ofLogWarning()<<"MidiFileEvent::getDurationInSeconds "<<midiFileName<<" not (yet) loaded"<<endl;
            return 0;
        }else{
            return musicDurationInSeconds;
        }
            
    }

    void threadedFunction() {
        // start
        
        while(isThreadRunning()) {
            lock();
            updateThread();
            unlock();
            
            if(!isReady){
            //    stop();
            }
        }
    }


    void updateThread(){
        isReady = false;
        MIDITimedBigMessage event;
        int eventTrack = 0;
        
        
        if(_state == MIDI_PLAYER_STATE_LOADING){
            
           
            
            string filePath = ofToDataPath(midiFileName, true);
            
            MIDIFileReadStreamFile rs ( filePath.c_str() );
            
            if ( !rs.IsValid() ){
                ofLogError( "ERROR OPENING FILE AT: ",  filePath);
                
            }
            
            
            trackLoader = new MIDIFileReadMultiTrack( tracks );
            trackReader = new MIDIFileRead(&rs,trackLoader );
            
            numMidiTracks = trackReader->ReadNumTracks();
            
            //trackReader->GetDivision();=clicks per beat. 

            //do this before parsing as it clears
            tracks->ClearAndResize( numMidiTracks );
            
            if ( trackReader->Parse() ){
               // cout << "reader parsed!: " << endl;
            }
            
            
            tracks->SetClksPerBeat(trackReader->GetDivision());
            sequencer = new MIDISequencer(tracks);
            
            musicDurationInSeconds = sequencer->GetMusicDurationInSeconds();//GetCurrentTimeInMs
            cout<<midiFileName<< " musicDurationInSeconds is "<<musicDurationInSeconds<<" trackReader->GetDivision() "<<trackReader->GetDivision()<<endl;
            
            int midiFormat = trackReader->GetFormat();
            cout<<midiFileName <<  " midiFormat: " << midiFormat << endl;
            
            
                                    loadedOk = 1;
            //parse to events
            parseToEvents();
            lastCheckedTime = 0;
            nextEventTime = 0;
            currentTime = 0;
            if(shouldPlay){
                setState(MIDI_PLAYER_STATE_PLAYING);
            }else{
                setState(MIDI_PLAYER_STATE_LOADED);
            }
        }else if(_state == MIDI_PLAYER_STATE_PLAYING){
            eventTrack = 0;
            
           
            
           
             if (!sequencer->GoToTimeMs (lastCheckedTime )) {
                    ofLogError("Couldn't go to time in sequence: " ,   ofToString(lastCheckedTime) );
                    //continue;
                }
            
             if ( !sequencer->GetNextEventTimeMs ( &nextEventTime ) ){
                   // ofLogError("No next events for sequence", ofToString(nextEventTime));
                    return;
                }
            
            
            
            for(;lastCheckedTime<currentTime;lastCheckedTime++){
            while ( nextEventTime <= lastCheckedTime && nextEventTime>lastCheckedTime-10){
           //
                
                
                //printf("currentTime=%06.0f : lastCheckedTime=%06.0f : scaledTime=%06.0f : nextEventTime=%06.0f : eventTrack=%02d  \n",currentTime,lastCheckedTime,currentTime, nextEventTime, eventTrack );
                
                
               
                if ( !sequencer->GetNextEventTimeMs ( &nextEventTime ) ){
                    ofLogError("No next events for sequence", ofToString(nextEventTime));
                    continue;
                }
                
                if ( nextEventTime <= currentTime && _state == MIDI_PLAYER_STATE_PLAYING){
                
                    if ( sequencer->GetNextEvent ( &eventTrack, &event )  ){
                            MIDITimedBigMessage *msg=&event;
                            if (msg->GetLength() > 0){
                                vector<unsigned char> message;
                                message.push_back(msg->GetStatus());
                                if (msg->GetLength()>0) message.push_back(msg->GetByte1());
                                if (msg->GetLength()>1) message.push_back(msg->GetByte2());
                                if (msg->GetLength()>2) message.push_back(msg->GetByte3());
                                if (msg->GetLength()>3) message.push_back(msg->GetByte4());
                                if (msg->GetLength()>4) message.push_back(msg->GetByte5());
                                message.resize(msg->GetLength());
                                
                                DumpMIDITimedBigMessage ( &event );
                            }
                            if ( !sequencer->GetNextEventTimeMs ( &nextEventTime ) ){
                                ofLogVerbose("NO MORE EVENTS FOR SEQUENCE, LAST TIME CHECKED IS: " ,   ofToString(nextEventTime) );
                            }
                    }

                }
                
            }
            }
            
            lastCheckedTime = currentTime;
            
        }
    }


    void parseToEvents(){
        if(loadedOk){
            int num_tracks = tracks->GetNumTracksWithEvents();

            for ( int nt = 0; nt < num_tracks; ++nt ){
                MIDITrack &trk = *tracks->GetTrack( nt );
                int num_events = trk.GetNumEvents();
                
                vector<MidiFileEvent> eList;
                events.push_back(eList);
                
                for ( int ne = 0; ne < num_events; ++ne ){
                    MIDITimedBigMessage *msg = trk.GetEvent( ne );
                    if (msg->GetLength() > 0){
                        vector<unsigned char> message;
                        message.push_back(msg->GetStatus());
                        if (msg->GetLength()>0) message.push_back(msg->GetByte1());
                        if (msg->GetLength()>1) message.push_back(msg->GetByte2());
                        if (msg->GetLength()>2) message.push_back(msg->GetByte3());
                        if (msg->GetLength()>3) message.push_back(msg->GetByte4());
                        if (msg->GetLength()>4) message.push_back(msg->GetByte5());
                        message.resize(msg->GetLength());
                        
                        MidiFileEvent e;
                        parseMsgToEvent(e,msg,false);
                        events[nt].push_back(e);
                    }
                }
            }
        }else{
            cout<<"Failed to load"<<endl;
        }
        
    
    }

    void print(){
        if(loadedOk){
            int num_tracks = tracks->GetNumTracksWithEvents();

            for ( int nt = 0; nt < num_tracks; ++nt ){
                MIDITrack &trk = *tracks->GetTrack( nt );
                int num_events = trk.GetNumEvents();

                for ( int ne = 0; ne < num_events; ++ne ){
                    MIDITimedBigMessage *msg = trk.GetEvent( ne );
                    if (msg->GetLength() > 0){
                        vector<unsigned char> message;
                        message.push_back(msg->GetStatus());
                        if (msg->GetLength()>0) message.push_back(msg->GetByte1());
                        if (msg->GetLength()>1) message.push_back(msg->GetByte2());
                        if (msg->GetLength()>2) message.push_back(msg->GetByte3());
                        if (msg->GetLength()>3) message.push_back(msg->GetByte4());
                        if (msg->GetLength()>4) message.push_back(msg->GetByte5());
                        message.resize(msg->GetLength());
                        
                        MidiFileEvent e;
                        parseMsgToEvent(e,msg,false);
                        cout<<e<<endl;
                    }
                }
            }
        }else{
            cout<<"Failed to load"<<endl;
        }
        
    
    }
    
    
    
    bool isLoaded(){
        return loadedOk;
    }
    
    void setLoop(bool s){
        doLoop = s;
    }
    
    
    void setPositionPercent(float percent){
        if(percent>=0 && percent<=1.0f && sequencer){
            currentTime = (int)floor( sequencer->GetMusicDurationInSeconds()*1000.0f*percent);
            myTime = currentTime;
            // lastCheckedTime = currentTime;
            cout<<"setPosition to currentTime "<<currentTime<<endl;
           // if (!sequencer->GoToTimeMs ( currentTime )) {
           //     ofLogError("Couldn't go to time in sequence: " ,   ofToString(currentTime) );
           // }
        }
    }


    float getPositionPercent(){
        return currentTime / sequencer->GetMusicDurationInSeconds()*1000.0f;
    }
    
    
    void setPositionMS(float ms){
        if(sequencer){
            currentTime = ms;
            myTime = currentTime;
            //lastCheckedTime = currentTime;
            //if (!sequencer->GoToTimeMs ( currentTime )) {
           //     ofLogError("Couldn't go to time in sequence: " ,   ofToString(currentTime) );
           // }
        }
    }


    float getPositionMS(){
        if(sequencer){
            return sequencer->GetCurrentTimeInMs();
        }else{
            return 0;
        }
    }
    
    

    vector<vector <MidiFileEvent> >events;
};



class ofxMidiFile{
    public:
    
    ofxMidiFile(){
        setTrackNum(4);
        setClicksPerBeat(384);//ableton default
        setTempo(120);
        setTimeSignature(4,4);
        setTitle("openFrameworks to midi");
    
        
    };
    ~ofxMidiFile(){
        _player.tracks->Clear();
        _player.stopThread();
    };
    
    void addNoteOn(int note,int velocity,int time, int track=1,int channel = 0){
        MIDITimedBigMessage m;
        m.SetTime(time);
        m.SetNoteOn(channel, note, velocity );
        _player.tracks->GetTrack( track )->PutEvent( m );
    
    };
    
    void addNoteOff(int note,int velocity,int time, int track=1,int channel = 0){
        MIDITimedBigMessage m;
        m.SetTime(time);
        m.SetNoteOff(channel, note, velocity );
        _player.tracks->GetTrack( track )->PutEvent( m );
    
    };
    
    
    void setTimeSignature(int numerator, int denominator = 4, int time = 0){
        MIDITimedBigMessage m;
        m.SetTime(time);

        // track 0 is used for tempo and time signature info, and some other stuff

        int trk = 0;

        /*
          SetTimeSig( numerator, denominator_power )
          The numerator is specified as a literal value, the denominator_power is specified as (get ready!)
          the value to which the power of 2 must be raised to equal the number of subdivisions per whole note.

          For example, a value of 0 means a whole note because 2 to the power of 0 is 1 (whole note),
          a value of 1 means a half-note because 2 to the power of 1 is 2 (half-note), and so on.

          (numerator, denominator_power) => musical measure conversion
          (1, 1) => 1/2
          (2, 1) => 2/2
          (1, 2) => 1/4
          (2, 2) => 2/4
          (3, 2) => 3/4
          (4, 2) => 4/4
          (1, 3) => 1/8
        */

        if(denominator == 1){
            m.SetTimeSig(numerator, 1 );
        }else if(denominator == 4){
            m.SetTimeSig( numerator, 2 );// measure 4/4 (default values for time signature)
        
        }else if(denominator == 8){
            m.SetTimeSig(numerator, 3 );
        }
    
        _player.tracks->GetTrack( trk )->PutEvent( m );
        
        
    };
    void setTitle(string str){
        _player.tracks->GetTrack( 0 )->PutTextEvent(t, META_TRACK_NAME, str.c_str());
    };
    
    
    
    void save(string str){
        
        ofFile file(str);
        //this became an issue on 64bit where c_str returns garbage
        //http://stackoverflow.com/questions/23464504/string-and-const-char-and-c-str
        string saveScope = file.getAbsolutePath();
        const char *outfile_name = saveScope.c_str();
        MIDIFileWriteStreamFileName out_stream( outfile_name );


        if( out_stream.IsValid() ){
        
            for(int i = 0;i< _player.tracks->GetNumTracks();i++){
                MIDITrack *t =_player.tracks->GetTrack ( i );
                t->SortEventsOrder();
            }
     
        
            // the object which takes the midi tracks and writes the midifile to the output stream
            MIDIFileWriteMultiTrack writer( _player.tracks, &out_stream );

            // write the output file
            if ( writer.Write( _player.tracks->GetNumTracks() ) ){
                cout << "\nOK writing file " << file.getAbsolutePath()<< endl;
            }else{
                cerr << "\nError writing file " << file.getAbsolutePath() << endl;
            }
        }   else{
            cerr << "\nError opening file " << file.getAbsolutePath() << endl;
        }


    };
    
    void setTempo(float bpm, MIDIClockTime time = 0){
        // int tempo = 1000000; // set tempo to 1 000 000 usec = 1 sec in crotchet
        // with value of clks_per_beat (100) result 10 msec in 1 midi tick
        // If no tempo is define, 120 beats per minute is assumed.
        
        //how long is each tick in bpm?
        //60bpm > 1 beats per second > 100 ticks per second > 10 msecs per tick   > tempo 1 000 000
        //120bpm > 2 beats per second > 200 ticks per second > 5 msecs per tick   > tempo   500 000
        //240bpm > 4 beats per second > 400 ticks per second > 2.5 msecs per tick > tempo   250 000
        
        
        _tempo = 1000000/(bpm / 60*_player.tracks->GetClksPerBeat()) * _player.tracks->GetClksPerBeat();
        
        
        MIDITimedBigMessage m;
        m.SetTime(time);
        m.SetTempo( _tempo );
        _player.tracks->GetTrack( 0 )->PutEvent( m );
    
    };
    
    //use to convert ms from beginning of song, to MIDIClockTime ticks assuming bpm has been constant
    MIDIClockTime msToMidiClockTicks(float ms){
        //eg.  MIDIClockTime dt = 100; // time interval (1 second)
        float msPerTick = (_tempo/1000.0f)/(float)_player.tracks->GetClksPerBeat();
        
        // cout<<"_tempo "<<_tempo<<" tracks.GetClksPerBeat() "<<tracks.GetClksPerBeat()<<" msPerTick "<<msPerTick<<endl;
        MIDIClockTime tickNum = ms /msPerTick;
        return tickNum;
    }
    
    void setAllNotesOff(){
        //SetAllNotesOff (unsigned char chan, unsigned char type, unsigned char mode)
    }
    void setTrackNum(int n){
        //careful, as it clears out
        _player.tracks->ClearAndResize(n);
        
    };
    
    void setClicksPerBeat(int c){
    // number of ticks in crotchet (1...32767)
        _player.tracks->SetClksPerBeat(c);
    }
    
    int getClicksPerBeat(){
        return _player.tracks->GetClksPerBeat();
    }
    
    void setTrackName(int n,string str){
        // META_TRACK_NAME text in tracks >= 1 Sibelius uses as instrument name (left of staves)
        _player.tracks->GetTrack( n )->PutTextEvent(t, META_TRACK_NAME, str.c_str());
    }
    
    
    void clear(){
        stop();
        _player.tracks->Clear();
        _player.loadedOk = false;
    }
    
    bool isLoaded(){
        return _player.isLoaded();
    }
    
    //read/play functions
    
    void load(string str){
        ofLogVerbose()<<"ofxMidiFile::load "<<str<<endl;
        _player.load(str);
    }
    
    void play(){
        _player.play();
    }
    
    void stop(){
        _player.stop();
    }
    
    void setLoop(bool s){
        _player.setLoop(s);
    }
    
    float getDurationInSeconds(){
        return _player.getDurationInSeconds();
    }
    
    void setPositionPercent(float percent){
        _player.setPositionPercent(percent);
    }
    
        
    float getPositionPercent(){
        return _player.getPositionPercent();
    }
    
    bool isPlaying(){
        return _player.isPlaying();
    }
    
    
     
    void setPositionMS(float ms){
       _player.setPositionMS(ms);
    }


    float getPositionMS(){
       return _player.getPositionMS();
    }

    void print(){
        _player.print();
    
    }
    
    
    void setTrackId(string str){
        _player.setTrackId(str);
    }
    
    string getTrackId(){
        return _player.getTrackId();
    }
    
    
    vector<vector <MidiFileEvent> > getEvents(){
        return _player.events;
    };
    
    
    protected:
        //MIDITimedBigMessage m;
        MIDIClockTime t; // time in midi ticks
        //MIDIMultiTrack tracks;
    
        int _tempo;

        ofxMidiFilePlayer _player;
};

#endif /* defined(__JALC_AudioAnalyser__ofxMidiFile__) */
