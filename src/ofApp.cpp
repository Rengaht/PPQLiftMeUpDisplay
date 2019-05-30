#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofHideCursor();
    
    std::cout << "listening for osc messages on port " << PORT << "\n";
    _osc_receiver.setup( PORT );
    
    ofAddListener(_http_utils.newResponseEvent,this,&ofApp::newResponse);
    _http_utils.start();
    
    REGION_WIDTH=ofGetHeight()*1.2;
    REGION_HEIGHT=ofGetHeight();
    
    TEXT_SIZE=ofGetHeight()*.8/4;
    TEXT_POS=REGION_WIDTH+(ofGetWidth()-REGION_WIDTH)/2;
    _font.loadFont("HuaKangWeiBeiTi-1.ttc", TEXT_SIZE*.75);
    
    _dmil=ofGetElapsedTimeMillis();
    _timer_emerge=FrameTimer(EMERGE_TIME,TRANSITION_TIME);
    _timer_transition=FrameTimer(TRANSITION_TIME);
    
    ofAddListener(_timer_transition.finish_event,this,&ofApp::onTransitionEnd);
    
    _ribbon=new ofxTwistedRibbon();
    _ribbon->thickness=5;
    
    _camera.setNearClip(-ofGetWidth());
    _camera.setFarClip(ofGetWidth());
    
    _fbo_print.allocate(PRINT_WIDTH,PRINT_HEIGHT,GL_RGB);
    _img_print.load("print.png");
    _timer_print=FrameTimer(10);
    
    
//    startGame();
    setState(PState::SLEEP);
}

//--------------------------------------------------------------
void ofApp::update(){
    
    _dmil=ofGetElapsedTimeMillis()-_last_mil;
    _last_mil+=_dmil;
    if(!_paused) _timer_char.update(_dmil);
    
    _timer_emerge.update(_dmil);
    _timer_transition.update(_dmil);
    
    if(_state==PState::PLAY && _timer_char.val()==1){
        sendTrajectory();
        _timer_char.reset();
    }
    
    _timer_print.update(_dmil);
    if(_timer_print.val()==1){
        createPrinterImage();
        _timer_print.reset();
    }
    
    if(_new_word){
        // check if in font
        auto r=_font.getStringBoundingBox(_text.back(), 0, 0);
        if(r.width==0){
            _text.pop_back();
            pendingChar();
        }else{
            startTextTransition();
        }
        _new_word=false;
    }
    
    //if(!_osc_receiver.hasWaitingMessages()) _paused=true;
    while(_osc_receiver.hasWaitingMessages()){
        
        // get the next message
        ofxOscMessage m;
        _osc_receiver.getNextMessage(&m);
        
        if(_state!=PState::PLAY) continue;
        
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
//                    ofLog()<<"... pause ...";
                }
            }else{
                
                tmp_acc*=ACC_SCALE;
                tmp_acc.rotateRad(_rot.x, _rot.y, _rot.z);
                
                _acc=_acc*.5+tmp_acc*.5;
                
                _vel+=_acc;
                
                if(_paused){
//                    ofLog()<<"... resume ...";
                    _paused=false;
                }
            }
            
            ofVec3f tmp_pos=_pos+_vel;
            
            if(tmp_pos.x>REGION_WIDTH/2){ _acc.x=0; _vel.x=0;}
            if(tmp_pos.x<-REGION_WIDTH/2){ _acc.x=0; _vel.x=0;}
            if(tmp_pos.y>REGION_HEIGHT/2){ _acc.y=0; _vel.y=0;}
            if(tmp_pos.y<-REGION_HEIGHT/2){ _acc.y=0; _vel.y=0;}
            if(tmp_pos.z>0){ _acc.z=0; _vel.z=0;}
            if(tmp_pos.z<-REGION_WIDTH){ _acc.z=0; _vel.z=0;}
            
            
            tmp_pos.x=ofClamp(tmp_pos.x,-REGION_WIDTH/2,REGION_WIDTH/2);
            tmp_pos.y=ofClamp(tmp_pos.y,-REGION_HEIGHT/2,REGION_HEIGHT/2);
            tmp_pos.z=ofClamp(tmp_pos.z,-REGION_WIDTH/2,REGION_WIDTH/2);
            
            (_trajectory.back()).addVertex(ofVec3f(tmp_pos.x,tmp_pos.y,tmp_pos.z));
