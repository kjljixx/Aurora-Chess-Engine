#pragma once
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#ifdef DEV

namespace search{

enum class StopReason : uint8_t{
  None = 0,
  Soft,
  Hard,
  Nodes,
  Iters,
  Forever,
  Time
};

inline const char* stopReasonName(StopReason r){
  switch(r){
    case StopReason::Soft: return "soft";
    case StopReason::Hard: return "hard";
    case StopReason::Nodes: return "nodes";
    case StopReason::Iters: return "iters";
    case StopReason::Forever: return "forever";
    case StopReason::Time: return "time";
    default: return "none";
  }
}

inline const char* rootChildrenCsvPath = "search_stats_root_children.csv";

struct SearchStats{
  uint64_t playoutTerminal = 0;
  uint64_t playoutTb = 0;
  uint64_t playoutTtHit = 0;
  uint64_t playoutEval = 0;

  uint64_t ttStores = 0;
  uint64_t ttCollisions = 0;

  uint64_t lruEvictions = 0;
  uint64_t lruPrunedEdgeSelects = 0;

  uint64_t backpropValueChanged = 0;
  uint64_t backpropVisitOnly = 0;

  uint64_t rootQADisagree = 0;
  uint64_t rootQADisagreeChecks = 0;

  uint64_t visitWindowVisitsSum = 0;
  uint64_t visitWindowIters = 0;
  uint64_t tmBestMoveChanges = 0;
  float tmSoftMultiplier = 1.0f;

  StopReason stopReason = StopReason::None;
  uint64_t treeReuseNodesKept = 0;
  uint64_t treeReuseEvents = 0;
  uint64_t destroyTreeEvents = 0;

  uint64_t expandBranchingSum = 0;
  uint64_t expandBranchingCount = 0;
  uint64_t effectiveBranchingSum = 0;
  uint64_t effectiveBranchingCount = 0;

  uint64_t qsearchNodes = 0;
  uint64_t qsearchDeltaPrune = 0;
  uint64_t qsearchSeePrune = 0;
  uint64_t qsearchBetaCut = 0;
  uint64_t qsearchMaxPly = 0;
  uint64_t pathDepth0_4 = 0;
  uint64_t pathDepth5_8 = 0;
  uint64_t pathDepth9_16 = 0;
  uint64_t pathDepth17Plus = 0;

  std::ofstream rootChildrenCsv;

  void reset(){
    std::ofstream keepCsv = std::move(rootChildrenCsv);
    *this = SearchStats{};
    rootChildrenCsv = std::move(keepCsv);
  }

  bool beginRootChildrenDump(const char* path = rootChildrenCsvPath){
    endRootChildrenDump();
    rootChildrenCsv.open(path, std::ios::out | std::ios::trunc);
    if(!rootChildrenCsv){
      std::cout << "info string searchstats failed to open " << path << "\n";
      return false;
    }
    rootChildrenCsv << "root_iters,root_visits,move,q,avg,iters,visits,variance,std_dev,pruned\n";
    return true;
  }

  void writeRootChildRow(
    int rootIters,
    uint32_t rootVisits,
    const std::string& move,
    float q,
    float avg,
    int iters,
    uint32_t visits,
    float variance,
    float stdDev,
    int pruned
  ){
    if(!rootChildrenCsv.is_open()) return;
    rootChildrenCsv << rootIters << ','
                    << rootVisits << ','
                    << move << ','
                    << q << ','
                    << avg << ','
                    << iters << ','
                    << visits << ','
                    << variance << ','
                    << stdDev << ','
                    << pruned << '\n';
  }

  void endRootChildrenDump(){
    if(rootChildrenCsv.is_open()){
      rootChildrenCsv.flush();
      rootChildrenCsv.close();
    }
  }

  void recordPathDepth(int depth){
    if(depth <= 4) pathDepth0_4++;
    else if(depth <= 8) pathDepth5_8++;
    else if(depth <= 16) pathDepth9_16++;
    else pathDepth17Plus++;
  }

