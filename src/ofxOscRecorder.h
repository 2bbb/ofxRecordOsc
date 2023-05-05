//
//  ofxOscRecorder.h
//
//  Created by 2bit on 2021/04/19.
//

#ifndef ofxOscRecorder_h
#define ofxOscRecorder_h

#define OFX_OSCRECORER_DEBUG 0

#include "ofxOscMessageExJsonConversion.h"
#include "ofxRecordOscData.h"

#include "ofxPubSubOsc.h"

#include "ofEventUtils.h"
#include "ofEvent.h"
#include "ofEvents.h"

#include "ofLog.h"

namespace ofx {
    namespace RecordOsc {
        struct Recorder {
            using clock = std::chrono::high_resolution_clock;
            
            void setup(const std::string &rec_start_address = "",
                       const std::string &rec_stop_address = "",
                       std::size_t num_subprocess = 8)
            {
                if(rec_start_address != "") metadata.system_message.recording_start = rec_start_address;
                if(rec_stop_address != "") metadata.system_message.recording_stop = rec_stop_address;
                setupSubProcesses(num_subprocess);
                auto &&events = ofEvents();
                ofAddListener(events.update,
                              this,
                              &Recorder::update,
                              OF_EVENT_ORDER_BEFORE_APP);
                ofAddListener(events.exit,
                              this,
                              &Recorder::exit,
                              OF_EVENT_ORDER_BEFORE_APP);
            }
            
            void setFileFormat(FileFormat file_format) {
                format = file_format;
            }
            
            bool startRecording(decltype(clock::now()) now) {
                if(is_recording_now) {
                    ofLogWarning("ofxOscRecorder") << "already recording is started.";
                    return false;
                }
                metadata.start();
                start = now;
                trashQueue();
                osc_sequence = ofJson::array();
                is_recording_now = true;
                return true;
            }
            
            bool stopRecording(const std::string &filename_prefix = "") {
                if(!isRecordingNow()) {
                    ofLogWarning("ofxOscRecorder") << "recording is not started.";
                    return false;
                }
                
                is_recording_now = false;
                while(!save_queue.empty()) ofSleepMillis(1);
                auto duration = clock::now() - start;
                double duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() / 1000.0;
                metadata.finish(duration_ms);
                saveData(filename_prefix);
                return true;
            }
            
            void listen(std::uint16_t port) {
                if(!metadata.addListeningPort(port)) {
                    ofLogWarning("ofxOscRecorder") << "port " << port << " is already listening.";
                }
                ofxSubscribeAllOscForPort(port, [=] (const ofxOscMessageEx &m, bool b) {
                    auto now = clock::now();
                    auto &&address = m.getAddress();
                    if(address == metadata.system_message.recording_start) {
                        startRecording(now);
                        return;
                    }
                    if(!isRecordingNow()) return;
                    
                    // message for recording
                    if(!is_allow(m)) return;
                    auto offset = now - start;
                    double offset_ms = std::chrono::duration_cast<std::chrono::milliseconds>(offset).count() / 1000.0;
                    save_queue.send({offset_ms, m});
                    digests.push_back(ofVAArgsToString("%6.3f: %s [%ld]", offset_ms, m.getAddress().c_str(), m.getNumArgs()));
                    
                    if(address == metadata.system_message.recording_stop) {
                        std::string filename_prefix = "osc_sequence";
                        if(0 < m.getNumArgs()
                           && (m.getArgType(0) == OFXOSC_TYPE_STRING
                               || m.getArgType(0) == OFXOSC_TYPE_SYMBOL)
                           )
                        {
                            filename_prefix = m.getArgAsString(0);
                        }
                        stopRecording(filename_prefix);
                    }
                });
            }
            
#pragma mark custom time calculator
            
            void setCustomTimeCalculator(std::function<double(const ofxOscMessageEx &, double)> calculator, bool need_mutex = false) {
                if(need_mutex) {
                    custom_time_calculator = [=] (const ofxOscMessageEx &m, double t)
                    {
                        auto &&_ = std::lock_guard<decltype(custom_time_calculator_mutex)>(custom_time_calculator_mutex);
                        return calculator(m, t);
                    };
                } else {
                    custom_time_calculator = calculator;
                }
            }
            
#pragma mark whitelist / blacklist
            
            void addWhitelist(const std::string &address)
            { metadata.addWhitelist(address); };
            
            template <typename string_container>
            void addWhitelists(const string_container &addresses)
            { metadata.addWhitelists(addresses); };
            
            void printWhitelists() const
            { metadata.printWhitelists(); };
            
            void clearWhitelists()
            { metadata.clearWhitelists(); };
            
            void addBlacklist(const std::string &address)
            { metadata.addBlacklist(address); };
            
