#pragma once
#include <map>

//Set to 1 if you want to build a version of Aurora which generates data, 2 for generating data while playing (cutechess), 0 for the normal version.
#define DATAGEN 0


#define VERSION_NUM "v1.17.1"
#define VERSION_NAME "-syzygy"
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
  options["Threads"] = Option(1, 1, 1, 1); //just here to make OpenBench happy

  options["SyzygyPath"] = Option("<empty>", 2);

  options["outputLevel"] = Option(2, 0, 3, 1);
  //0: only output bestmove at end of search
  //1: ouput bestmove and info at end of search
  //2: output bestmove and info at end of search and output info every 2 seconds
  //3: output bestmove and info at end of search and output info + verbose move stats every 2 seconds

  options["explorationFactor"] = Option(0.09, 0.001, 1024, 0);
  options["rootExplorationFactor"] = Option(0.17, 0.001, 1024, 0);

  options["evalScaleFactor"] = Option(1, -1024, 1024, 0);

  options["searchTimePortion"] = Option(0.05, 0, 1, 0);
}

}