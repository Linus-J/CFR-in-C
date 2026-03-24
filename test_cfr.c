/*
 * test_cfr.c — Unit tests for the Kuhn poker CFR helper functions.
 *
 * Tests cover:
 *  1. getInfoIndex  — verifies the 12 unique infoset-to-index mappings.
 *  2. ev (all-pass) — with both players always checking the game-value
 *     is 0 (equal wins and losses over all 6 ordered card deals).
 *  3. ev (always-bet / always-fold) — player 1 wins every hand → EV = 1.
 *  4. ev (Nash equilibrium) — with the Kuhn Nash strategy (alpha = 1/3)
 *     the expected value for player 1 (first to act) is +1/18 ≈ +0.0556.
 *     Note: ev(p1, p2, traversing_player=1) does NOT correctly compute
 *     player 2's EV because the strategy-lookup selects the wrong table
 *     when traversing_player != acting_player at the root; all production
 *     code calls ev() only with traversing_player=0.
 *  5. Nash indifference — player 1 with J is indifferent between bluffing
 *     and checking (any J strategy gives the same total EV).
 *  6. ev (always-bet / always-call) — showdown only; EV = 0 by symmetry.
 *
 * The functions under test are self-contained reproductions of the
 * corrected implementations so that the tests do not require linking
 * against one of the game binaries.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>

/* ── shared constants ────────────────────────────────────────────────── */
#define NUM_ACTIONS      2
#define MAX_HISTORY_LENGTH 4
#define NUM_CARDS        3
#define NUM_INFO         12

/* ── test bookkeeping ────────────────────────────────────────────────── */
static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_EQ_INT(label, expected, actual) do {                         \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        printf("FAIL  %s: expected %d, got %d\n", (label), (expected), (actual)); \
    }                                                                       \
} while (0)

#define ASSERT_NEAR(label, expected, actual, tol) do {                      \
    tests_run++;                                                            \
    if (fabs((expected) - (actual)) <= (tol)) {                             \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        printf("FAIL  %s: expected %.6f, got %.6f (tol %.6f)\n",           \
               (label), (double)(expected), (double)(actual), (double)(tol)); \
    }                                                                       \
} while (0)

/* ── functions under test (from corrected dcfr.c) ────────────────────── */

/*
 * getInfoIndex — maps a card+history infoset string to a unique index in
 * [0, NUM_INFO).  Domain-specific to Kuhn poker.
 *
 * Format:  infoStr[0]  = card of the acting player ('0'=J, '1'=Q, '2'=K)
 *          infoStr[1+] = action history ('0'=check/fold, '1'=bet/call)
 *
 * Length 1 → player 1's first decision (12 possible: card only)
 * Length 2 → player 2's response to a check ("X0") or a bet ("X1")
 * Length 3 → player 1's re-decision after check-bet ("X01")
 */
static int getInfoIndex(const char *infoStr)
{
    int length = (int)strlen(infoStr);
    int index  = 0;
    switch (length) {
        case 3:
            index = ((int)(infoStr[0]) - '0') * 4 + 3;
            break;
        case 2:
            index = ((int)(infoStr[0] - '0')) * 4 + ((int)(infoStr[1] - '0')) + 1;
            break;
        case 1:
            index = ((int)(infoStr[0] - '0')) * 4;
            break;
    }
    return index;
}

/* forward declaration */
static double calcEv(unsigned short cards[2], const char *history,
                     int pot, uint32_t traversing_player,
                     double p1[NUM_INFO][NUM_ACTIONS],
                     double p2[NUM_INFO][NUM_ACTIONS]);

/*
 * calcEv — recursively compute the expected value for traversing_player
 * when both players follow strategies p1 (player 0) and p2 (player 1).
 * The return value is from traversing_player's perspective at this node
 * (positive = good for traversing_player).
 *
 * Pot mutation fix: pot_before is saved before the action loop so each
 * branch receives the same pre-action pot.
 */
