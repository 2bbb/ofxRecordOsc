//
//  ofxRecordedOscPlayer.h
//
//  Created by 2bit on 2021/04/19.
//

#ifndef ofxRecordedOscPlayer_h
#define ofxRecordedOscPlayer_h

#include "ofxOscMessageExJsonConversion.h"
#include "ofxRecordOscData.h"

#include "ofxPubSubOsc.h"

namespace ofx {
    namespace RecordOsc {
        struct Player {
            void setup(const std::string &filepath,
                       FileFormat format = FileFormat::Json)
            {
                auto &&json = RecordOsc::detail::load(filepath, format);
                messages = json["sequence"].get<decltype(messages)>();
                metadata = json["metadata"];
                std::stable_sort(messages.begin(),
                                 messages.end());
                std::for_each(messages.cbegin(),
                              messages.cend(),
                              [=](const SequenceData &m) {
                                  addresses[m.mess.getAddress()]++;
                              });
            }
            
            void summary() const {
                using pair = std::pair<std::string, std::size_t>;
                std::vector<pair> sorted;
                sorted.reserve(addresses.size());
                std::transform(addresses.begin(),
                               addresses.end(),
                               std::back_inserter(sorted),
                               [](const pair &p) { return p; });
                std::sort(sorted.begin(),
                          sorted.end(),
                          [](const pair &x, const pair &y) { return x.second < y.second; });
                for(const auto &p : sorted) {
                    ofLogNotice("ofxRecordedOscPlayer") << p.first << " " << p.second;
                }
            }
            
            void play(double from_ms, double to_ms) const {
                auto from = std::lower_bound(messages.begin(),
                                             messages.end(),
                                             from_ms);
                auto to = std::upper_bound(messages.begin(),
                                           messages.end(),
                                           to_ms);
                for(auto it = from; it != to; ++it) {
                    auto &mess = it->mess;
                    ofxNotifyToSubscribedOsc(mess.getWaitingPort(), mess);
                }
            }
            
            double duration() const
            { return messages.empty() ? 0.0 : messages.back().offset; };
            
            double receivedFirstMessageAt() const
            { return messages.empty() ? 0.0 : messages.front().offset; };
            
            double receivedLastMessageAt() const
            { return messages.empty() ? 0.0 : messages[messages.size() - 1].offset; };
            
        protected:
            std::vector<SequenceData> messages;
            Metadata metadata;
            std::map<std::string, std::size_t> addresses;
        }; // struct Player
    }; // namespace OscRecorder
}; // namespace ofx

using ofxRecordedOscPlayer = ofx::RecordOsc::Player;

#endif /* ofxRecordedOscPlayer_h */
