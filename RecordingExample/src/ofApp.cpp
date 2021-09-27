#include "ofMain.h"
#include "ofxOscRecorder.h"

class ofApp : public ofBaseApp {
    ofxOscRecorder recorder;
public:
    void setup() {
        // start recording: send "/start" to 22222 or 26666
        // stop recording: send "/stop" to 22222 or 26666
        //                 send ["/stop", "FILENAME"] then save FILENAME-YYYYMMDD-HHmmSS.json will be saved.
        recorder.setup("/start", "/stop");
        
        // save as json-like format
//        recorder.setFileFormat(ofxRecordOscFileFormat::MessagePack);
        
        // for non-realtime or custom time measure recording
//        recorder.setCustomTimeCalculator([](const ofxOscMessageEx &mess, double) {
//            return mess[0].as<float>();
//        });
        
        // ignore message by addresses
//        recorder.addBlacklist("/ping");
//        std::vector<std::string> blacklists = {{"/ping", "/hb"}};
//        recorder.addBlacklists(blacklists);

        // allow only specific messages
        // if not set whitelist, then allow all
//        recorder.addWhitelist("/needed/message");
//        std::vector<std::string> whitelists = {{"/needed/message", "/needed/value"}};
//        recorder.addWhitelists(whitelists);

        // setup listen ports
        recorder.listen(22222);
        recorder.listen(26666);
    }
    void update() {
        
    }
    void draw() {
        ofSetColor(255);
        ofDrawBitmapStringHighlight(recorder.isRecordingNow() ? "recording" : "waiting",
                                    20, 20,
                                    recorder.isRecordingNow() ? ofColor::red : ofColor(0, 0));
        ofDrawBitmapString(recorder.digestString(), 20, 40);
    }
    void exit() {
        
    }
    
    void keyPressed(int key) {}
    void keyReleased(int key) {}
    void mouseMoved(int x, int y) {}
    void mouseDragged(int x, int y, int button) {}
    void mousePressed(int x, int y, int button) {}
    void mouseReleased(int x, int y, int button) {}
    void mouseEntered(int x, int y) {}
    void mouseExited(int x, int y) {}
    void windowResized(int w, int h) {}
    void dragEvent(ofDragInfo dragInfo) {}
    void gotMessage(ofMessage msg) {}
};

int main() {
    ofSetupOpenGL(1024, 768, OF_WINDOW);
    ofRunApp(new ofApp());
}