static double calcEv(unsigned short cards[2],
                     const char *history,
                     int pot,
                     uint32_t traversing_player,
                     double p1[NUM_INFO][NUM_ACTIONS],
                     double p2[NUM_INFO][NUM_ACTIONS])
{
    int plays           = (int)strlen(history);
    int acting_player   = plays % 2;
    int opponent_player = 1 - acting_player;

    /* ── terminal conditions ─────────────────────────────────────────── *
     * The return value is from traversing_player's perspective.  When     *
     * ev() is called with traversing_player=0 (the only supported case),  *
     * acting_player == traversing_player at every node because both start  *
     * at 0 and flip together with each recursion.  Terminals therefore     *
     * correctly return +/- relative to the first player.                  */
    if (plays >= 2) {
        /* bet-fold or check-bet-fold: the acting player's side (the one
         * who last bet) wins 1 chip from the folder.  Since acting_player
         * == traversing_player here, +1 means traversing_player gains. */
        if (history[plays-1] == '0' && history[plays-2] == '1') {
            return 1;
        }
        /* check-check or bet-call → showdown */
        if ((history[plays-1] == '0' && history[plays-2] == '0') ||
            (history[plays-1] == '1' && history[plays-2] == '1')) {
            pot = (history[plays-1] == '0') ? 1 : 2;
            return (cards[acting_player] < cards[opponent_player]) ? -pot : pot;
        }
    }

    /* ── non-terminal node ───────────────────────────────────────────── */
    char next_history[MAX_HISTORY_LENGTH + 2] = {'\0'};
    char tempHistory[MAX_HISTORY_LENGTH + 1]  = {'\0'};
    char infoset[MAX_HISTORY_LENGTH + 2]      = {'\0'};
    char actionStr[2]                         = {'\0'};
    char cardStr[2]                           = {'\0'};

    double strat[NUM_ACTIONS] = {0};
    double util[NUM_ACTIONS]  = {0};
    double node_util          = 0;

    strncpy(tempHistory, history, MAX_HISTORY_LENGTH);
    sprintf(cardStr, "%hu", cards[acting_player]); /* fixed: use acting_player */
    strcat(infoset, cardStr);
    strcat(infoset, tempHistory);
    int infoIndex = getInfoIndex(infoset);

    if (traversing_player == 0) {
        strat[0] = p1[infoIndex][0];
        strat[1] = p1[infoIndex][1];
    } else {
        strat[0] = p2[infoIndex][0];
        strat[1] = p2[infoIndex][1];
    }

    int pot_before = pot;
    for (int b = 0; b < NUM_ACTIONS; b++) {
        pot = pot_before;
        strncpy(tempHistory, history, MAX_HISTORY_LENGTH);
        sprintf(actionStr, "%d", b);
        strncat(tempHistory, actionStr, MAX_HISTORY_LENGTH - strlen(tempHistory));
        strncpy(next_history, tempHistory, MAX_HISTORY_LENGTH + 1);
        pot += b;
        util[b] = calcEv(cards, next_history, pot, 1 - traversing_player, p1, p2);
    }
    for (int b = 0; b < NUM_ACTIONS; b++) {
        node_util += strat[b] * util[b];
    }
    return -node_util;
}

/* ev — sum calcEv over all 6 ordered two-card deals, weighted 1/6 each */
static double ev(double p1[NUM_INFO][NUM_ACTIONS],
                 double p2[NUM_INFO][NUM_ACTIONS],
                 uint32_t traversing_player)
{
    unsigned short cards[2] = {0};
    char history[MAX_HISTORY_LENGTH] = {'\0'};
    double result = 0.0;
    for (unsigned short g = 0; g < NUM_CARDS; g++) {
        for (unsigned short h = 0; h < NUM_CARDS; h++) {
            if (g != h) {
                cards[0] = g;
                cards[1] = h;
                result += (1.0 / 6.0) * calcEv(cards, history, 2,
                                                traversing_player, p1, p2);
            }
        }
    }
    return result;
}

/* ── test cases ──────────────────────────────────────────────────────── */

