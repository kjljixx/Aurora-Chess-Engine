#pragma once
#include <map>

//Set to 1 if you want to build a version of Aurora which generates data, 2 for generating data while playing (cutechess), 0 for the normal version.
#define DATAGEN 0

#define VERSION "v1.11.4"

namespace Aurora{

struct Option{
  float defaultValue;
  float minValue;
  float maxValue;
  int type; //0 = string, 1 = spin
  float value;

  Option(float defaultValue, float minValue, float maxValue, int type) :
    defaultValue(defaultValue), minValue(minValue), maxValue(maxValue), type(type), value(defaultValue) {}
  Option() :
    defaultValue(0), minValue(0), maxValue(0), type(0), value(0) {}
};

std::map<std::string, Option> options;

void initOptions(){
  options["outputLevel"] = Option(2, 0, 3, 1);
  //0: only output bestmove at end of search
  //1: ouput bestmove and info at end of search
  //2: output bestmove and info at end of search and output info every 2 seconds
  //3: output bestmove and info at end of search and output info + verbose move stats every 2 seconds

  options["explorationFactor"] = Option(0.13170624986905002, 0.001, 1024, 0);
  options["rootExplorationFactor"] = Option(0.2654005671036296, 0.001, 1024, 0);

  options["evalScaleFactor"] = Option(1, -1024, 1024, 0);

  options["searchTimePortion"] = Option(0.05, 0, 1, 0);

  options["nodeLimit"] = Option(4294967296, 1, 4294967296, 1)
}

}