//            ofLog()<<"add pos: "<<tmp_pos;
            _pos=tmp_pos;
            
            ofColor color(int(ofGetElapsedTimef() * 10) % 55+200,0,0);
//            int hue = int(ofGetElapsedTimef() * 10) % 255;
//            color.setHsb(hue, 120, 220);
            _ribbon->update(_pos,color);
            
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    
    ofBackground(255);
    
    ofSetColor(250);
    ofDrawRectangle(0,0, REGION_WIDTH, REGION_HEIGHT);

//    _camera.begin();

    ofPushMatrix();
    ofTranslate(REGION_WIDTH/2, REGION_HEIGHT/2);
    
    ofPushStyle();
    ofNoFill();
    int m=_trajectory.size();

    ofPushMatrix();
    float t=_timer_transition.val();
    
   

    ofVec3f pos_(ofLerp(0,_trans_dest_pos.x,t),ofLerp(0,_trans_dest_pos.y,t),ofLerp(0,_trans_dest_pos.z,t));
    float scl_=ofMap(t,0,1,1,_trans_dest_scale);
    
    
    ofTranslate(pos_);
    ofScale(scl_,scl_);;
    ofSetColor(255,0,0,255*(1.0-t));
//    _trajectory.back().draw();
    _ribbon->draw();
    
    ofPopMatrix();

    ofPopStyle();
    ofPopMatrix();
 
//    _camera.end();

    
    ofDisableDepthTest();
    

    
    /* draw text */
    for(int i=0;i<4;++i){
        ofPushMatrix();
        ofTranslate(getTextPos(i));
        draw33Grid(TEXT_SIZE,1);
        ofPopMatrix();
    }
    
    ofPushMatrix();
    int i=0;
    for(auto&t :_text){
       
        ofPushMatrix();
        ofTranslate(getTextPos(i)); //center
        
        ofPushStyle();
        if(i==_text.size()-1){
            ofSetColor(120,255*_timer_emerge.valEaseOut());
        }else ofSetColor(120);
       
        _font.drawString(t,-TEXT_SIZE/2,TEXT_SIZE/4);
        
        ofPopMatrix();
        ofPopStyle();
        
        i++;
    }
    ofPopMatrix();
    
    
 