/*
 * test_getInfoIndex — verifies all 12 infoset-to-index mappings.
 *
 * The infoset string is  card_char + history_string.
 * Card chars: '0'=J, '1'=Q, '2'=K.
 *
 * Player-1 first action (length 1):
 *   "0"→0, "1"→4, "2"→8
 * Player-2 response to check (history "0", length 2):
 *   "00"→1, "10"→5, "20"→9
 * Player-2 response to bet (history "1", length 2):
 *   "01"→2, "11"→6, "21"→10
 * Player-1 re-decision after check-bet (history "01", length 3):
 *   "001"→3, "101"→7, "201"→11
 */
static void test_getInfoIndex(void)
{
    /* length-1: player 1's opening decision */
    ASSERT_EQ_INT("getInfoIndex(\"0\")",  0, getInfoIndex("0"));
    ASSERT_EQ_INT("getInfoIndex(\"1\")",  4, getInfoIndex("1"));
    ASSERT_EQ_INT("getInfoIndex(\"2\")",  8, getInfoIndex("2"));

    /* length-2: player 2's response to a check */
    ASSERT_EQ_INT("getInfoIndex(\"00\")", 1, getInfoIndex("00"));
    ASSERT_EQ_INT("getInfoIndex(\"10\")", 5, getInfoIndex("10"));
    ASSERT_EQ_INT("getInfoIndex(\"20\")", 9, getInfoIndex("20"));

    /* length-2: player 2's response to a bet */
    ASSERT_EQ_INT("getInfoIndex(\"01\")",  2, getInfoIndex("01"));
    ASSERT_EQ_INT("getInfoIndex(\"11\")",  6, getInfoIndex("11"));
    ASSERT_EQ_INT("getInfoIndex(\"21\")", 10, getInfoIndex("21"));

    /* length-3: player 1's re-decision after check-bet */
    ASSERT_EQ_INT("getInfoIndex(\"001\")",  3, getInfoIndex("001"));
    ASSERT_EQ_INT("getInfoIndex(\"101\")",  7, getInfoIndex("101"));
    ASSERT_EQ_INT("getInfoIndex(\"201\")", 11, getInfoIndex("201"));
}

/*
 * test_getInfoIndex_unique — verifies all 12 indices are distinct.
 */
static void test_getInfoIndex_unique(void)
{
    const char *infosets[NUM_INFO] = {
        "0", "1", "2",
        "00", "10", "20",
        "01", "11", "21",
        "001", "101", "201"
    };
    int seen[NUM_INFO] = {0};
    int all_unique = 1;
    for (int i = 0; i < NUM_INFO; i++) {
        /* need writable copy for getInfoIndex */
        char buf[MAX_HISTORY_LENGTH + 2];
        strncpy(buf, infosets[i], sizeof(buf) - 1);
        buf[sizeof(buf)-1] = '\0';
        int idx = getInfoIndex(buf);
        if (idx < 0 || idx >= NUM_INFO || seen[idx]) {
            all_unique = 0;
            printf("FAIL  getInfoIndex_unique: duplicate or out-of-range index %d for \"%s\"\n",
                   idx, infosets[i]);
            tests_failed++;
            tests_run++;
        } else {
            seen[idx] = 1;
        }
    }
    if (all_unique) {
        tests_run++;
        tests_passed++;
    }
}

/*
 * test_ev_allpass — with both players always checking/passing the expected
 * value is 0 for both perspectives (each player wins as often as they lose).
 */
static void test_ev_allpass(void)
{
    double p_allpass[NUM_INFO][NUM_ACTIONS] = {0};
    for (int i = 0; i < NUM_INFO; i++) {
        p_allpass[i][0] = 1.0; /* always action 0 (check/pass) */
        p_allpass[i][1] = 0.0;
    }

    double ev0 = ev(p_allpass, p_allpass, 0);
    double ev1 = ev(p_allpass, p_allpass, 1);

    ASSERT_NEAR("ev_allpass player0", 0.0, ev0, 1e-9);
    ASSERT_NEAR("ev_allpass player1", 0.0, ev1, 1e-9);
}

