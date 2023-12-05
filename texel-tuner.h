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
        static void get_custom_board_representation_eval_result(const std::array<int8_t, 64>& board, std::vector<int>& coefficients);
        static std::array<int8_t, 64> get_custom_board_representation_from_fen(const std::string& fen);
        static void print_parameters(const parameters_t& parameters, std::string evalfilepath = "eval.auroraeval");
    };

  parameters_t auroraEval::get_initial_parameters(){
    parameters_t parameters;

    texelTuneEval::init();

    for(int i=0; i<6; i++){
      //int pair = texelTuneEval::mg_value[i];
      pair_t pair = {texelTuneEval::mg_value[i], texelTuneEval::eg_value[i]};
      parameters.push_back(pair);
    }
    for(int i=0; i<64; i++){
      for(int j=0; j<13; j++){
        for(int k=i+1; k<64; k++){
          for(int l=0; l<13; l++){
            //int pair = texelTuneEval::doublePiecePairTable[i][j][k][l];
            pair_t pair = {texelTuneEval::doublePiecePairTable[0][i][j][k][l], texelTuneEval::doublePiecePairTable[1][i][j][k][l]};
            parameters.push_back(pair);
          }
        }
      }
    }

    return parameters;
  }

  EvalResult auroraEval::get_fen_eval_result(const std::string& fen){
    chess::Board board(fen);

    // std::cout << "NEWEVAL:";
    std::vector<int> coefficients = texelTuneEval::fullboardSEE(board);
    EvalResult result;
    //std::cout << "COEFF:";
    result.coefficients = coefficients;
    result.endgame_scale = 1;
    //result.score = trace.score;
    return result;
  }

  void auroraEval::get_custom_board_representation_eval_result(const std::array<int8_t, 64>& board, std::vector<int>& coefficients){
    // std::cout << "NEWEVAL:";
    texelTuneEval::evaluate(board, coefficients);
    return;
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

    evalfile << "2 ";
    for(int a=0; a<2; a++){
      for(int i=0; i<6; i++){
        evalfile << std::fixed << parameters[i][a] << " ";
      }
    }
    int iter = 6;
    for(int a=0; a<2; a++){
      iter = 6;
      for(int i=0; i<64; i++){
        for(int j=0; j<13; j++){
          for(int k=i+1; k<64; k++){
            for(int l=0; l<13; l++){
              evalfile << std::fixed//<< iter << ":"
              << parameters[iter][a] << " ";
              iter++;
            }
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