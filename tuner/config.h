#ifndef CONFIG_H
#define CONFIG_H 1

#include<cstdint>
#include "C:\Users\kjlji\OneDrive\Documents\VSCode\C++\AuroraChessEngine-main\texel-tuner.h"

#define TAPERED 0
//using TuneEval = Toy::ToyEval;
//using TuneEval = Toy::ToyEvalTapered;
using TuneEval = texelTuner::auroraEval;
constexpr int32_t data_load_thread_count = 8;
constexpr int32_t thread_count = 8;
constexpr tune_t preferred_k = 2.1;
constexpr int32_t max_epoch = 5001;
constexpr bool retune_from_zero = true;
constexpr bool enable_qsearch = false;
constexpr tune_t initial_learning_rate = 0.01;
constexpr int32_t learning_rate_drop_interval = 3000;
constexpr tune_t learning_rate_drop_ratio = 0.25;
constexpr bool print_data_entries = false;
constexpr int32_t data_load_print_interval = 10000;

#endif // !CONFIG_H
