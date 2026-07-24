# Aurora Playing-Strength Knowledge Base

This document summarizes what OpenBench progression/feature testing has taught us about making Aurora stronger. It is meant for someone new to the project who wants to ideate on Elo gains—not as a raw test log.

Source data: `openbench_summary.txt` (OpenBench tests ~#8–#128). Elo figures are approximate SPRT/match estimates; treat them as directional evidence, not precise ratings.

## How to read this

- **PASS / FAIL** means SPRT accepted or rejected the patch vs its base (usually `main`).
- Bounds like `[0, 10]` are “is this a gain?” tests. Bounds like `[-10, 0]` are non-regression (“is this not a loss?”).
- Many tests were **manually stopped early** or used **buggy implementations**. A red Elo number does **not** always mean the idea is dead—especially when losses are catastrophic (−100 to −1200), which usually indicates a bug, not a refined negative result.
- Aurora is an **MCTS/UCT + minimax hybrid** with **NNUE**. Ideas that work in Stockfish-style alpha-beta often need a different shape here (and vice versa).

## Architecture context (why some ideas matter more)

Aurora builds a search tree, selects edges with a UCT-like priority, evaluates leaves (qsearch + NNUE, TT, tablebases), and **backpropagates with minimax** (parent gets the best child value), while also maintaining **running averages (`avgValue`)** used in selection and best-move choice.

Practical implications:

1. **Nodes / memory efficiency ≈ strength.** More useful tree under the same hash/time budget is a first-class Elo lever.
2. **Value quality beats visit quantity** when the two trade off. How Q, averages, and TT values interact has decided several large tests.
3. **UCT constants are not cosmetic.** Exploration, variance scaling, visit boosts, and time management have repeatedly paid for SPSA work.
4. **Classical pruning is dangerous** until proven. Throwing away edges changes the tree topology that UCT depends on.

---

## What has made Aurora stronger

### 1. SPSA / parameter tuning (consistent, large ROI)

Repeatedly among the strongest patches. Tuned defaults for exploration, value EMA floors, variance scaling, visit boosts, time management, TT/hash split, LRU visit estimates, and CP↔value scaling have passed multiple times, e.g.:

| Test | Result | Notes |
|------|--------|--------|
| #120 spsa | **+19.3** PASS | LTC-ish 20s+0.2, Hash=32; exposed more tunables |
| #47 / #46 spsa | **+13.3 / +9.8** PASS | Tuned params vs main |
| #108 / #118 spsa | **+7.2 / +6.3** PASS | STC; includes new options (`ttHashProportion`, `lruPrunedVisitsEstimate`, `cpMultiplier`) |
| #37 spsa | **+8.9** PASS | spsa vs spsa progression |

**Takeaway:** When a behavior is controlled by a magic constant, promote it to a UCI option and SPSA it. Small structural hooks + tuning often beat clever one-shot formulas.

However, tunes of the standard variables are typically most effective after many other changes to the engine have been made since the last tune.

**Ideation:** new scalars in selection, backpropagation weighting, time management, TT replacement, and eval scaling are high-probability Elo work.

### 2. Using averages (`avgValue`) for decisions (big structural wins)

Aurora does not only use pure minimax edge values. Branches that leaned harder on averaged values gained a lot:

| Test | Result | Idea |
|------|--------|------|
| #88 / #86 lru-avg-val | **+47.9 / +34.5** PASS | Average-aware handling around LRU-pruned tree nodes (LTC/STC) |
| #92 / #91 choose-avg-val | **+24.1 / +12.0** PASS | Prefer average value when choosing the played move |

Today, bestmove selection uses average-aware logic (`findBestAEdge`), while info/PV paths still lean on minimax Q (It's worth saying that info/PV paths do NOT influence the actual moves played in the tests or otherwise influence the results of the test)

**Takeaway:** In this hybrid, **smoothed values help selection/play**; raw minimax Q alone is not always best for the move you output.

**Ideation:** better blends of Q vs average; visit-/variance-gated trust in averages; better recovery after LRU compaction. Avoid “only use averages when reliable” without care (#78 failed hard—see below).

### 3. Tree representation / memory efficiency (`index-as-pointers`)

Replacing `Node*` child pointers with `uint32_t` indices (arena/index tree) was a clear win, especially when hash was tiny:

| Test | Result | Notes |
|------|--------|--------|
| #101 | **+31.6** PASS | Hash=1 — memory pressure makes the win obvious |
| #103 / #90 | non-reg PASS | Not a loss at normal hash |
| #89 | ~+2 stopped after huge sample | Neutral-to-slightly-positive at Hash=1, `[0,5]` |

**Takeaway:** For MCTS engines, **bytes per node and cache behavior are Elo features**.

**Ideation:** denser nodes, better packing, smarter LRU, more tree under the same `Hash`, fewer pointer chases.

### 4. Transposition-table reuse (`reuse-tt`)

| Test | Result |
|------|--------|
| #35 / #36 reuse-tt | **+30.6 / +19.1** PASS |

Keeping/reusing TT information across searches (rather than throwing it away) was a large gain.

**Takeaway:** Leaf/eval caching is high value; anything that improves TT hit quality or lifetime is promising.

**Failed related ideas (do not confuse with this win):** writing averages into TT instead of the current value (`tt-avgs` #128 −8 FAIL), and poking TT from qsearch (`qsearch-use-tt` failed). Reuse helped; *what* you store and *when* you probe still matter.

For qsearch-use-tt specifically, a couple extra auxiliary data points:
BARELY ever gets any tt hits
All tests are 1m nodes from startpos:

NO UPDATING QSEARCH FROM TT:
qSearch tt hits: 100147 hits/18542024 searches

playout tt hits: 3977335 hits/11451348 searches

UPDATING QSEARCH FROM TT:

1050529/25907636
3107183/12718223

### 5. Visit / continuation boost in UCT (`cont-boost`)

| Test | Result |
|------|--------|
| #70 / #71 cont-boost | **+5.2 / +10.7** PASS |

Selection boost for under-visited children relative to parent visits (today’s `visitBoostMultiplier` / `visitBoostOffset`) passed STC and LTC.

**Takeaway:** Mild, tuned corrections to the visit term help; aggressive redesigns of the visit denominator often do not (see failures).

### 6. Correctness fixes (small but real)

| Test | Result | Theme |
|------|--------|--------|
| #34 en-passant-hash-fix | non-reg PASS (~+2) | Hash correctness |
| #31 3f-fix-2 | non-reg PASS (~+5) | Threefold / repetition logic |

**Takeaway:** Silent search/hash bugs are free Elo when found. Prefer targeted non-regression SPRTs for fixes.

### 7. Eval numerics (`float-cp`) — conditional win

| Test | Result | TC |
|------|--------|-----|
| #107 float-cp | **+5.8** PASS | Fixed nodes `N=1` |
| #105 / #109 | FAIL ~−7 to −9 | Timed / `N=100` |

Keeping float precision through NNUE/qsearch/CP conversion helped in a nodes-equal setting but did not automatically translate to all match conditions. Treat as “promising but sensitive,” not a blank check.

### 8. Cumulative product progress

Fixed-game matches of newer `main` vs an older baseline (`C0E67236…`) showed large positive Elo (~+32 to +86 depending on hash/TC: #110, #112, #113, #123). Individual patches compound.

---

## What has not worked (or not yet)

Interpret catastrophic Elo as “implementation/bug until proven otherwise.” Mild fails with large samples are more informative.

### Clear / repeated negatives

| Theme | Evidence | Reading |
|-------|----------|---------|
| **`child-visits-pow` / `child-visits-pow-2`** | Many fails, often −50 to −90; even node-equal tests weak or negative | Changing the visit-term exponent / power away from the established `1/sqrt(visits)` shape is a minefield |
| **`top2-eq-explore`** | −200 to −400 fails | Forcing equal exploration of top-2 moves was disastrous (bug and/or bad policy) |
| **`only-use-reliable-avgs`** | #78 **−32** FAIL | Gating averages too aggressively hurt; contrast with successful avg usage above |
| **`tt-avgs`** | #128 **−8** FAIL | Storing `avgValue` into TT instead of current edge value did not help |
| **`qsearch-use-tt`** | #95/#96 FAIL | Using TT inside qsearch was not a free win |
| **`better-avgs` (EMA tables)** | Multiple FAIL / near-zero STOP | Fancier EMA weight schedules did not beat the simple clamp/`1/iters` style on STC |

### Likely-buggy or unfinished (idea not necessarily dead)

| Theme | Evidence | Reading |
|-------|----------|---------|
| **`threading` / SMP** | #126/−1200, #127/−109 even with Threads=1 in some runs | Shared-TT / multi-tree WIP; **single-thread regressions mean the patch was not ready**. Multi-thread scaling remains an open strength opportunity if correctness/NPS hold |
| **`prune-bad-see`** | Several −1000-class results; later #59 only ~−14 stopped | Early versions almost certainly buggy. Soft SEE ideas may still be worth a careful, non-destructive retry |
| **Hash/branch `4783CD0CCAB56030`** | ~−60 FAIL | Treat as a bad snapshot/bug, not a theme rejection |

### Inconclusive / time-control sensitive

| Theme | Evidence | Reading |
|-------|----------|---------|
| **`boost-explore`** | STC FAIL −2.5; LTC STOP **+7.2** (#66) | May help longer TCs; needs a clean LTC SPRT |
| **`child-variance`** | vs main mostly FAIL; one internal STOP +6.9 | Parent **variance scaling** later became a tuned mainline feature via SPSA—so “use variance” is not dead; that specific child-variance patch series was |
| **`explore-update-nnue`** | STOP −17 (STC) vs STOP +10 (ultra-fast) | Inconclusive; NNUE update strategy during explore needs a cleaner experiment |
| **`cache-inv-visits-pow`** | Long STOPs near 0 | No clear gain |
| Many empty / SPSA self-tests | 0 games | Ignore for ideation |

---

## Cross-cutting lessons

1. **Tune before reinventing.** SPSA on selection/time/eval scalars is Aurora’s most reliable Elo factory.
2. **Hybrid search has hybrid values.** Minimax Q, running averages, and TT values solve different jobs. Mixing them carefully has won Elo; naive “always use X” has lost Elo.
3. **Preserve the visit-term shape** unless you have a strong LTC result. Power-law experiments failed often.
4. **Memory and reuse beat exotic pruning** so far (indices, TT reuse, LRU-aware averages).
5. **Bugs look like “ideas don’t work.”** −1000 Elo, Threads=1 losses for “SMP,” and SEE prune wipeouts should not close a research avenue by themselves.
6. **STC ≠ LTC.** Exploration/boost ideas have flipped sign with time control; verify important ideas at both.
7. **Node-equal vs wall-clock.** `float-cp` shows a change can pass fixed-nodes and fail timed games (or the reverse). Know which question you asked.

---

## Ideation guide for newcomers

High-probability directions (aligned with past wins):

1. **More tunable structure + SPSA** — any new heuristic with constants should be optionized early.
2. **Richer value estimates** — better Q/average blends; variance-aware trust; improving what happens after LRU prune (without repeating #78’s over-gating).
3. **Tree/TT efficiency** — denser nodes, better replacement, smarter reuse, hash split (`ttHashProportion`-style) experiments.
4. **Time management** — already tunable and Elo-relevant; remaining soft/hard fraction behavior, sudden death, best-move instability (`bestMoveChanges*`) are natural targets.
5. **NNUE / datagen quality** — these OpenBench rows are mostly search/tuning; network quality is still one of the largest external levers (see README / `andromeda` notes).
6. **Correctness & hashing** — cheap Elo when you find real bugs.

Promising but needs careful methodology:

1. **SMP** — huge ceiling if shared info is correct and single-thread isn’t regressed.
2. **LTC exploration tweaks** (`boost-explore`-class) — retest cleanly at long TC.
3. **Non-destructive move filtering** — SEE/history as ordering or prior bias rather than hard prune.
4. **Incremental / smarter NNUE updates during tree walk** — prior tests inconclusive.

Lower priority unless you have a new angle:

1. Replacing `1/sqrt(visits)` with ad-hoc powers.
2. Forcing top-N moves to share exploration equally.
3. Dumping averages indiscriminately into TT.
4. Copying alpha-beta pruning recipes without an MCTS-specific design.

### Suggested experiment hygiene

- One idea per test; keep diffs small.
- If Elo < −100 quickly, assume a bug before abandoning the concept.
- Prefer `[0,10]` for gains and `[-10,0]` for refactors/speed/index changes.
- Retest TC-sensitive ideas at both STC and LTC.
- After structural hooks land, schedule SPSA before declaring the idea “done.”

---

## Theme index (quick reference)

| Theme / branch family | Verdict | Strength signal |
|-----------------------|---------|-----------------|
| spsa / option exposure | **Keep doing** | Multiple +6 to +19 passes |
| choose-avg-val / lru-avg-val | **Core win** | +12 to +48 |
| reuse-tt | **Core win** | +19 to +31 |
| index-as-pointers | **Core win** | Strong at low hash; non-reg elsewhere |
| cont-boost (visit boost) | **Win** | +5 to +11 |
| correctness (EP hash, 3fold) | **Win** | Small non-reg passes |
| float-cp | **Mixed** | Pass nodes-equal; fail some timed |
| better-avgs / EMA schedules | **Weak so far** | Fails / noise |
| tt-avgs, qsearch-use-tt | **Weak so far** | Mild–moderate fails |
| only-use-reliable-avgs | **Weak** | −32 |
| child-visits-pow* | **Avoid default** | Repeated large fails |
| top2-eq-explore | **Avoid default** | Catastrophic |
| prune-bad-see | **Retry only carefully** | Likely buggy history |
| threading | **Open problem** | Implementation not ready |
| boost-explore / explore-update-nnue / child-variance patches | **Revisit with care** | TC-sensitive or superseded by later tuning |

---

## Appendix: caveat checklist when mining `openbench_summary.txt`

- Empty stats / 0 games → ignore.
- SPRT STOP with few games → weak evidence.
- Dev Options listing many floats usually means an SPSA candidate build, not a single-line patch.
- `main` vs old hash baselines measure **accumulated** progress, not one feature.
- Missing diffs (“No diff available…”) still have branch names + Elo; use them for theme labeling, not implementation detail.
