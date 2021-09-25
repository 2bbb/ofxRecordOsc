//
//  ofxRecordOscMetadata.h
//  osc-recorder
//
//  Created by 2bit on 2021/04/20.
//

#ifndef ofxRecordOscMetadata_h
#define ofxRecordOscMetadata_h

#include "ofUtils.h"
#include "ofJson.h"

#include "ofLog.h"

#include <iterator>
#include <algorithm>
#include <string>
#include <cstddef>
#include <cstdint>
#include <set>
#include <sys/stat.h>

namespace ofx {
    namespace RecordOsc {
        struct SequenceData {
            double offset;
            ofxOscMessageEx mess;
            
#pragma mark compare for sorting
            bool operator==(double t) const
            { return offset == t; };
            bool operator==(const SequenceData &m) const
            { return offset == m.offset; };
            friend bool operator==(double t, const SequenceData &m)
            { return t == m.offset; };
            
            bool operator!=(double t) const
            { return offset != t; };
            bool operator!=(const SequenceData &m) const
            { return offset != m.offset; };
            friend bool operator!=(double t, const SequenceData &m)
            { return t != m.offset; };

            bool operator<(double t) const
            { return offset < t; };
            bool operator<(const SequenceData &m) const
            { return offset < m.offset; };
            friend bool operator<(double t, const SequenceData &m)
            { return t < m.offset; };

            bool operator<=(double t) const
            { return offset <= t; };
            bool operator<=(const SequenceData &m) const
            { return offset <= m.offset; };
            friend bool operator<=(double t, const SequenceData &m)
            { return t <= m.offset; };

            bool operator>(double t) const
            { return offset > t; };
            bool operator>(const SequenceData &m) const
            { return offset > m.offset; };
            friend bool operator>(double t, const SequenceData &m)
            { return t > m.offset; };

            bool operator>=(double t) const
            { return offset >= t; };
            bool operator>=(const SequenceData &m) const
            { return offset >= m.offset; };
            friend bool operator>=(double t, const SequenceData &m)
            { return t >= m.offset; };
            
            friend
            inline void from_json(const ofJson &json,
                                  SequenceData &m)
            {
                m.offset = json[0].get<double>();
                m.mess = json[1];
            }
        }; // struct SequenceData
        
        struct Metadata {
            struct {
                std::string recording_start{"/recorder/start"};
                std::string recording_stop{"/recorder/stop"};
            } system_message;

            std::string   started_time_str;
            std::uint32_t started_timestamp;
            
            std::string   finished_time_str;
            std::uint32_t finished_timestamp;
            
            std::set<std::string> whitelists;
            std::set<std::string> blacklists;
            std::set<std::uint16_t> listening_ports;
            
            bool addListeningPort(std::uint16_t port) {
                if(listening_ports.find(port) != listening_ports.end()) return false;
                listening_ports.insert(port);
                return true;
            }
            
#pragma mark whitelist
            
            void addWhitelist(const std::string &address)
            { whitelists.insert(address); }
            
            template <typename string_container>
            void addWhitelists(const string_container &addresses) {
                using std::begin;
                using std::end;
                std::copy(begin(addresses),
                          end(addresses),
                          std::inserter(whitelists, begin(whitelists)));
            }
            
            void printWhitelists() const {
                if(whitelists.size()) {
                    ofLogNotice("ofxOscRecorder") << "---- whiltelist ----";
                    std::for_each(whitelists.begin(),
                                  whitelists.end(),
                                  [](const std::string &whitelist) {
                                      std::cout << "  " << whitelist << "\n";
                                  });
                } else {
                    ofLogNotice("ofxOscRecorder") << "no whitelists were added";
                }
            }
            
            void clearWhitelists()
            { whitelists.clear(); }
            
#pragma mark blacklist
            
            void addBlacklist(const std::string &address)
            { blacklists.insert(address); }
            
            template <typename string_container>
            void addBlacklists(const string_container &addresses) {
                using std::begin;
                using std::end;
                std::copy(begin(addresses),
                          end(addresses),
                          std::inserter(blacklists, begin(blacklists)));
            }
            
            void printBlacklists() const {
                if(blacklists.size()) {
                    ofLogNotice("ofxOscRecorder") << "---- blacklist ----";
                    std::for_each(blacklists.begin(),
                                  blacklists.end(),
                                  [](const std::string &blacklist) {
                                      std::cout << "  " << blacklist << "\n";
                                  });
                } else {
                    ofLogNotice("ofxOscRecorder") << "no blacklists were added";
                }
            }
            
            void clearBlacklists()
            { blacklists.clear(); }
            
#pragma mark -
            
            double duration;
            
            void start() {
                started_time_str = ofGetTimestampString("%Y/%m/%d %H:%M:%S.%i");
                started_timestamp = ofGetUnixTime();
            }
            
            void finish(double duration_ms) {
                finished_time_str = ofGetTimestampString("%Y/%m/%d %H:%M:%S.%i");
                finished_timestamp = ofGetUnixTime();
                duration = duration_ms;
            }
            
            friend
            inline void from_json(const ofJson &j, Metadata &md) {
                md.system_message.recording_start = j["system_message"]["recording_start"].get<std::string>();
                md.system_message.recording_stop = j["system_message"]["recording_stop"].get<std::string>();
                
                md.started_timestamp  = j["started_timestamp"];

                md.started_time_str   = j["started_time_str"].get<std::string>();
                md.started_timestamp  = j["started_timestamp"];
                md.finished_time_str  = j["finished_time_str"].get<std::string>();
                md.finished_timestamp = j["finished_timestamp"];
                md.duration           = j["duration"];
                
                md.whitelists         = j["whitelists"].get<decltype(md.whitelists)>();
                md.blacklists         = j["blacklists"].get<decltype(md.blacklists)>();
                md.listening_ports    = j["listening_ports"].get<decltype(md.listening_ports)>();
            }
            