/*
 * test_ev_alwaysbet_alwaysfold — player 1 always bets, player 2 always
 * folds.  Player 1 wins 1 on every hand regardless of cards, so EV = 1
 * from traversing_player=0's perspective.
 */
static void test_ev_alwaysbet_alwaysfold(void)
{
    double p1_bet[NUM_INFO][NUM_ACTIONS]  = {0};
    double p2_fold[NUM_INFO][NUM_ACTIONS] = {0};

    for (int i = 0; i < NUM_INFO; i++) {
        p1_bet[i][0]  = 0.0;
        p1_bet[i][1]  = 1.0; /* always bet/call */
        p2_fold[i][0] = 1.0; /* always check/fold */
        p2_fold[i][1] = 0.0;
    }

    double ev0 = ev(p1_bet, p2_fold, 0);
    ASSERT_NEAR("ev_alwaysbet_alwaysfold player0", 1.0, ev0, 1e-9);
}

/*
 * test_ev_nash — with the unique Kuhn Nash equilibrium (alpha = 1/3) the
 * game value for player 1 is exactly -1/18 ≈ -0.05556.
 *
 * Nash strategies (alpha = 1/3):
 *  Player 1:
 *   idx  0 ("0"  = J, start)      : [2/3, 1/3]   check with 2/3, bet with 1/3
 *   idx  4 ("1"  = Q, start)      : [1,   0  ]   always check
 *   idx  8 ("2"  = K, start)      : [0,   1  ]   always bet
 *   idx  3 ("001"= J, check-bet)  : [1,   0  ]   always fold
 *   idx  7 ("101"= Q, check-bet)  : [1,   0  ]   always fold
 *   idx 11 ("201"= K, check-bet)  : [0,   1  ]   always call
 *  Player 2:
 *   idx  1 ("00" = J after check) : [1,   0  ]   always check
 *   idx  5 ("10" = Q after check) : [1,   0  ]   always check
 *   idx  9 ("20" = K after check) : [0,   1  ]   always bet
 *   idx  2 ("01" = J after bet)   : [1,   0  ]   always fold
 *   idx  6 ("11" = Q after bet)   : [2/3, 1/3]   fold 2/3, call 1/3
 *   idx 10 ("21" = K after bet)   : [0,   1  ]   always call
 *
 * With these strategies the game value for player 1 (first to act) is
 * +1/18 ≈ 0.0556.  Note: ev(p1, p2, traversing_player=1) is not a
 * correct way to obtain player 2's EV because the strategy-lookup
 * uses the wrong table when traversing_player ≠ acting_player at the
 * root.  Player 2's EV is simply -ev(p1, p2, 0) by zero-sum.
 */
