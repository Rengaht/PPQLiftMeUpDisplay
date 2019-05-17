#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include "ofxHttpUtils.h"
#include "ofxJSON.h"
#include "ofxTrueTypeFontUC.h"

#define MAX_VEL 10.0
#define PORT 8080
#define TRAJECT_RAD 1
#define ACC_SCALE 10.0
#define TEXT_SIZE 60

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void mouseReleased(int x, int y, int button);
    
        void reset();
        void newResponse(ofxHttpResponse & response);
        void sendTrajectory();
    
        list<ofVec2f> _pos;
        ofVec3f _acc;
        ofVec3f _vel;
        ofVec3f _rot;
    
        ofxOscReceiver    _osc_receiver;
    
        ofxHttpUtils _http_utils;
    
        list<string> _text;
    
        ofxTrueTypeFontUC _font;
};
