#ifndef FOURKU_H
#define FOURKU_H 1

#include "../base.h"
#include "../external/texelChess.hpp"
#include <string>
#include <vector>

namespace Fourku
{
    class FourkuEval
    {
    public:
        constexpr static bool includes_additional_score = true;
        constexpr static bool supports_external_chess_eval = true;

        static parameters_t get_initial_parameters();
        static EvalResult get_fen_eval_result(const std::string& fen);
        static EvalResult get_external_eval_result(const chess::Board& board);
        static void print_parameters(const parameters_t& parameters);
    };
}

#endif // 4KU_H