            friend
            inline void to_json(ofJson &j, const Metadata &md) {
                ofJson system_message_json = ofJson::object();
                system_message_json["recording_start"] =
                md.system_message.recording_start;
                system_message_json["recording_stop"] = md.system_message.recording_stop;
                j["system_message"] = system_message_json;
                j["started_time_str"]   = md.started_time_str;
                j["started_timestamp"]  = md.started_timestamp;
                j["finished_time_str"]  = md.finished_time_str;
                j["finished_timestamp"] = md.finished_timestamp;
                j["duration"]           = md.duration;
                
                j["whitelists"]         = md.whitelists;
                j["blacklists"]         = md.blacklists;
                j["listening_ports"]    = md.listening_ports;
            }
        }; // struct Metadata
        
        enum class FileFormat : std::uint8_t {
            Json,
            Bson,
            CBOR,
            MessagePack,
            UBJson
        };
        
        namespace detail {
            std::string to_ext(FileFormat format) {
                switch(format) {
                    case FileFormat::Bson: return "bson";
                    case FileFormat::CBOR: return "cbor";
                    case FileFormat::MessagePack: return "msgpack";
                    case FileFormat::UBJson: return "ubjson";
                    case FileFormat::Json: return "json";
                    default:
                        ofLogWarning("ofxRecordOsc") << "unknown file format: " << (int)format << ". file will be write by JSON format.";
                        return "json";
                }
            }
            
            bool save_binary(const std::string &filepath,
                             const std::vector<std::uint8_t> &&data)
            {
                FILE *fp = std::fopen(filepath.c_str(), "wb");
                if(fp == nullptr) {
                    ofLogError("ofxRecordOsc") << "can't open file: " << filepath << " on save";
                    return false;
                }
                auto wrote = std::fwrite(data.data(), sizeof(std::uint8_t), data.size(), fp);
                if(wrote == data.size()) return true;
                ofLogWarning("ofxRecordOsc") << "written size is incorrect. written: " << wrote << ", data-size: " << data.size();
                return false;
            }
            
            bool save(const std::string &filepath,
                      const ofJson &json,
                      FileFormat format)
            {
                switch(format) {
                    case FileFormat::Bson:
                        return save_binary(filepath, ofJson::to_bson(json));
                    case FileFormat::CBOR:
                        return save_binary(filepath, ofJson::to_cbor(json));
                    case FileFormat::MessagePack:
                        return save_binary(filepath, ofJson::to_msgpack(json));
                    case FileFormat::UBJson:
                        return save_binary(filepath, ofJson::to_ubjson(json));
                    case FileFormat::Json:
                        return ofSaveJson(filepath, json);
                    default:
                        ofLogWarning("ofxRecordOsc") << "unknown file format: " << (int)format << ". file will be write by JSON format.";
                        return ofSaveJson(filepath, json);
                }
            }
            
            bool load_binary(const std::string &filepath,
                             std::vector<std::uint8_t> &data)
            {
                FILE *fp = std::fopen(filepath.c_str(), "rb");
                if(fp == nullptr) {
                    ofLogError("ofxRecordOsc") << "can't open file: " << filepath << " on load.";
                    return false;
                }
                
                struct stat stat_buf;
                if(stat(filepath.c_str(), &stat_buf) != 0) {
                    ofLogError("ofxRecordOsc") << "can't open file: " << filepath << " on load.";
                    return false;
                }
                
                std::size_t filesize = stat_buf.st_size;
                data.resize(filesize);
                
                std::size_t read = 0;
                std::size_t read_fragment = 0;
                do {
                    read_fragment = std::fread(data.data() + read_fragment, sizeof(std::uint8_t), data.size(), fp);
                    read += read_fragment;
                } while(0 < read_fragment);
                if(read == data.size()) return true;
                ofLogWarning("ofxRecordOsc") << "read size is incorrect. read: " << read << ", required data-size: " << data.size();
                return false;
            }
            
            ofJson load(const std::string &filepath, FileFormat format) {
                std::string ext = ofSplitString(filepath, ".").back();
                if(to_ext(format) != ext) ofLogWarning("ofxRecordOsc");
                switch(format) {
                    case FileFormat::Bson: {
                        std::vector<std::uint8_t> binary;
                        load_binary(ofToDataPath(filepath, true), binary);
                        return ofJson::from_bson(binary);
                    }
                    case FileFormat::CBOR: {
                        std::vector<std::uint8_t> binary;
                        load_binary(ofToDataPath(filepath, true), binary);
                        return ofJson::from_cbor(binary);
                    }
                    case FileFormat::MessagePack: {
                        std::vector<std::uint8_t> binary;
                        load_binary(ofToDataPath(filepath, true), binary);
                        return ofJson::from_msgpack(binary);
                    }
                    case FileFormat::UBJson: {
                        std::vector<std::uint8_t> binary;
                        load_binary(ofToDataPath(filepath, true), binary);
                        return ofJson::from_ubjson(binary);
                    }
                    case FileFormat::Json:
                        return ofLoadJson(filepath);
                    default:
                        ofLogWarning("ofxRecordOsc") << "unknown file format: " << (int)format << ". file will be write by JSON format.";
                        return ofLoadJson(filepath);
                }
            }
        }; // namespace detail
    }; // namespace RecordOsc
}; // namespace ofx

using ofxRecordOscFileFormat = ofx::RecordOsc::FileFormat;

#endif /* ofxRecordOscMetadata_h */
