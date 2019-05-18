#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    std::cout << "listening for osc messages on port " << PORT << "\n";
    _osc_receiver.setup( PORT );
    
    ofAddListener(_http_utils.newResponseEvent,this,&ofApp::newResponse);
    _http_utils.start();
    
    TEXT_SIZE=ofGetHeight()*.8/4/2;
    _font.loadFont("NotoSerifCJKtc-Medium.otf", TEXT_SIZE);
    
    _dmil=ofGetElapsedTimeMillis();
    startGame();
}

//--------------------------------------------------------------
void ofApp::update(){
    
    _dmil=ofGetElapsedTimeMillis()-_last_mil;
    _last_mil+=_dmil;
    if(!_paused) _timer_char.update(_dmil);
    
    if(_playing && _timer_char.val()==1){
        sendTrajectory();
        _timer_char.reset();
//        startNewChar();
    }
    
    
    //if(!_osc_receiver.hasWaitingMessages()) _paused=true;
    while(_osc_receiver.hasWaitingMessages()){
        // get the next message
        ofxOscMessage m;
        _osc_receiver.getNextMessage(&m);
        
        // check for mouse moved message
        if(m.getAddress()=="/acc"){
            
            ofVec3f tmp_rot(m.getArgAsFloat(3),m.getArgAsFloat(4),m.getArgAsFloat(5));
            _rot=_rot*.5+tmp_rot*.5;
            
            ofVec3f tmp_acc(m.getArgAsFloat(0),m.getArgAsFloat(1),m.getArgAsFloat(2));
            
            if(tmp_acc.length()<MIN_ACC){
                tmp_acc=ofVec3f(0);
                _vel*=.5;
                
                if(!_paused){
                    _paused=true;
                    ofLog()<<"... pause ...";
                }
            }else{
                
                tmp_acc*=ACC_SCALE;
                tmp_acc.rotateRad(_rot.x, _rot.y, _rot.z);
                
                _acc=_acc*.5+tmp_acc*.5;
                
                _vel+=_acc;
                
                if(_paused){
                    ofLog()<<"... resume ...";
                    _paused=false;
                }
            }
            
            ofVec3f tmp_pos=_pos+_vel;
            
            if(tmp_pos.x>ofGetWidth()/2){ _acc.x=0; _vel.x=0;}
            if(tmp_pos.x<-ofGetWidth()/2){ _acc.x=0; _vel.x=0;}
            if(tmp_pos.y>ofGetHeight()/2){ _acc.y=0; _vel.y=0;}
            if(tmp_pos.y<-ofGetHeight()/2){ _acc.y=0; _vel.y=0;}
            
            tmp_pos.x=ofClamp(tmp_pos.x,-ofGetWidth()/2,ofGetWidth()/2);
            tmp_pos.y=ofClamp(tmp_pos.y,-ofGetHeight()/2,ofGetHeight()/2);
            
            (_trajectory.back()).push_back(tmp_pos);
            
            _pos=tmp_pos;
            
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(255);
    
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
    
    
    ofSetColor(255,0,0);
    ofNoFill();
    for(auto& t:_trajectory){
        if(t.size()>0){
            ofBeginShape();
                for(auto& p:t)  ofVertex(p.x,p.y);
            ofEndShape();
        }
    }
    

    ofPopMatrix();
    
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2-TEXT_SIZE/2,ofGetHeight()/2-TEXT_SIZE*2*2);
    ofSetColor(120);
    int i=0;
    for(auto&t :_text){
       
        ofPushMatrix();
        ofTranslate(-TEXT_SIZE/2,TEXT_SIZE*2*i);
        ofPushStyle();
        ofNoFill();
            draw33Grid(TEXT_SIZE*2);
        ofPopStyle();
        
        auto r=_font.getStringBoundingBox(t, 0, 0);
        _font.drawString(t,TEXT_SIZE-r.width/2,TEXT_SIZE+TEXT_SIZE/1.9);
        
        ofPopMatrix();
        
        i++;
    }
    ofPopMatrix();

#ifdef DRAW_DEBUG
    if(_playing){
        ofPushStyle();
        ofSetColor(255,0,0,180);
        ofFill();
            ofDrawRectangle(0,ofGetHeight()-2,ofGetWidth()*_timer_char.val(),2);
        ofPopStyle();
        
    }
    ofPushMatrix();
    ofTranslate(20, ofGetHeight()-20);
    ofRotateX(ofRadToDeg(_rot.x));
    ofRotateY(ofRadToDeg(_rot.y));
    ofRotateZ(ofRadToDeg(_rot.z));
        ofDrawAxis(10);
    ofPopMatrix();
#endif
    
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch(key){
        case 'r':
            startGame();
            break;
        case 's':
            startNewChar();
            break;
        case 'p':
            endGame();
            break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

void ofApp::reset(){
    
    _trajectory.clear();
    
    _vel=ofVec3f(0);
    _acc=ofVec3f(0);
    _rot=ofVec3f(0);
    _pos=ofVec3f(0);
    
    _text.clear();
}


void ofApp::newResponse(ofxHttpResponse & response){
    auto s= ofToString(response.status) + ": " + (string)response.responseBody;
    ofxJSONElement res_((string)response.responseBody);
    if(res_[0]=="SUCCESS"){
        auto t=res_[1][0][1];
        int len=t.size();
      
        bool found_=false;
        for(int i=0;i<len;++i){
            string tmp_=t[i].asString();
//            if(tmp_.size()>1) break;
            auto x=ofToHex(tmp_);
            ofLog()<<tmp_<<" hex= "<<ofToHex(tmp_);
            if(x>="e40000"){
                
                bool repeat_=false;
                for(auto& ss:_text){
                    if(ss.compare(tmp_)==0){
                        repeat_=true;break;
                    }
                }
                if(!repeat_){
                    _text.push_back(t[i].asString());
                    found_=true;
                    break;
                }
            }
        }
        
        if(!found_){
            ofLog()<<"!!! cant get a word !!!";
            //TODO: add more time
            float t=ofRandom(MIN_CHAR_INTERVAL,MAX_CHAR_INTERVAL);
            _timer_char=FrameTimer(t);
            _timer_char.restart();
        }else{
            
            if(_text.size()>=4) endGame();
            else startNewChar();
        }
    }else{
        ofLog()<<"!!! cant get a word !!!";
        //TODO: add more time
        float t=ofRandom(MIN_CHAR_INTERVAL,MAX_CHAR_INTERVAL);
        _timer_char=FrameTimer(t);
        _timer_char.restart();
    }
    
    ofLog()<<s;
    
   
}

void ofApp::sendTrajectory(){
    ofxHttpForm form;
    form.action="https://www.google.com.tw/inputtools/request?ime=handwriting&app=mobilesearch&cs=1&oe=UTF-8";
    form.method=OFX_HTTP_POST;
    form.addHeaderField("content-type","application/json");
    
    ofxJSONElement json_;

    ofxJSONElement req_;
    ofxJSONElement trace_;
    int i=0;
    for(auto&p: _trajectory.back()){
        trace_[0][i]=p.x+ofGetWidth()/2;
        trace_[1][i]=p.y+ofGetHeight()/2;
        i++;
    }
    
    req_["ink"][0]=trace_;
    req_["language"]="zh_TW";
    req_["max_num_result"]=1;
    json_["requests"][0]=req_;
    json_["options"]="enable_pre_space";
    
    //ofLog()<<json_.getRawString();
    
    form.data=json_.getRawString();
    
    _http_utils.addForm(form);
}

void ofApp::startNewChar(){
    
    _trajectory.push_back(list<ofVec2f>());
    _pos*=0;
    _vel*=0;
    _acc*=0;
    
    float t=ofRandom(MIN_CHAR_INTERVAL,MAX_CHAR_INTERVAL);
    _timer_char=FrameTimer(t);
    _timer_char.restart();
    
    ofLog()<<">>> next char t="<<t;
    
}

void ofApp::startGame(){
    
    
    ofLog()<<"------ Game Start ------ ";
    _playing=true;
    _paused=true;
    
    reset();
    startNewChar();
}

void ofApp::endGame(){
    
    ofLog()<<"------ End of Game ------ ";
    _playing=false;
    
    // TODO: print!
    
}

void ofApp::draw33Grid(float wid_){
    
    
    ofPushStyle();
    
    ofSetLineWidth(3);
        ofDrawRectangle(0,0,wid_,wid_);
    ofPushStyle();
    ofSetColor(255,120);
    ofFill();
        ofDrawRectangle(0,0,wid_,wid_);
    ofPopStyle();
    
    //ofNoFill();
    
    ofSetLineWidth(1);
    for(int x=0;x<3;++x)
        for(int y=0;y<3;++y)
            ofDrawRectangle(x*wid_/3,y*wid_/3,wid_/3,wid_/3);
    
   
    ofPopStyle();
}
