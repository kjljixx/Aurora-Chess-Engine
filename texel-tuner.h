#include "tuner/base.h"
#include "tuner/external/texelChess.hpp"
#include "texelTuneEval.h"
#include <fstream>
//#include "evaluation.h"

namespace texelTuner{
  class auroraEval
    {
    public:
        constexpr static bool includes_additional_score = false;
        constexpr static bool supports_external_chess_eval = false;

        static parameters_t get_initial_parameters();
        static EvalResult get_fen_eval_result(const std::string& fen);
        static EvalResult get_external_eval_result(const texelChess::Board& board);
        static EvalResult get_custom_board_representation_eval_result(const std::array<int8_t, 64>&);
        static std::array<int8_t, 64> get_custom_board_representation_from_fen(const std::string& fen);
        static void print_parameters(const parameters_t& parameters, std::string evalfilepath = "eval.auroraeval");
    };

  parameters_t auroraEval::get_initial_parameters(){
    parameters_t parameters;

    texelTuneEval::init();

    for(int i=0; i<64; i++){
      for(int j=0; j<13; j++){
        for(int k=i+1; k<64; k++){
          for(int l=0; l<13; l++){
            parameters.push_back(texelTuneEval::doublePiecePairTable[i][j][k][l]);
          }
        }
      }
    }

    return parameters;
  }

  EvalResult auroraEval::get_fen_eval_result(const std::string& fen){
    chess::Board board(fen);

    // std::cout << "NEWEVAL:";
    std::vector<int> coefficients = texelTuneEval::evaluate(board);
    EvalResult result;
    //std::cout << "COEFF:";
    result.coefficients = coefficients;
    result.endgame_scale = 1;
    //result.score = trace.score;
    return result;
  }

  EvalResult auroraEval::get_custom_board_representation_eval_result(const std::array<int8_t, 64>& board){
    // std::cout << "NEWEVAL:";
    std::vector<int> coefficients = texelTuneEval::evaluate(board);
    EvalResult result;
    //std::cout << "COEFF:";
    result.coefficients = coefficients;
    result.endgame_scale = 1;
    //result.score = trace.score;
    return result;
  }
  std::array<int8_t, 64> auroraEval::get_custom_board_representation_from_fen(const std::string& fen){
    chess::Board board(fen);

    std::array<int8_t, 64> customRep;
    for(int i=0; i<64; i++){
      customRep[i] = board.mailbox[board.sideToMove][i];
    }

    return customRep;
  }
  void auroraEval::print_parameters(const parameters_t& parameters, std::string evalfilepath){
    std::ofstream evalfile;
    evalfile.open(evalfilepath);

    evalfile << "1 ";
    int iter = 0;
    for(int i=0; i<64; i++){
      for(int j=0; j<13; j++){
        for(int k=i+1; k<64; k++){
          for(int l=0; l<13; l++){
            evalfile //<< iter << ":" 
            << parameters[iter] << " ";
            iter++;
          }
        }
      }
    }
    evalfile.close();
  }
  
  int checkEval(const std::string& fen){
    return true;
  }
}