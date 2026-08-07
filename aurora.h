#pragma once
#include <string>
#include <vector>

//Set to 1 if you want to build a version of Aurora which generates data, 2 for generating data while playing (cutechess), 0 for the normal version.
#define DATAGEN 0

#define VERSION_NUM "v1.27.0"
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
  bool hidden;

  Option(const std::string& name, float defaultValue, float minValue, float maxValue, int type, bool hidden = false) :
    name(name), defaultValue(defaultValue), minValue(minValue), maxValue(maxValue), value(defaultValue), type(type), hidden(hidden)
    {
      options.push_back(this);
    }
  Option(const std::string& name, const std::string& defaultValue, int type, bool hidden = false) :
    name(name), sDefaultValue(defaultValue), minValue(-1), maxValue(-1), sValue(defaultValue), defaultValue(0), value(0), type(type), hidden(hidden)
    {
      options.push_back(this);
    }
};

inline Option hash("Hash", 16, 0, 65536, 1);
inline Option ttHash("TTHash", 0, 0, 65536, 1);
inline Option threads("Threads", 1, 1, 1, 1); // just here to make OpenBench happy

inline Option syzygyPath("SyzygyPath", "<empty>", 2);

inline Option outputLevel("outputLevel", 2, -1, 3, 1);
  // -1: just search, don't output anything
  //  0: only output bestmove at end of search
  //  1: output bestmove and info at end of search
  //  2: output bestmove and info at end of search and output info every 2 seconds
  //  3: output bestmove and info at end of search and output info + verbose move stats every 2 seconds
inline Option rootStatsInterval("rootStatsInterval", 0, 0, 1000000, 1, true);
inline Option timeManager("timeManager", 0, 0, 3, 1);
// 0: normal time management
// 1: basic time management based on time left and increment only
// 2: normal time management with nodestime
// 3: basic time management with nodestime based on time left and increment only

inline Option moveOverhead("Move Overhead", 50, 0, 10000, 1);

inline Option rootExplorationFactor("rootExplorationFactor", 0.026337, 0.001, 1024, 0, true);
inline Option explorationFactor("explorationFactor", 0.014091, 0.001, 1024, 0, true);
inline Option valChangedMinWeight("valChangedMinWeight", 0.123489, 0.001, 1024, 0, true);
inline Option valSameMinWeight("valSameMinWeight", 0.007526, 0.001, 1024, 0, true);

inline Option varianceScaleMultiplier("varianceScaleMultiplier", 15.067323, 0, 1024, 0, true);
inline Option varianceScaleOffset("varianceScaleOffset", 0.006641, -1, 1, 0, true);
inline Option varianceScaleMin("varianceScaleMin", 1.001753, 0, 1024, 0, true);
inline Option varianceScaleMax("varianceScaleMax", 1.913014, 0, 1024, 0, true);

inline Option visitWindow("visitWindow", 0.034869, 0, 10, 0, true);

inline Option visitBoostMultiplier("visitBoostMultiplier", 1.117301, 0, 10, 0, true);
inline Option visitBoostOffset("visitBoostOffset", 0.000528, 0, 1, 0, true);
inline Option bestMoveChangesCoefficient("bestMoveChangesCoefficient", 0.224143, 0, 1024, 0, true);
inline Option bestMoveChangesExponent("bestMoveChangesExponent", 0.569136, 0, 16, 0, true);
inline Option bestMoveChangesMultiplierMin("bestMoveChangesMultiplierMin", 0.26941, 0, 1024, 0, true);
inline Option bestMoveChangesMultiplierMax("bestMoveChangesMultiplierMax", 1.946894, 0, 1024, 0, true);

inline Option timeManagementMovesLeft("timeManagementMovesLeft", 30.215695, 1, 200, 0, true);
inline Option timeManagementSoftFraction("timeManagementSoftFraction", 0.053364, 0, 1, 0, true);
inline Option timeManagementHardFraction("timeManagementHardFraction", 0.093877, 0, 1, 0, true);

inline Option ttHashProportion("ttHashProportion", 0.196714, 0, 1, 0, true);

inline Option lruPrunedVisitsEstimate("lruPrunedVisitsEstimate", 13.884551, 0, 1000, 0, true);

inline Option deltaMargin("deltaMargin", 100, 0, 2000, 0, true);

inline Option qSearchEvasionPlies("qSearchEvasionPlies", 1, 0, 10, 0, true);

inline Option cpMultiplier("cpMultiplier", 101.700963, 50.0, 150.0, 0, true);

inline Option* getOption(const std::string& name){
  for(Option* option : options){
    if(option->name == name) return option;
  }
  return nullptr;
}

}
