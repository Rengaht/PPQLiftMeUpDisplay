#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    std::cout << "listening for osc messages on port " << PORT << "\n";
    _osc_receiver.setup( PORT );
    
    ofAddListener(_http_utils.newResponseEvent,this,&ofApp::newResponse);
    _http_utils.start();
    
    _font.loadFont("NotoSerifCJKtc-Medium.otf", TEXT_SIZE);
}

//--------------------------------------------------------------
void ofApp::update(){
    while(_osc_receiver.hasWaitingMessages()){
        // get the next message
        ofxOscMessage m;
        _osc_receiver.getNextMessage(&m);
        
        // check for mouse moved message
        if(m.getAddress()=="/acc"){
            ofVec3f tmp_acc(m.getArgAsFloat(0),m.getArgAsFloat(1),m.getArgAsFloat(2));
            tmp_acc*=ACC_SCALE;
            
            ofVec3f tmp_rot(m.getArgAsFloat(3),m.getArgAsFloat(4),m.getArgAsFloat(5));
            _rot=_rot*.5+tmp_rot*.5;
            
            tmp_acc.rotateRad(_rot.x, _rot.y, _rot.z);
            
            _acc=_acc*.5+tmp_acc*.5;
            
            _vel+=_acc;
            ofVec3f tmp_pos=(*_pos.end())+_vel;
            
            _pos.push_back(tmp_pos);
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
    
    ofPushMatrix();
    ofRotateX(ofRadToDeg(_rot.x));
    ofRotateY(ofRadToDeg(_rot.y));
    ofRotateZ(ofRadToDeg(_rot.z));
        ofDrawAxis(100);
    ofPopMatrix();
    
    ofSetColor(255);
    for(auto&p :_pos) ofDrawCircle(p.x,p.y,TRAJECT_RAD,TRAJECT_RAD);
    
    ofPopMatrix();
    
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2-TEXT_SIZE/2,TEXT_SIZE*1.5);
    ofSetColor(255,255,0);
    int i=0;
    for(auto&t :_text){
        _font.drawString(t,0,TEXT_SIZE*1.5*i);
        i++;
    }
    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch(key){
        case 'r':
            reset();
            break;
        case 's':
            sendTrajectory();
            break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

void ofApp::reset(){
    _pos.clear();
    _pos.push_back(ofVec3f(0));
    _vel=ofVec3f(0);
    _acc=ofVec3f(0);
    _rot=ofVec3f(0);
    
    _text.clear();
}


void ofApp::newResponse(ofxHttpResponse & response){
    auto s= ofToString(response.status) + ": " + (string)response.responseBody;
    ofxJSONElement res_((string)response.responseBody);
    if(res_[0]=="SUCCESS"){
        auto t=res_[1][0][1];
        int len=t.size();
//        for(int i=0;i<len;++i) ofLog()<<t[i];
        
        _text.push_back(t[0].asString());
    }
    
    ofLog()<<s;
}

void ofApp::sendTrajectory(){
    ofxHttpForm form;
    form.action="https://www.google.com.tw/inputtools/request?ime=handwriting&app=mobilesearch&cs=1&oe=UTF-8";
    form.method=OFX_HTTP_POST;
    form.addHeaderField("content-type","application/json");
    
    ofxJSONElement json_;
//    "options": "enable_pre_space",
//    "requests": [{
//                 "writing_guide": {
//                 "writing_area_width": options.width || this.width || undefined,
//                 "writing_area_height": options.height || this.width || undefined
//                 },
//                 "ink": trace,
//                 "language": options.language || "zh_TW"
//                 }]
    ofxJSONElement req_;
    ofxJSONElement trace_;
    int i=0;
    for(auto&p: _pos){
        trace_[0][i]=p.x+ofGetWidth()/2;
        trace_[1][i]=p.y+ofGetHeight()/2;
        i++;
    }
    
    req_["ink"][0]=trace_;//ofxJSONElement(string("[[[300, 310, 320, 330, 340],[320, 320, 320, 320, 320]],[[320, 320, 320, 320, 320],[300, 310, 320, 330, 340]]]"));
    req_["language"]="zh_TW";
    json_["requests"][0]=req_;
    json_["options"]="enable_pre_space";
    
//    ofLog()<<json_.getRawString();
    
    form.data=json_.getRawString();
    
    _http_utils.addForm(form);
}
