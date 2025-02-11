#pragma once
#include <vector>

//Set to 1 if you want to build a version of Aurora which generates data, 2 for generating data while playing (cutechess), 0 for the normal version.
#define DATAGEN 0

#define VERSION_NUM "v1.26.0"
#define VERSION_NAME ""
#ifdef DEV
#define DEV_STRING "-dev"
#else
#define DEV_STRING ""
#endif
#ifdef GIT_HASH
#define GIT_HASH_STRING GIT_HASH
#else
#define GIT_HASH_STRING "N/A"
#endif

namespace Aurora{

struct Option;

std::vector<Option*> options;

struct Option{
  std::string name;
  float defaultValue;
  float minValue;
  float maxValue;
  float value;

  //only used when the option is a string (type = 2)
  std::string sDefaultValue;
  std::string sValue;
  
  int type; //0 = string (which aurora uses for floats), 1 = spin (an int), 2 = string (an actual string)

  Option(std::string name, float defaultValue, float minValue, float maxValue, int type) :
    name(name), defaultValue(defaultValue), minValue(minValue), maxValue(maxValue), value(defaultValue), type(type)
    {
      options.push_back(this);
    }
  Option(std::string name, std::string defaultValue, int type) :
    name(name), sDefaultValue(defaultValue), minValue(-1), maxValue(-1), sValue(defaultValue), type(type)
    {
      options.push_back(this);
    }
};

Option hash("Hash", 0, 0, 65536, 1);
Option ttHash("TTHash", 0, 0, 65536, 1);
Option threads("Threads", 1, 1, 1, 1); // just here to make OpenBench happy

Option syzygyPath("SyzygyPath", "<empty>", 2);

Option outputLevel("outputLevel", 2, -1, 3, 1);
// -1: just search, don't output anything
//  0: only output bestmove at end of search
//  1: output bestmove and info at end of search
//  2: output bestmove and info at end of search and output info every 2 seconds
//  3: output bestmove and info at end of search and output info + verbose move stats every 2 seconds

Option rootExplorationFactor("rootExplorationFactor", 0.06906412698639361, 0.001, 1024, 0);
Option explorationFactor("explorationFactor", 0.0395545614187926, 0.001, 1024, 0);
Option valChangedMinWeight("valChangedMinWeight", 0.1560282435642479, 0.001, 1024, 0);
Option valSameMinWeight("valSameMinWeight", 0.015277783601196866, 0.001, 1024, 0);

Option* getOption(std::string name){
  for(Option* option : options){
    if(option->name == name) return option;
  }
  return nullptr;
}

}
