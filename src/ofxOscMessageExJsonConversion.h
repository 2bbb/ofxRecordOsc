//
//  ofxOscMessageExJsonConversion.h
//  osc-recorder
//
//  Created by 2bit on 2021/04/19.
//

#ifndef ofxOscMessageExJsonConversion_h
#define ofxOscMessageExJsonConversion_h

#include "ofxOscMessageEx.h"
#include "ofJson.h"

namespace ofx {
    namespace RecordOsc {
        namespace detail {
            union int64_serializer {
                std::int64_t value;
                struct {
                    std::uint32_t upper;
                    std::uint32_t lower;
                };
            };
        };
    };
};

inline void to_json(ofJson &json,
                    const ofxOscMessageEx &mess)
{
    auto &&m = ofJson::object();
    m["address"] = mess.getAddress();
    m["host"] = mess.getRemoteHost();
    m["port"] = mess.getRemotePort();
    m["received_port"] = mess.getWaitingPort();
    auto &&args = ofJson::array();
    for(auto i = 0; i < mess.getNumArgs(); ++i) {
        args.push_back(mess.getArgType(i));
        switch(mess.getArgType(i)) {
            case OFXOSC_TYPE_INT32:
                args.push_back(mess.getArgAsInt32(i));
                break;
            case OFXOSC_TYPE_CHAR:
                args.push_back(mess.getArgAsChar(i));
                break;
            case OFXOSC_TYPE_INT64:
                ofx::RecordOsc::detail::int64_serializer serializer;
                serializer.value = mess.getArgAsInt64(i);
                args.push_back(std::array<std::uint32_t, 2>{{
                    serializer.upper,
                    serializer.lower
                }});
                break;
            case OFXOSC_TYPE_FLOAT:
                args.push_back(mess.getArgAsFloat(i));
                break;
            case OFXOSC_TYPE_DOUBLE:
                args.push_back(mess.getArgAsDouble(i));
                break;
            case OFXOSC_TYPE_STRING:
            case OFXOSC_TYPE_SYMBOL:
                args.push_back(mess.getArgAsString(i));
                break;
            case OFXOSC_TYPE_MIDI_MESSAGE:
                args.push_back(mess.getArgAsMidiMessage(i));
                break;
            case OFXOSC_TYPE_TRUE:
                args.push_back(true);
                break;
            case OFXOSC_TYPE_FALSE:
                args.push_back(false);
                break;
            case OFXOSC_TYPE_TIMETAG:
                args.push_back(mess.getArgAsTimetag(i));
                break;
            case OFXOSC_TYPE_BLOB:
                /// TODO: incomplete implementation. base64like encoding or other is needed
                args.push_back(mess.getArgAsBlob(i).getText());
                break;
            case OFXOSC_TYPE_RGBA_COLOR:
                args.push_back(mess.getArgAsRgbaColor(i));
                break;
            case OFXOSC_TYPE_NONE:
            case OFXOSC_TYPE_TRIGGER:
            case OFXOSC_TYPE_INDEXOUTOFBOUNDS:
                args.push_back(0);
                break;
        }
    }
    m["args"] = std::move(args);
    json = std::move(m);
}

inline void from_json(const ofJson &json,
                      ofxOscMessageEx &mess)
{
    mess.setAddress(json["address"].get<std::string>());
    mess.setRemoteEndpoint(json["host"].get<std::string>(),
                           json["port"].get<std::uint16_t>());
    mess.setWaitingPort(json["received_port"].get<std::uint16_t>());
    auto &&args = json["args"];
    for(auto i = 0; i < args.size(); i += 2) {
        auto type = args[i].get<ofxOscArgType>();
        auto &&arg = args[i + 1];
        switch(type) {
            case OFXOSC_TYPE_INT32:
                mess.add(arg.get<std::int32_t>());
                break;
            case OFXOSC_TYPE_CHAR:
                mess.add(arg.get<char>());
                break;
            case OFXOSC_TYPE_INT64: {
                ofx::RecordOsc::detail::int64_serializer serializer;
                serializer.upper = arg[0];
                serializer.lower = arg[1];
                mess.add(serializer.value);
                break;
            }
            case OFXOSC_TYPE_FLOAT:
                mess.add(arg.get<float>());
                break;
            case OFXOSC_TYPE_DOUBLE:
                mess.add(arg.get<double>());
                break;
            case OFXOSC_TYPE_STRING:
            case OFXOSC_TYPE_SYMBOL:
                mess.add(arg.get<std::string>());
                break;
            case OFXOSC_TYPE_MIDI_MESSAGE:
                mess.addMidiMessageArg(arg.get<std::uint32_t>());
                break;
            case OFXOSC_TYPE_TRUE:
                mess.add(true);
                break;
            case OFXOSC_TYPE_FALSE:
                mess.add(false);
                break;
            case OFXOSC_TYPE_TIMETAG:
                mess.addTimetagArg(arg.get<std::uint64_t>());
                break;
            case OFXOSC_TYPE_BLOB: {
                /// TODO: incomplete implementation. base64like encoding or other is needed
                auto &&str = arg.get<std::string>();
                mess.addBlobArg(ofBuffer{str.c_str(), str.length()});
                break;
            }
            case OFXOSC_TYPE_RGBA_COLOR:
                mess.addTimetagArg(arg.get<std::uint32_t>());
                break;
            case OFXOSC_TYPE_NONE:
                mess.addNoneArg();
                break;
            case OFXOSC_TYPE_TRIGGER:
                mess.addTriggerArg();
                break;
            case OFXOSC_TYPE_INDEXOUTOFBOUNDS:
                break;
        }
    }
}

#endif /* ofxOscMessageExJsonConversion_h */