  void recordTtStore(uint32_t existingHash, float existingVal, uint32_t newHashHi){
    if(existingVal != -2 && existingHash != newHashHi){
      ttCollisions++;
    }
    ttStores++;
  }

  void print() const{
    const uint64_t playoutTotal = playoutTerminal + playoutTb + playoutTtHit + playoutEval;
    const double visitWindowMean = visitWindowIters
      ? double(visitWindowVisitsSum) / double(visitWindowIters)
      : 0.0;
    const double expandBranchingMean = expandBranchingCount
      ? double(expandBranchingSum) / double(expandBranchingCount)
      : 0.0;
    const double effectiveBranchingMean = effectiveBranchingCount
      ? double(effectiveBranchingSum) / double(effectiveBranchingCount)
      : 0.0;

    std::cout << "info string searchstats playout"
              << " terminal=" << playoutTerminal
              << " tb=" << playoutTb
              << " ttHit=" << playoutTtHit
              << " eval=" << playoutEval
              << " total=" << playoutTotal << "\n";
    std::cout << "info string searchstats tt"
              << " stores=" << ttStores
              << " collisions=" << ttCollisions << "\n";
    std::cout << "info string searchstats lru"
              << " evictions=" << lruEvictions
              << " prunedEdgeSelects=" << lruPrunedEdgeSelects << "\n";
    std::cout << "info string searchstats backprop"
              << " valueChanged=" << backpropValueChanged
              << " visitOnly=" << backpropVisitOnly << "\n";
    std::cout << "info string searchstats root"
              << " qaDisagree=" << rootQADisagree
              << " qaChecks=" << rootQADisagreeChecks << "\n";
    std::cout << "info string searchstats branching"
              << " expandMean=" << expandBranchingMean
              << " expandSamples=" << expandBranchingCount
              << " effectiveMean=" << effectiveBranchingMean
              << " effectiveSamples=" << effectiveBranchingCount << "\n";
    std::cout << "info string searchstats visitWindow"
              << " visitsSum=" << visitWindowVisitsSum
              << " iters=" << visitWindowIters
              << " meanVisits=" << visitWindowMean << "\n";
    std::cout << "info string searchstats tm"
              << " bestMoveChanges=" << tmBestMoveChanges
              << " softMultiplier=" << tmSoftMultiplier
              << " stopReason=" << stopReasonName(stopReason) << "\n";
    std::cout << "info string searchstats reuse"
              << " nodesKept=" << treeReuseNodesKept
              << " reuseEvents=" << treeReuseEvents
              << " destroyEvents=" << destroyTreeEvents << "\n";
    std::cout << "info string searchstats qsearch"
              << " nodes=" << qsearchNodes
              << " deltaPrune=" << qsearchDeltaPrune
              << " seePrune=" << qsearchSeePrune
              << " betaCut=" << qsearchBetaCut
              << " maxPly=" << qsearchMaxPly << "\n";
    std::cout << "info string searchstats pathDepth"
              << " 0-4=" << pathDepth0_4
              << " 5-8=" << pathDepth5_8
              << " 9-16=" << pathDepth9_16
              << " 17+=" << pathDepth17Plus << "\n";
  }
};

inline thread_local SearchStats* g_searchStats = nullptr;

} // namespace search

#define SEARCH_STAT(field) do { if (::search::g_searchStats) ++(::search::g_searchStats->field); } while(0)
#define SEARCH_STAT_ADD(field, n) do { if (::search::g_searchStats) (::search::g_searchStats->field) += (n); } while(0)
#define SEARCH_STAT_MAX(field, n) do { if (::search::g_searchStats && (n) > ::search::g_searchStats->field) ::search::g_searchStats->field = (n); } while(0)

#else // !DEV

#define SEARCH_STAT(field) ((void)0)
#define SEARCH_STAT_ADD(field, n) ((void)0)
#define SEARCH_STAT_MAX(field, n) ((void)0)

#endif // DEV