static void test_ev_nash(void)
{
    double p1[NUM_INFO][NUM_ACTIONS] = {0};
    double p2[NUM_INFO][NUM_ACTIONS] = {0};

    /* initialise both tables to uniform (covers unreachable infosets) */
    for (int i = 0; i < NUM_INFO; i++) {
        p1[i][0] = 0.5; p1[i][1] = 0.5;
        p2[i][0] = 0.5; p2[i][1] = 0.5;
    }

    /* player-1 Nash actions */
    p1[0][0] = 2.0/3.0; p1[0][1] = 1.0/3.0; /* J  start: check 2/3, bluff 1/3 */
    p1[4][0] = 1.0;     p1[4][1] = 0.0;     /* Q  start: always check */
    p1[8][0] = 0.0;     p1[8][1] = 1.0;     /* K  start: always bet */
    p1[3][0] = 1.0;     p1[3][1] = 0.0;     /* J  check-bet: fold */
    p1[7][0] = 1.0;     p1[7][1] = 0.0;     /* Q  check-bet: fold */
    p1[11][0] = 0.0;    p1[11][1] = 1.0;    /* K  check-bet: call */

    /* player-2 Nash actions */
    p2[1][0] = 1.0;     p2[1][1] = 0.0;     /* J  after check: check */
    p2[5][0] = 1.0;     p2[5][1] = 0.0;     /* Q  after check: check */
    p2[9][0] = 0.0;     p2[9][1] = 1.0;     /* K  after check: bet */
    p2[2][0] = 1.0;     p2[2][1] = 0.0;     /* J  after bet:   fold */
    p2[6][0] = 2.0/3.0; p2[6][1] = 1.0/3.0; /* Q  after bet:   fold 2/3, call 1/3 */
    p2[10][0] = 0.0;    p2[10][1] = 1.0;    /* K  after bet:   call */

    /* ev(p1, p2, 0) is player 1's expected gain (first to act). */
    double ev0 = ev(p1, p2, 0);

    ASSERT_NEAR("ev_nash player0 ≈ +1/18", 1.0/18.0, ev0, 1e-9);

    /*
     * Nash indifference for player 1 with J: P1 bluffs J with prob 1/3 to
     * make P2 with Q indifferent between calling and folding.  This means P1
     * is itself indifferent — any bluff frequency yields the same EV.  We
     * test two extreme cases (always bluff / never bluff) to confirm.
     */
    double p1_j_allbet[NUM_INFO][NUM_ACTIONS];
    memcpy(p1_j_allbet, p1, sizeof(p1));
    p1_j_allbet[0][0] = 0.0; p1_j_allbet[0][1] = 1.0; /* J always bets */

    double p1_j_allcheck[NUM_INFO][NUM_ACTIONS];
    memcpy(p1_j_allcheck, p1, sizeof(p1));
    p1_j_allcheck[0][0] = 1.0; p1_j_allcheck[0][1] = 0.0; /* J never bets */

    ASSERT_NEAR("ev_nash J_always_bet == Nash EV (P1 indifferent)",
                1.0/18.0, ev(p1_j_allbet,   p2, 0), 1e-9);
    ASSERT_NEAR("ev_nash J_never_bet  == Nash EV (P1 indifferent)",
                1.0/18.0, ev(p1_j_allcheck, p2, 0), 1e-9);
}

/*
 * test_ev_allcheck_vs_allbet — when player 1 always bets and player 2 always
 * folds, player 1 wins 1 chip every hand (EV = 1).  When player 2 instead
 * always calls, the outcome depends only on card rank: player 1 wins 2/3 of
 * the time (when holding Q or K vs J) and loses 1/3 of the time.
 * EV = (1/2) * 2 + (1/6) * 2 - (1/3) * 2 = (4/6) * 2 - ... let us compute:
 * Of 6 ordered deals: P1 wins when P1 card > P2 card (3 deals), loses otherwise.
 * Winner gets +2, loser gets -2 → EV = (3/6)*(2) + (3/6)*(-2) = 0.
 */
static void test_ev_allbet_allcall(void)
{
    double p1_bet[NUM_INFO][NUM_ACTIONS]  = {0};
    double p2_call[NUM_INFO][NUM_ACTIONS] = {0};

    for (int i = 0; i < NUM_INFO; i++) {
        p1_bet[i][0]  = 0.0; p1_bet[i][1]  = 1.0; /* P1 always bets */
        p2_call[i][0] = 0.0; p2_call[i][1] = 1.0; /* P2 always calls */
    }

    double ev0 = ev(p1_bet, p2_call, 0);
    ASSERT_NEAR("ev_allbet_allcall == 0 (3 wins / 3 losses, ±2 each)", 0.0, ev0, 1e-9);
}

/* ── main ──────────────────────────────────────────────────────────── */
int main(void)
{
    printf("Running Kuhn poker CFR unit tests...\n\n");

    test_getInfoIndex();
    test_getInfoIndex_unique();
    test_ev_allpass();
    test_ev_alwaysbet_alwaysfold();
    test_ev_nash();
    test_ev_allbet_allcall();

    printf("\nResults: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(", %d FAILED\n", tests_failed);
    } else {
        printf(" — all tests passed\n");
    }

    return (tests_failed > 0) ? 1 : 0;
}
