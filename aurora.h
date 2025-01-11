#pragma once
#include <map>

//Set to 1 if you want to build a version of Aurora which generates data, 2 for generating data while playing (cutechess), 0 for the normal version.
#define DATAGEN 0


#define VERSION_NUM "v1.25.0"
#define VERSION_NAME "-tt"
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

struct Option{
  float defaultValue;
  float minValue;
  float maxValue;
  float value;

  //only used when the option is a string (type = 2)
  std::string sDefaultValue;
  std::string sValue;
  
  int type; //0 = string (which aurora uses for floats), 1 = spin (an int), 2 = string (an actual string)

  Option(float defaultValue, float minValue, float maxValue, int type) :
    defaultValue(defaultValue), minValue(minValue), maxValue(maxValue), value(defaultValue), type(type) {}
  Option(std::string defaultValue, int type) :
    minValue(-1), maxValue(-1), sDefaultValue(defaultValue), sValue(defaultValue), type(type) {}
  Option() :
    defaultValue(0), minValue(0), maxValue(0), value(0), type(0) {}
};

std::map<std::string, Option> options;

void initOptions(){
  options["Hash"] = Option(0, 0, 65536, 1);
  options["TTHash"] = Option(16, 0, 65536, 1);
  options["Threads"] = Option(1, 1, 1, 1); //just here to make OpenBench happy

  options["SyzygyPath"] = Option("<empty>", 2);

  options["outputLevel"] = Option(2, 0, 3, 1);
  //0: only output bestmove at end of search
  //1: ouput bestmove and info at end of search
  //2: output bestmove and info at end of search and output info every 2 seconds
  //3: output bestmove and info at end of search and output info + verbose move stats every 2 seconds

  options["rootExplorationFactor"] = Option(0.06906412698639361, 0.001, 1024, 0);
  options["explorationFactor"] = Option(0.0395545614187926, 0.001, 1024, 0);
  options["valChangedMinWeight"] = Option(0.1560282435642479, 0.001, 1024, 0);
  options["valSameMinWeight"] = Option(0.015277783601196866, 0.001, 1024, 0);
  options["biasStartingWeight"] = Option(82.28227020619, 0.001, 1024, 0);
}

}