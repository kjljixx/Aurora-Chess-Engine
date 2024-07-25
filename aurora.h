#pragma once
#include <map>

#define DATAGEN

#ifdef NO_DATAGEN
#undef DATAGEN
#endif

#define VERSION_NUM "mini-v1.17.0"
#define VERSION_NAME "-net-mini-4"
#ifdef DEV
#define DEV_STRING "-dev"
#else
#define DEV_STRING ""
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

  options["SyzygyPath"] = Option("<empty>", 2);

  //0: only output bestmove at end of search
  //1: ouput bestmove and info at end of search
  //2: output bestmove and info at end of search and output info every 2 seconds
  //3: output bestmove and info at end of search and output info + verbose move stats every 2 seconds
  options["outputLevel"] = Option(2, 0, 3, 1);

  options["explorationFactor"] = Option(0.04609718919, 0.001, 1024, 0);
  options["rootExplorationFactor"] = Option(0.09289019555, 0.001, 1024, 0);

  options["evalScaleFactor"] = Option(1, -1024, 1024, 0);

  options["searchTimePortion"] = Option(0.05, 0, 1, 0);
}
}
