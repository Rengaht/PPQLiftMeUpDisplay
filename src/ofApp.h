#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include "ofxHttpUtils.h"
#include "ofxJSON.h"
#include "ofxTrueTypeFontUL2.h"
#include "FrameTimer.h"

#define DRAW_DEBUG

#define MAX_VEL 5.0
#define MIN_ACC 0.05
#define PORT 8080
#define TRAJECT_RAD 1
#define ACC_SCALE 10.0

#define MAX_CHAR_INTERVAL 10000
#define MIN_CHAR_INTERVAL 3000


class ofApp : public ofBaseApp{

        void draw33Grid(float wid_);
    
	public:
    
        float TEXT_SIZE;
    
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void mouseReleased(int x, int y, int button);
    
        void reset();
        void newResponse(ofxHttpResponse & response);
        void sendTrajectory();
    
        list<list<ofVec2f>> _trajectory;
        ofVec3f _acc;
        ofVec3f _vel;
        ofVec3f _rot;
        ofVec3f _pos;
        bool _paused;
    
        ofxOscReceiver    _osc_receiver;
    
        ofxHttpUtils _http_utils;
    
        list<string> _text;
    
        ofxTrueTypeFontUL2 _font;
    
        FrameTimer _timer_char;
        void startNewChar();
    
    
        float _dmil,_last_mil;
    
        bool _playing;
        void startGame();
        void endGame();
};
