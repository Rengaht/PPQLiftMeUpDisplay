#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofHideCursor();
    DRAW_DEBUG=false;
    
    _color_text=ofColor(69,254,7);
    _color_ribbon_start=ofColor(255,0,137);
    _color_ribbon_end=ofColor(69,254,7);
    _color_background=ofColor(0);
    _color_grid=ofColor(255);
    
    std::cout << "listening for osc messages on port " << PORT << "\n";
    _osc_receiver.setup( PORT );
    
    ofAddListener(_http_utils.newResponseEvent,this,&ofApp::newResponse);
    _http_utils.start();
    
    REGION_WIDTH=ofGetHeight();
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
    
    
    _fbo_tmp.allocate(1280,720,GL_RGB);
    
//    ofSetFullscreen(true);
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
        sendTrajectory(ofRandom(3,30));
//        sendTrajectory(1);
        _timer_char.reset();
    }
    
    _timer_print.update(_dmil);
    if(_timer_print.val()==1){
        createPrinterImage();
        _timer_print.reset();
    }
    
    if(_new_word){
        // check if in font
        for(auto it=_waiting_word.begin();it!=_waiting_word.end();){
            auto r=_font.getStringBoundingBox(*it, 0, 0);
            if(r.width==0) _waiting_word.erase(it);
            else it++;
        }
        
        if(_waiting_word.size()==0){
//            _text.pop_back();
            pendingChar();
        }else{
            
            _text.push_back(_waiting_word[floor(ofRandom(_waiting_word.size()))]);
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
            _rot=_rot*(1.0-DATA_SMOOTH)+tmp_rot*DATA_SMOOTH;
            
            ofVec3f tmp_acc(m.getArgAsFloat(0),m.getArgAsFloat(1),m.getArgAsFloat(2));
            
            if(tmp_acc.length()<MIN_ACC){
                tmp_acc=ofVec3f(0);
                _vel*=.8;
                _acc.rotateRad(ofRandom(-AUTO_ROT_SCALE,AUTO_ROT_SCALE)*PI,ofRandom(-AUTO_ROT_SCALE,AUTO_ROT_SCALE)*PI,ofRandom(-AUTO_ROT_SCALE,AUTO_ROT_SCALE)*PI);
                
                if(!_paused){
                    _paused=true;
//                    ofLog()<<"... pause ...";
                }
            }else{
                
                tmp_acc*=ACC_SCALE;
                tmp_acc.rotateRad(_rot.x, _rot.y, _rot.z);
                
                _acc=_acc*DATA_SMOOTH+tmp_acc*(1.0-DATA_SMOOTH);
                
                if(ofRandom(30)<1)
                    _acc.rotateRad(ofRandom(-AUTO_ROT_SCALE,AUTO_ROT_SCALE)*PI,ofRandom(-AUTO_ROT_SCALE,AUTO_ROT_SCALE)*PI,ofRandom(-AUTO_ROT_SCALE,AUTO_ROT_SCALE)*PI);
                
                _vel+=_acc;
                if(_vel.length()>MAX_VEL) _vel.scale(MAX_VEL);
                
                if(_paused){
//                    ofLog()<<"... resume ...";
                    _paused=false;
                }
            }
            
            ofVec3f tmp_pos=_pos+_vel;
            
            if(tmp_pos.x>REGION_WIDTH/2){
                _acc.x*=0; _vel.x=-abs(_vel.x); tmp_pos=_pos+_vel;}
            if(tmp_pos.x<-REGION_WIDTH/2){ _acc.x*=0; _vel.x=abs(_vel.x);  tmp_pos=_pos+_vel;}
            if(tmp_pos.y>REGION_HEIGHT/2){
                _acc.y*=0; _vel.y=-abs(_vel.y);  tmp_pos=_pos+_vel;}
            if(tmp_pos.y<-REGION_HEIGHT/2){ _acc.y*=0; _vel.y=abs(_vel.y);  tmp_pos=_pos+_vel;}
            
            if(tmp_pos.z>0){ _acc.z=0; _vel.z=0;  tmp_pos=_pos+_vel;}
            if(tmp_pos.z<-REGION_WIDTH){ _acc.z=0; _vel.z=0;  tmp_pos=_pos+_vel;}
            

            tmp_pos.x=ofClamp(tmp_pos.x,-REGION_WIDTH/2,REGION_WIDTH/2);
            tmp_pos.y=ofClamp(tmp_pos.y,-REGION_HEIGHT/2,REGION_HEIGHT/2);
            tmp_pos.z=0;//ofClamp(tmp_pos.z,-REGION_WIDTH/2,REGION_WIDTH/2);
            
            (_trajectory.back()).addVertex(tmp_pos);
//            ofLog()<<"add pos: "<<tmp_pos;
            _pos=_pos*(1.0-DATA_SMOOTH)+tmp_pos*DATA_SMOOTH;
            
//            ofColor color(int(ofGetElapsedTimef() * 10) % 55+200,0,0);
//            ofColor color=lerpColor(_color_ribbon_start,_color_ribbon_end,sin(((int)_ribbon->points.size()%20)/20.0*PI));
            float dist_=ofClamp(_pos.distance(ofPoint(0,0))/REGION_WIDTH*2,0,1);
            ofColor color=lerpColor(_color_ribbon_start,_color_ribbon_end,1.0-dist_);
//            ofLog()<<color;
//            int hue = int(ofGetElapsedTimef() * 10) % 255;
//            color.setHsb(hue, 120, 220);
            _ribbon->update(_pos,color);
            
        }
    }
}
ofColor ofApp::lerpColor(ofColor start_,ofColor end_,float p){
    return ofColor(ofLerp((float)start_.r,(float)end_.r,p),
                   ofLerp((float)start_.g,(float)end_.g,p),
                   ofLerp((float)start_.b,(float)end_.b,p));
}
//--------------------------------------------------------------
void ofApp::draw(){
    
    
    ofBackground(_color_background);
    ofPushStyle();
    ofSetColor(_color_background);
        ofDrawRectangle(0,0, REGION_WIDTH, REGION_HEIGHT);
    ofPopStyle();
    
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
        draw33Grid(TEXT_SIZE,_color_grid,1);
        ofPopMatrix();
    }
    
    ofPushMatrix();
    int i=0;
    for(auto&t :_text){

        ofPushMatrix();
        ofTranslate(getTextPos(i)); //center

        ofPushStyle();
        if(i==_text.size()-1){
            ofSetColor(_color_text,255*_timer_emerge.valEaseOut());
        }else ofSetColor(_color_text);

        _font.drawString(t,-TEXT_SIZE/2,TEXT_SIZE/4);

        ofPopMatrix();
        ofPopStyle();

        i++;
    }
    ofPopMatrix();
    

   
 
    if(!DRAW_DEBUG) return;
