#ifndef REPLAY_GLOBAL_H
#define REPLAY_GLOBAL_H

#include <string>
#include <sstream>
#include <boost/filesystem.hpp>

// Input recording system
namespace ticpp { class Element; }
using ticpp::Element;

extern bool recording;
extern bool replaying;

extern bool init_action_log(std::string command, std::string file);
extern void record_action(std::string action_type, std::string inner_text);
extern bool has_next_action();
extern Element* pop_next_action(std::string expected_action_type="");
extern std::string encode_file(fs::path file);
extern void decode_file(std::string data, fs::path file);

#endif