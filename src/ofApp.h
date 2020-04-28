#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include "ofxHttpUtils.h"
#include "ofxJSON.h"
#include "ofxTrueTypeFontUL2.h"
#include "ofxTwistedRibbon.h"
#include "FrameTimer.h"

//#define DRAW_DEBUG

#define MAX_VEL 50.0
#define MIN_ACC 0.05
#define PORT 8080
#define TRAJECT_RAD 5
#define ACC_SCALE 25.0
#define DATA_SMOOTH .3
#define AUTO_ROT_SCALE .2
#define GUIDE_RATIO 1

#define MAX_CHAR_INTERVAL 12000
#define MIN_CHAR_INTERVAL 3000
#define TRANSITION_TIME 500
#define EMERGE_TIME 1000

#define PRINT_WIDTH 2480
#define PRINT_HEIGHT 3508
#define PRINT_TEXT_SIZE 600

#define MSTATE 4

class ofApp : public ofBaseApp{

        void draw33Grid(float wid_,ofColor color_,float alpha_);
        ofVec2f getTextPos(int idx_);
        bool isLegalChar(string str_);
    
	public:
    
        bool DRAW_DEBUG;
    
        enum PState {SLEEP,PLAY,END,PRINT};
        PState _state;
        void setState(PState set_);
        void nextState();
    
        float REGION_WIDTH;
        float REGION_HEIGHT;
    
        float TEXT_POS;
        float TEXT_SIZE;
    
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void mouseReleased(int x, int y, int button);
    
        void reset();
        void newResponse(ofxHttpResponse & response);
        void sendTrajectory(int seg_);
    
        list<ofPolyline> _trajectory;
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
        void pendingChar();
    
    
    
        FrameTimer _timer_transition;
        FrameTimer _timer_emerge;
        ofVec3f _trans_dest_pos;
        float _trans_dest_scale;
    
        void startTextTransition();
        void onTransitionEnd(int& i);
    
    
        float _dmil,_last_mil;
    
//        bool _playing;
        void startGame();
        void endGame();
    
        bool _processing;
        ofVec3f _transition_dest;
    
        ofEasyCam _camera;
        ofxTwistedRibbon *_ribbon;
    
        bool _new_word;
        vector<string> _waiting_word;
    
        FrameTimer _timer_print;
        ofFbo _fbo_print;
        ofImage _img_print;
//        string _filename;
        void createPrinterImage();
        void sendToPrinter(string file_);
    
        list<list<ofVec2f>> _tmp_trace;
    
        ofFbo _fbo_tmp;
    
        
        ofColor _color_text,_color_ribbon_start,_color_ribbon_end;
        ofColor _color_background,_color_grid;
    
        ofColor lerpColor(ofColor start_,ofColor end_,float p);
    
};
