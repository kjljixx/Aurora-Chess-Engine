#pragma once
#include <string>
#include <vector>

//Set to 1 if you want to build a version of Aurora which generates data, 2 for generating data while playing (cutechess), 0 for the normal version.
#define DATAGEN 0

#define VERSION_NUM "v1.26.1"
#define VERSION_NAME "-nodestime"
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

inline std::vector<Option*> options;

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

inline Option hash("Hash", 0, 0, 65536, 1);
inline Option ttHash("TTHash", 0, 0, 65536, 1);
inline Option threads("Threads", 1, 1, 1, 1); // just here to make OpenBench happy

inline Option syzygyPath("SyzygyPath", "<empty>", 2);

inline Option outputLevel("outputLevel", 2, -1, 3, 1);
// -1: just search, don't output anything
//  0: only output bestmove at end of search
//  1: output bestmove and info at end of search
//  2: output bestmove and info at end of search and output info every 2 seconds
//  3: output bestmove and info at end of search and output info + verbose move stats every 2 seconds

inline Option rootExplorationFactor("rootExplorationFactor", 0.026, 0.001, 1024, 0);
inline Option explorationFactor("explorationFactor", 0.015, 0.001, 1024, 0);
inline Option valChangedMinWeight("valChangedMinWeight", 0.1560282435642479, 0.001, 1024, 0);
inline Option valSameMinWeight("valSameMinWeight", 0.015277783601196866, 0.001, 1024, 0);

inline Option varianceScaleMultiplier("varianceScaleMultiplier", 16, 0, 1024, 0);
inline Option varianceScaleOffset("varianceScaleOffset", 0.00625, -1, 1, 0);
inline Option varianceScaleMin("varianceScaleMin", 1.0, 0, 1024, 0);
inline Option varianceScaleMax("varianceScaleMax", 2.0, 0, 1024, 0);

inline Option visitWindow("visitWindow", 0.04, 0, 10, 0);

inline Option bestMoveChangesCoefficient("bestMoveChangesCoefficient", 0.26061644, 0, 1024, 0);
inline Option bestMoveChangesExponent("bestMoveChangesExponent", 0.54, 0, 16, 0);
inline Option bestMoveChangesMultiplierMin("bestMoveChangesMultiplierMin", 0.2, 0, 1024, 0);
inline Option bestMoveChangesMultiplierMax("bestMoveChangesMultiplierMax", 2.0, 0, 1024, 0);

// 0: normal time management
// 1: basic time management based on time left and increment only
// 2: normal time management with nodestime
// 3: basic time management with nodestime based on time left and increment only
inline Option timeManager("timeManager", 0, 0, 3, 1);
inline Option timeManagementMovesLeft("timeManagementMovesLeft", 30, 1, 200, 1);
inline Option timeManagementSoftFraction("timeManagementSoftFraction", 0.05, 0, 1, 0);
inline Option timeManagementHardFraction("timeManagementHardFraction", 0.1, 0, 1, 0);

inline Option* getOption(std::string name){
  for(Option* option : options){
    if(option->name == name) return option;
  }
  return nullptr;
}

}