//#ifdef DRAW_DEBUG
    
     // draw separated characters
    ofPushMatrix();
    ofTranslate(20, 20);
    int k=0;
    for(auto& p:_tmp_trace){
        ofSetColor(ofColor::fromHsb((k*50)%255, 255, 255));
        ofNoFill();
        
        ofBeginShape();
        for(auto& t:p) ofVertex(t.x,t.y);
        ofEndShape();
        k++;
    }
    ofPopMatrix();
    
   
    
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
            ofDrawBitmapString("printed",-50,0);
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
//#endif
    
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch(key){
        
        case 'r': //reset
        case 'R':
            setState(PState::SLEEP);
            break;
        case 'n': // reset trajectory
        case 'N':
            startNewChar();
            break;
        case 'p': // print
        case 'P':
            createPrinterImage();
            break;
        case ' ': // change state
            setState(PState((_state+1)%MSTATE));
            break;
        case 'f':
        case 'F':
            ofToggleFullscreen();
            break;
//        case 't':
//            sendTrajectory(1);
//            break;
//        case 'y':
//            sendTrajectory(ofRandom(3,30));
//            break;
        case 'd':
        case 'D':
            DRAW_DEBUG=!DRAW_DEBUG;
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
void ofApp::nextState(){
    switch(_state){
        case SLEEP:
            setState(PState::PLAY);
            break;
        case PLAY:
            setState(PState::END);
            break;
        case END:
            setState(PState::PRINT);
            break;
        case PRINT:
            setState(PState::SLEEP);
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
        
        _waiting_word.clear();
        
        for(int i=0;i<len;i++){
            string tmp_=t[i].asString();
            if(isLegalChar(tmp_)){
                
                bool repeat_=false;
                for(auto& ss:_text){
                    if(ss.compare(tmp_)==0){
                        repeat_=true;break;
                    }
                }
                if(!repeat_){
//                    _text.push_back(t[i].asString());
                    found_=true;
                    //break;
                    _waiting_word.push_back(tmp_);
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

void ofApp::sendTrajectory(int seg_){
    
    ofLog()<<"trajectory seg= "<<seg_;
    
    ofxHttpForm form;
    form.action="https://www.google.com.tw/inputtools/request?ime=handwriting&ha&cs=1&oe=UTF-8";
    form.method=OFX_HTTP_POST;
    form.addHeaderField("content-type","application/json");
    
    ofxJSONElement json_;

    ofxJSONElement req_;
    int i=0;
    auto verts= _trajectory.back().getVertices();
    
    
//    int seg=floor(ofRandom(3,30));
    int c=floor(verts.size()/seg_);
    
    ofxJSONElement trace_;
    
    _tmp_trace.clear();
    _tmp_trace.push_back(list<ofVec2f>());
    
    float df=.2;
    ofVec2f offset_(ofRandom(-df,df)*REGION_WIDTH,ofRandom(-df,df)*REGION_HEIGHT);
    
    for(auto&p:verts){
        
        auto pp=_camera.worldToCamera(p);
        trace_[0][i]=p.x+REGION_WIDTH/2*GUIDE_RATIO+offset_.x;
        trace_[1][i]=p.y+REGION_HEIGHT/2*GUIDE_RATIO+offset_.y;
        
        _tmp_trace.back().push_back(ofVec2f(trace_[0][i].asFloat(),trace_[1][i].asFloat()));
        i++;
        
        if(i>=c && ofRandom(seg_)<1){
            req_["ink"].append(trace_);
            trace_.clear();
            i=0;
            offset_=ofVec2f(ofRandom(-df,df)*REGION_WIDTH,ofRandom(-df,df)*REGION_HEIGHT);
            _tmp_trace.push_back(list<ofVec2f>());
        }
      
    }
    if(req_["ink"].size()<1) req_["ink"].append(trace_);
    
//    req_["ink"][0]=trace_;
    req_["language"]="zh";
    req_["max_num_result"]=20;
    
    ofxJSONElement guide_;
    guide_["writing_area_width"]=GUIDE_RATIO*REGION_WIDTH;
    guide_["writing_area_height"]=GUIDE_RATIO*REGION_HEIGHT;
    
    req_["writing_guide"]=guide_;
    
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
    
    
    ///////////////////////////////////////////////////////////////////////////
    
//    _fbo_tmp.begin();
//    ofClear(255);
//    ofPushMatrix();
//    ofTranslate(20, 20);
//    int k=0;
//    for(auto& p:_tmp_trace){
//        ofSetColor(ofColor::fromHsb((k*50)%255, 255, 255));
//        ofSetLineWidth(5);
//        ofNoFill();
//
//        ofBeginShape();
//        for(auto& t:p) ofVertex(t.x,t.y);
//        ofEndShape();
//        k++;
//    }
//    ofPopMatrix();
//    _fbo_tmp.end();
//
//    ofPixels pix;
//    _fbo_tmp.readToPixels(pix);
//
//    ofImage img;
//    img.setFromPixels(pix);
//
//    string file_="tmp/qqq"+ofGetTimestampString()+".png";
//    img.save(file_);
//
//    _fbo_tmp.begin();
//    ofClear(255);
//    ofPushMatrix();
//    ofTranslate(REGION_WIDTH/2, REGION_HEIGHT/2);
//        ofSetColor(255);
//        _ribbon->draw();
//    ofPopMatrix();
//    _fbo_tmp.end();
//
//    _fbo_tmp.readToPixels(pix);
//    img.setFromPixels(pix);
//    file_="tmp/sss"+ofGetTimestampString()+".png";
//    img.save(file_);
    ///////////////////////////////////////////////////////////////////////////
    
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

void ofApp::draw33Grid(float wid_,ofColor color_,float alpha_){
    
    ofPushMatrix();
    ofTranslate(-wid_/2,-wid_/2);
    
//    ofPushStyle();
//    ofSetColor(255,120*alpha_);
//    ofFill();
//    ofDrawRectangle(0,0,wid_,wid_);
//    ofPopStyle();
    
    ofPushStyle();
    ofSetColor(color_,255*alpha_);
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
    
    ofPushStyle();
    ofColor(255);
        _img_print.draw(0,0,PRINT_WIDTH,PRINT_HEIGHT);
    ofPopStyle();
    
        int i=0;
        for(auto& t:_text){
            ofPushMatrix();
            ofTranslate(PRINT_WIDTH/2,PRINT_HEIGHT/2-PRINT_TEXT_SIZE*_text.size()/2.0+PRINT_TEXT_SIZE*(i+.5));
            draw33Grid(PRINT_TEXT_SIZE,ofColor(0),1);
            
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
