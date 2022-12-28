#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "pcg/pcg_basic.h"

// Number of actions for each player at each stage of the game
#define NUM_ACTIONS 2

#define MAX_HISTORY_LENGTH 4

// Number of cards in a deck
#define NUM_CARDS 3

// Number of infosets in the game
#define NUM_INFO 12

// Number of players in the game
#define NUM_PLAYERS 2

// Utility values for each player in each possible game outcome
const double UTILITIES[3][3] = {
    { 1, -1 },
    { -1, 1 },
    { 0, 0 }
};

// Initialise the strategy and regret arrays to 0
double strategy[NUM_INFO][NUM_ACTIONS] = {0};
double strategy_sum[NUM_INFO][NUM_ACTIONS] = {0};
double regrets[NUM_INFO][NUM_ACTIONS] = {0};
pcg32_random_t rng;

static inline uint32_t random_bounded_divisionless(uint32_t range) {
    uint64_t random32bit, multiresult;
    random32bit =  pcg32_random_r(&rng);
    multiresult = random32bit * range;
    return multiresult >> 32;
}

int getInfoIndex(char infoStr[MAX_HISTORY_LENGTH]){
    //Perfect mapping of infoset to index, DOMAIN SPECIFIC TO VANILLA KUHN POKER
    int length = strlen(infoStr);
    int index = 0;
    switch(length){
        case 3:
            index = ((int)(infoStr[0])-'0')*4+3;
            break;
        case 2:
            index = ((int)(infoStr[0]-'0'))*4+((int)(infoStr[1]-'0'))+1;
            break;
        case 1:
            index = ((int)(infoStr[0]-'0'))*4;
            break;
    }
    return index;
}

// Helper function to compute the current strategy for a given player and card
void *compute_strategy(int infoIndex, double **tempStrategy)
{
    free(*tempStrategy);
    *tempStrategy = malloc(NUM_ACTIONS * sizeof(double));
    // Compute the normalizing factor
    
    double normalizing_factor = 0;
    for (int i = 0; i < NUM_ACTIONS; i++)
    {
        if (regrets[infoIndex][i]>0){
            strategy[infoIndex][i] = regrets[infoIndex][i];
        }
        else{
            strategy[infoIndex][i] = 0;
        }
        normalizing_factor += strategy[infoIndex][i];
    }

    for (int i = 0; i < NUM_ACTIONS; i++)
    {
        if (normalizing_factor > 0){ // If the normalizing factor is > 0, compute the strategy as the regret-matched strategy
            strategy[infoIndex][i] /= normalizing_factor;
        }
        else{ // Otherwise, the strategy is uniformly random
            strategy[infoIndex][i] = 1.0 / NUM_ACTIONS;
        }
    }
    for (int i = 0; i < NUM_ACTIONS; i++)
    {
        (*tempStrategy)[i] = strategy[infoIndex][i];
    }
}

void compute_avg_strategy(int infoIndex)
{
    double avg_strategy[NUM_ACTIONS] = {0};
    double normalizing_sum = 0;
    for (int i = 0; i < NUM_ACTIONS; i++){
        normalizing_sum += strategy_sum[infoIndex][i];
    }

    for (int i = 0; i < NUM_ACTIONS; i++){
        if (normalizing_sum > 0){
            avg_strategy[i] = strategy_sum[infoIndex][i] / normalizing_sum;
        }   
        else{
            avg_strategy[i] = 1.0 / NUM_ACTIONS;
        }
    }
    printf("[%f, %f]\n", avg_strategy[0], avg_strategy[1]);
}