#ifdef DRAW_DEBUG
    switch(_state){
        case PLAY:
            ofPushStyle();
            ofSetColor(255,0,0,180);
            ofFill();
                ofDrawRectangle(0,ofGetHeight()-2,ofGetWidth()*_timer_char.val(),2);
            

            ofPushMatrix();
            ofTranslate(ofGetWidth()-20, ofGetHeight()-20);
                ofDrawCircle(0, 0, 5);
                ofDrawBitmapString("play",-50,0);
            ofPopMatrix();
            ofPopStyle();
            break;
        case SLEEP:
            ofPushStyle();
            ofSetColor(0,255,0,180);
            ofFill();
            
            ofPushMatrix();
            ofTranslate(ofGetWidth()-20, ofGetHeight()-20);
                ofDrawCircle(0, 0, 5);
                ofDrawBitmapString("sleep",-50,0);
            ofPopMatrix();
            
            ofPopStyle();
            break;
        case END:
            ofPushStyle();
            ofSetColor(0,0,255,180);
            ofFill();
            
            ofPushMatrix();
            ofTranslate(ofGetWidth()-20, ofGetHeight()-20);
                ofDrawCircle(0, 0, 5);
                ofDrawBitmapString("finish",-50,0);
            ofPopMatrix();
            
            ofPopStyle();
            break;
        case PRINT:
            ofPushStyle();
            ofSetColor(120,180);
            ofFill();
            
            ofPushMatrix();
            ofTranslate(ofGetWidth()-20, ofGetHeight()-20);
            ofDrawCircle(0, 0, 5);
            ofDrawBitmapString("print",-50,0);
            ofPopMatrix();
            
            ofPopStyle();
            break;
            
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
            setState(PState::PLAY);
            break;
        case 'e':
            setState(PState::END);
            break;
        case 'n': // reset trajectory
            startNewChar();
            break;
        case 'p': // print
            createPrinterImage();
            break;
        case ' ': // change state
            setState(PState((_state+1)%MSTATE));
            break;
        case 'f':
            ofToggleFullscreen();
            break;
    }
}
void ofApp::setState(PState set_){
    
    _state=set_;
    switch(_state){
        case SLEEP:
            reset();
            break;
        case PLAY:
            startGame();
            break;
        case END:
            endGame();
            break;
        case PRINT:
            createPrinterImage();
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
    _new_word=false;
    
    _timer_print.reset();
    
    delete _ribbon;
    _ribbon=new ofxTwistedRibbon();
    _ribbon->thickness=TRAJECT_RAD;
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
            if(isLegalChar(tmp_)){
                
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
            pendingChar();
        }else{
            
            //_timer_emerge.restart();
            _new_word=true;
            //startTextTransition();
            
        }
    }else{
        ofLog()<<"!!! cant get a word !!!";
        pendingChar();
    }
    
    ofLog()<<s;
   
   
}
void ofApp::pendingChar(){
    float t=ofRandom(MIN_CHAR_INTERVAL,MAX_CHAR_INTERVAL);
    _timer_char=FrameTimer(t);
    _timer_char.restart();
    
    _processing=false;
    
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
    auto verts= _trajectory.back().getVertices();
    for(auto&p:verts){
        trace_[0][i]=p.x+REGION_WIDTH/2;
        trace_[1][i]=p.y+REGION_HEIGHT/2;
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
    _processing=true;
    
    
}

void ofApp::startTextTransition(){
    
    float tdepth_=0;
    auto verts=_trajectory.back().getVertices();
    for(auto p:verts) tdepth_+=p.z;
    tdepth_/=(float)verts.size();
    
    _trans_dest_scale=0;//TEXT_SIZE/REGION_WIDTH;
    
   
    auto rec=_trajectory.back().getBoundingBox();
    _trans_dest_pos=ofVec3f(-REGION_WIDTH/2,-REGION_HEIGHT/2,0);
    _trans_dest_pos+=getTextPos(_text.size()-1);
    _trans_dest_pos.x-=rec.getCenter().x*_trans_dest_scale;
    _trans_dest_pos.y-=rec.getCenter().x*_trans_dest_scale;
    _trans_dest_pos.z=-tdepth_;
    
    
    _timer_emerge.restart();
    _timer_transition.restart();
    
}

void ofApp::onTransitionEnd(int& e){
    
    _processing=false;
    
    if(_text.size()>=4) endGame();
    else startNewChar();
}
void ofApp::startNewChar(){
    
    _timer_transition.reset();
    
    _trajectory.push_back(ofPolyline());
    _pos*=0;
    _vel*=0;
    _acc*=0;
    
    _trans_dest_pos=ofVec2f(0);
    _trans_dest_scale=1;
    
    float t=ofRandom(MIN_CHAR_INTERVAL,MAX_CHAR_INTERVAL);
    _timer_char=FrameTimer(t);
    _timer_char.restart();
    
    delete _ribbon;
    _ribbon=new ofxTwistedRibbon();
    _ribbon->thickness=TRAJECT_RAD;
    
    ofLog()<<">>> next char t="<<t;
    
}

void ofApp::startGame(){
    
    
    ofLog()<<"------ Game Start ------ ";
//    _playing=true;
    _paused=true;
    _processing=false;
    
//    _state=PLAY;
    
//    reset();
    startNewChar();
    
}

void ofApp::endGame(){
    
    ofLog()<<"------ End of Game ------ ";
//    _playing=false;
    
    _state=END;
    
    // TODO: print!
    //_timer_print.restart();
//    createPrinterImage();
}

void ofApp::draw33Grid(float wid_,float alpha_){
    
    ofPushMatrix();
    ofTranslate(-wid_/2,-wid_/2);
    
//    ofPushStyle();
//    ofSetColor(255,120*alpha_);
//    ofFill();
//    ofDrawRectangle(0,0,wid_,wid_);
//    ofPopStyle();
    
    ofPushStyle();
    ofSetColor(20,255*alpha_);
    ofSetLineWidth(3);
    ofNoFill();
        ofDrawRectangle(0,0,wid_,wid_);
    
    ofSetLineWidth(1);
    for(int x=0;x<3;++x)
        for(int y=0;y<3;++y)
            ofDrawRectangle(x*wid_/3,y*wid_/3,wid_/3,wid_/3);
    
   
    ofPopStyle();
    
    ofPopMatrix();
}
ofVec2f ofApp::getTextPos(int idx_){ // return center
    return ofVec2f(TEXT_POS,ofGetHeight()/2-TEXT_SIZE*2+TEXT_SIZE*idx_+TEXT_SIZE/2);
}

bool ofApp::isLegalChar(string str){
//    auto x=ofToHex(tmp_);
//    ofLog()<<tmp_<<" hex= "<<ofToHex(tmp_);
//    if(x>="e40000") return true;
//    return false;
    
    ofLog()<<"length of "<<str<<" = "<<str.length();
    if(str.length()>3){
        ofLog()<<"more than one word: "<<str;
        return false;
    }
    unsigned char utf[4]={0};
    unsigned char unicode[3]={0};
    bool res=false;
    for(int i=0; i<str.length();i++){
        if((str[i] & 0x80)==0){
            res = false;
        }else{
            utf[0] = str[i];
            utf[1] = str[i + 1];
            utf[2] = str[i + 2];
            i++;
            i++;
            unicode[0] = ((utf[0] & 0x0F) << 4) | ((utf[1] & 0x3C) >>2);
            unicode[1] = ((utf[1] & 0x03) << 6) | (utf[2] & 0x3F);
            ofLog()<<str<<" "<<ofToHex(unicode[0])<<" "<<ofToHex(unicode[1]);
            if(unicode[0] >= 0x4e && unicode[0] <= 0x9f){
                if(unicode[0] == 0x9f && unicode[1] >0xa5) res = false;
                else{
                    res=true;
                }
            }else res = false;
        }
    }
    return res;
    
}


void ofApp::createPrinterImage(){
    
    if(_text.size()<1) return;
    
    _fbo_print.begin();
    ofClear(255);
    _img_print.draw(0,0,PRINT_WIDTH,PRINT_HEIGHT);
    
        int i=0;
        for(auto& t:_text){
            ofPushMatrix();
            ofTranslate(PRINT_WIDTH/2,PRINT_HEIGHT/2-PRINT_TEXT_SIZE*_text.size()/2.0+PRINT_TEXT_SIZE*(i+.5));
            draw33Grid(PRINT_TEXT_SIZE,1);
            
            float sc_=(float)PRINT_TEXT_SIZE/TEXT_SIZE;
            ofScale(sc_,sc_);
            ofPushStyle();
            ofSetColor(20);
            
            _font.drawString(t,-TEXT_SIZE/2,TEXT_SIZE/4);
            
            ofPopMatrix();
            ofPopStyle();
            
            i++;
        }
    
    _fbo_print.end();
    
    ofPixels pix;
    _fbo_print.readToPixels(pix);
    
    ofImage img;
    img.setFromPixels(pix);
    
    string file_="tmp/snapshot"+ofGetTimestampString()+".png";
    img.save(file_);
    
    sendToPrinter(file_);
}

void ofApp::sendToPrinter(string file_){
    string cmd="lp "+ofToDataPath(file_,true);
    ofLog()<<cmd;
    ofSystem(cmd);
}