            template <typename string_container>
            void addBlacklists(const string_container &addresses)
            { metadata.addBlacklists(addresses); };
            
            void printBlacklists() const
            { metadata.printBlacklists(); };
            
            void clearBlacklists()
            { metadata.clearBlacklists(); };

#pragma mark -
            
            void update(ofEventArgs &) {
                if(digest_length < digests.size()) {
                    digests.erase(digests.begin(),
                                  digests.begin() + digests.size() - 100);
                }
            }
            
            bool isRecordingNow() const
            { return is_recording_now; };
            
            std::string digestString() const {
                return std::accumulate(digests.crbegin(),
                                       digests.crend(),
                                       std::string{""},
                                       [] (std::string res, const std::string &add) {
                                           return res + add + "\n";
                                       });
            };
            
            void saveData(const std::string &fileprefix) {
                auto &&_ = std::lock_guard<decltype(osc_sequence_mutex)>(osc_sequence_mutex);
                ofJson save_data = ofJson::object();
                save_data["metadata"] = metadata;
                save_data["sequence"] = std::move(osc_sequence);
                auto ext = RecordOsc::detail::to_ext(format);
                auto filepath = ofToDataPath(fileprefix + "-" + ofGetTimestampString("%Y%m%d-%H%M%S") + "." + ext, true);
                auto success = RecordOsc::detail::save(filepath, save_data, format);
                if(success) {
                    ofLogNotice("ofxOscRecorder") << "finished to save data to " << filepath;
                } else {
                    ofLogError("ofxOscRecorder") << "failed to save data to " << filepath;
                }
                osc_sequence = ofJson();
                trashQueue();
            };
            
            void exit(ofEventArgs &) {
                if(isRecordingNow()) {
                    stopRecording("autosave-on-exit");
                }
                is_running = false;
                for(auto &th : process_threads) th.join();
            }
        private:
            bool is_recording_now{false};

            FileFormat format{FileFormat::Json};
            
            clock::time_point start;
            std::mutex osc_sequence_mutex;
            ofJson osc_sequence;
            Metadata metadata;
            std::atomic_bool is_running;
            std::vector<std::thread> process_threads;
            
            std::function<double(const ofxOscMessageEx &, double)> custom_time_calculator;
            std::mutex custom_time_calculator_mutex;

            bool is_allow(const ofxOscMessageEx &m) {
                if(metadata.whitelists.size()) {
                    if(metadata.whitelists.find(m.getAddress()) == metadata.whitelists.end()) return false;
                }
                if(metadata.blacklists.size()) {
                    if(metadata.blacklists.find(m.getAddress()) != metadata.blacklists.end()) return false;
                }
                return true;
            }

            ofThreadChannel<SequenceData> save_queue;
            std::vector<std::string> digests;
            std::size_t digest_length{100};

            void setupSubProcesses(std::size_t num_subprocess) {
                process_threads.reserve(num_subprocess);
                is_running = true;
                for(auto i = 0; i < num_subprocess; ++i) {
                    process_threads.emplace_back(std::thread([=] {
#ifdef TARGET_OSX
                        pthread_setname_np(ofVAArgsToString("oscrec-conv-%d", i).c_str());
#endif
                        while(this->is_running) {
                            if(!save_queue.empty()) {
                                SequenceData data;
                                save_queue.receive(data);
                                const auto &mess = data.mess;
                                ofJson &&json = ofJson::array();
                                auto offset_ms = data.offset;
                                if(custom_time_calculator) {
                                    auto custom_ms = custom_time_calculator(mess, offset_ms);
                                    json.push_back(custom_ms);
                                } else {
                                    json.push_back(offset_ms);
                                }
                                ofJson mess_json;
                                to_json(mess_json, mess);
                                json.push_back(mess_json);
                                auto &&_ = std::lock_guard<decltype(osc_sequence_mutex)>(osc_sequence_mutex);
                                if(!isRecordingNow()) continue;
                                osc_sequence.push_back(std::move(json));
                                std::this_thread::sleep_for(std::chrono::microseconds(10));
                            } else {
                                std::this_thread::sleep_for(std::chrono::milliseconds(3));
                            }
                        }
#if OFX_OSCRECORER_DEBUG
                        {
                            auto &&_ = std::lock_guard<decltype(osc_sequence_mutex)>(osc_sequence_mutex);
                            ofLogNotice("process finish") << i;
                        }
#endif
                    }));
                } // end for
            } // setupSubProcesses
            
            void trashQueue() {
                SequenceData trash;
                while(!save_queue.empty()) save_queue.receive(trash);
            }
        };
    };
};

using ofxOscRecorder = ofx::RecordOsc::Recorder;

#endif /* ofxOscRecorder_h */