double external_cfr(unsigned short cards[2], char history[MAX_HISTORY_LENGTH], int pot, int traversing_player){
    int plays = strlen(history);
    int acting_player = plays % 2;
    int opponent_player = 1 - acting_player;
    /*
        To extend: Check here if terminal history, then change the below to getPayoff function / lookup table.
    */
    if (plays >= 2){ //Check payoff if history not 0, 1
        if (history[plays-1] == '0' && history[plays-2] == '1'){ //bet fold or pass bet fold
            if (acting_player == traversing_player){
                return 1;
            }
            else{
                return -1;
            }
        }
        if ((history[plays-1] == '0' && history[plays-2] == '0') || (history[plays-1] == '1' && history[plays-2] == '1')){ //check check, bet call or check bet call, go to showdown
            if (acting_player == traversing_player){
                if (cards[acting_player] > cards[opponent_player]){
                    return pot/2;
                }
                else{
                    return -pot/2;
                }
            }
            else{
                if (cards[acting_player] > cards[opponent_player]){
                    return -pot/2;
                }
                else{
                    return pot/2;
                }
            }
        }
    }
    char next_history[MAX_HISTORY_LENGTH+2]={'\0'};
    char temp[MAX_HISTORY_LENGTH+2]={'\0'};
    char tempHistory[MAX_HISTORY_LENGTH]={'\0'};
    char infoset[MAX_HISTORY_LENGTH+2]={'\0'};
    char actionStr[2]={'\0'};
    char cardStr[2]={'\0'};
    if (acting_player == traversing_player){
        double *tempStrategy;
        tempStrategy = NULL;
        double util[NUM_ACTIONS] = {0};
        double node_util = 0;
        strcpy(tempHistory, history);
        sprintf(cardStr, "%hu", cards[acting_player]);
        strcat(infoset,cardStr);
        strcat(infoset, tempHistory);
        int infoIndex = getInfoIndex(infoset);
        compute_strategy(infoIndex,&tempStrategy); //want to put infoset instead of cards, need a map from infoset to int
        for (int b=0; b<NUM_ACTIONS; b++){
            strcpy(tempHistory, history);
            sprintf(actionStr, "%d", b);
            strcpy(next_history, strcat(tempHistory, actionStr));
            strcpy(temp,next_history);
            pot += b;
            util[b] = external_cfr(cards, next_history, pot, traversing_player);
            node_util += tempStrategy[b] * util[b];
        }
        free(tempStrategy);
        for (int c=0; c<NUM_ACTIONS; c++){
            regrets[infoIndex][c] += util[c] - node_util;
        }
        return node_util;
    }
    else{ //acting_player != traversing_player
        double *tempStrategy;
        tempStrategy = NULL;
        strcpy(tempHistory, history);
        sprintf(cardStr, "%hu", cards[acting_player]);
        strcat(infoset,cardStr);
        strcat(infoset, tempHistory);
        int infoIndex = getInfoIndex(infoset);
        compute_strategy(infoIndex,&tempStrategy);
        double tempUtil = 0;
        strcpy(tempHistory, history);
        if (((double)rand())/RAND_MAX < tempStrategy[0]){
            sprintf(actionStr, "%d", 0);
            strcpy(next_history, strcat(tempHistory, actionStr));
        }
        else{
            sprintf(actionStr, "%d", 1);
            strcpy(next_history, strcat(tempHistory, actionStr));
            pot += 1;
        }
        strcpy(temp, next_history);
        tempUtil = external_cfr(cards, next_history, pot, traversing_player);
        for (int a=0; a<NUM_ACTIONS; a++){
            strategy_sum[infoIndex][a] += tempStrategy[a];
        }
        free(tempStrategy);
        return tempUtil;
    }
}

void cfr(int iterations)
{
    // Initialise cards
    unsigned short deck[NUM_CARDS] = {0};
    for (unsigned short i = 0; i < NUM_CARDS; i++)
    {
        deck[i] = i;
    }
    double util[NUM_ACTIONS] = {0};
    unsigned short cards[2] = {0};
    int temp = 0;
    char history[MAX_HISTORY_LENGTH]={'\0'};
    for (int t=1; t<iterations + 1; t++){
        if (t%100000==0){
            printf("Iteration: %d\n", t);
        }
        for (int i=0; i<NUM_PLAYERS; i++){ //Number of players is two
            //Shuffle cards
            for (int j=NUM_CARDS; j>1; j--) {
                int p = random_bounded_divisionless(j); // number in [0,i)
                temp = deck[j-1];
                deck[j-1] = deck[p]; // swap the values at j-1 and p
                deck[p] = temp;
            }
            cards[0] = deck[0];
            cards[1] = deck[1];
            util[i] += external_cfr(cards, history, 2, i);
        }
    }
    printf("Average game value: %f\n", (util[0]/iterations));
    for (int i=0; i<NUM_INFO; i++){
        printf("%d: ", i);
        compute_avg_strategy(i);
    }
}

int main() {
    // Initialise the random number generator
    srand(time(NULL));
    pcg32_srandom_r(&rng, time(NULL) ^ (intptr_t)&printf, (intptr_t)&rng);
    cfr(10000000); //iterations
    return 0;
}