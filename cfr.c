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

// Initialise the strategy and regret arrays to 0
double strategy[NUM_INFO][NUM_ACTIONS] = {0};
double strategy_sum[NUM_INFO][NUM_ACTIONS] = {0};
double regrets[NUM_INFO][NUM_ACTIONS] = {0};
double tempRegrets[NUM_INFO][NUM_ACTIONS] = {0};

double brStrategy[NUM_INFO][NUM_ACTIONS] = {0};

int getInfoIndex(char infoStr[MAX_HISTORY_LENGTH]){
    //Perfect mapping of infoset to index, DOMAIN SPECIFIC TO VANILLA KUHN POKER
    int length = strlen(infoStr);
    int index = 0;
    switch(length){
        case 3:
            index = ((int)(infoStr[0])-'0')*4+3;
            break;
        case 2: // '0' '1'
            index = ((int)(infoStr[0]-'0'))*4+((int)(infoStr[1]-'0'))+1;
            break;
        case 1: //' '
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

void compute_avg_strategy(int infoIndex, double **tempStrategy)
{
    free(*tempStrategy);
    *tempStrategy = malloc(NUM_ACTIONS * sizeof(double));
    double normalizing_sum = 0;
    for (int i = 0; i < NUM_ACTIONS; i++){
        normalizing_sum += strategy_sum[infoIndex][i];
    }

    for (int i = 0; i < NUM_ACTIONS; i++){
        if (normalizing_sum > 0){
            (*tempStrategy)[i] = strategy_sum[infoIndex][i] / normalizing_sum;
        }   
        else{
            (*tempStrategy)[i] = 1.0 / NUM_ACTIONS;
        }
    }
}

void print_avg_strategy(int infoIndex)
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

double calcEv(unsigned short cards[2], char history[MAX_HISTORY_LENGTH], int pot, uint32_t traversing_player, double p1[NUM_INFO][NUM_ACTIONS], double p2[NUM_INFO][NUM_ACTIONS]){
    int plays = strlen(history);
    int acting_player = plays % 2;
    int opponent_player = 1 - acting_player;
    if (plays >= 2){ //Check payoff if history not 0, 1
        if (history[plays-1] == '0' && history[plays-2] == '1'){ //bet fold or pass bet fold
            return 1;
        }
        if ((history[plays-1] == '0' && history[plays-2] == '0') || (history[plays-1] == '1' && history[plays-2] == '1')){ //check check, bet call or check bet call, go to showdown
            if ((history[plays-1] == '0' && history[plays-2] == '0')){
                pot = 1;
            }else{
                pot = 2;
            }
            if (cards[acting_player] < cards[opponent_player]){
                return -pot;
            }
            else{
                return pot;
            }
        }
    }

    char next_history[MAX_HISTORY_LENGTH+2]={'\0'};
    char tempHistory[MAX_HISTORY_LENGTH]={'\0'};
    char infoset[MAX_HISTORY_LENGTH+2]={'\0'};
    char actionStr[2]={'\0'};
    char cardStr[2]={'\0'};

    double strat[NUM_ACTIONS] = {0};
    double util[NUM_ACTIONS] = {0};
    double node_util = 0;
    strcpy(tempHistory, history);
    sprintf(cardStr, "%hu", cards[traversing_player]);
    strcat(infoset,cardStr);
    strcat(infoset, tempHistory);
    int infoIndex = getInfoIndex(infoset);

    if (traversing_player == 0){
        strat[0] = p1[infoIndex][0];
        strat[1] = p1[infoIndex][1];
    }
    else{
        strat[0] = p2[infoIndex][0];
        strat[1] = p2[infoIndex][1];
    }

    for (int b=0; b<NUM_ACTIONS; b++){
        strcpy(tempHistory, history);
        sprintf(actionStr, "%d", b);
        strcat(tempHistory, actionStr);
        strcpy(next_history, tempHistory);
        pot += b;
        util[b] = calcEv(cards, next_history, pot, 1-traversing_player, p1, p2);
    }
    for (int b=0; b<NUM_ACTIONS; b++){
        node_util += strat[b] * util[b];
    }
    return -node_util;
}

double ev(double p1[NUM_INFO][NUM_ACTIONS], double p2[NUM_INFO][NUM_ACTIONS], uint32_t traversing_player){
    double exp = 0;
    unsigned short deck[NUM_CARDS] = {0};
    for (unsigned short i = 0; i < NUM_CARDS; i++)
    {
        deck[i] = i;
    }
    unsigned short cards[2] = {0};
    char history[MAX_HISTORY_LENGTH]={'\0'};
    for (unsigned short g = 0; g < NUM_CARDS; g++)
    {
        for (unsigned short h = 0; h < NUM_CARDS; h++)
        {
            if (g!=h){
                cards[0] = g;
                cards[1] = h;
                exp += ((double)1/6)*calcEv(cards, history, 2, traversing_player, p1, p2);                
            }
        }
    }
    return exp;
}

double calc_best_response(unsigned short cards[2], char history[MAX_HISTORY_LENGTH], int pot, unsigned short traversing_player, double prob, double player[NUM_INFO][NUM_ACTIONS]){
    int plays = strlen(history);
    int acting_player = plays % 2;
    int opponent_player = 1 - acting_player;
    if (plays >= 2){ //Check payoff if history not 0, 1
        if (history[plays-1] == '0' && history[plays-2] == '1'){ //bet fold or pass bet fold
            return -1;
        }
        if ((history[plays-1] == '0' && history[plays-2] == '0') || (history[plays-1] == '1' && history[plays-2] == '1')){ //check check, bet call or check bet call, go to showdown
            if ((history[plays-1] == '0' && history[plays-2] == '0')){
                pot = 1;
            }else{
                pot = 2;
            }
            if (cards[acting_player] < cards[opponent_player]){
                return pot;
            }
            else{
                return -pot;
            }
        }
    }
    char next_history[MAX_HISTORY_LENGTH+2]={'\0'};
    char temp[MAX_HISTORY_LENGTH+2]={'\0'};
    char tempHistory[MAX_HISTORY_LENGTH]={'\0'};
    char infoset[MAX_HISTORY_LENGTH+2]={'\0'};
    char actionStr[2]={'\0'};
    char cardStr[2]={'\0'};

    double tempStrategy[NUM_ACTIONS] = {0};
    double util[NUM_ACTIONS] = {0};
    double node_util = 0;
    strcpy(tempHistory, history);
    sprintf(cardStr, "%hu", cards[acting_player]);
    strcat(infoset,cardStr);
    strcat(infoset, tempHistory);
    int infoIndex = getInfoIndex(infoset);
    double brVal = 0;

    if (acting_player == traversing_player){
        for (int b=0; b<NUM_ACTIONS; b++){
            strcpy(tempHistory, history);
            sprintf(actionStr, "%d", b);
            strcpy(next_history, strcat(tempHistory, actionStr));
            strcpy(temp,next_history);
            pot += b;
            util[b] = calc_best_response(cards, next_history, pot, traversing_player,prob,player);
        }
        brVal = fmax(util[0],util[1]);
        brStrategy[infoIndex][0] += prob * util[0];
        brStrategy[infoIndex][1] += prob * util[1];
        return -brVal;
    }
    else{
        double norm = 0;
        for (int b=0; b<NUM_ACTIONS; b++){
            norm += player[infoIndex][b];
        }
        for (int b=0; b<NUM_ACTIONS; b++){
            tempStrategy[b] = player[infoIndex][b]/norm;
        }
        
        for (int b=0; b<NUM_ACTIONS; b++){
            strcpy(tempHistory, history);
            sprintf(actionStr, "%d", b);
            strcpy(next_history, strcat(tempHistory, actionStr));
            strcpy(temp,next_history);
            pot += b;
            util[b] = calc_best_response(cards, next_history, pot, traversing_player,tempStrategy[b]*prob,player);
            node_util += tempStrategy[b] * util[b];
        }
        return -node_util;
    }
}

void best_response(double player[NUM_INFO][NUM_ACTIONS]){
    uint32_t total_cards = 0;
    total_cards = NUM_CARDS;
    // Initialise cards
    unsigned short cards[2] = {0};
    uint32_t deck[NUM_CARDS] = {0};
    char history[MAX_HISTORY_LENGTH]={'\0'};
    char flopHistory[MAX_HISTORY_LENGTH]={'\0'};
    for (uint32_t i = 0; i < total_cards; i++)
    {
        deck[i] = i;
    }
    for (uint32_t g = 0; g < total_cards; g++)
    {
        for (uint32_t h = 0; h < total_cards; h++)
        {
            if (g!=h ){
                for (uint32_t i=0; i<2; i++){ //Number of players is two
                    cards[0] = g;
                    cards[1] = h;
                    calc_best_response(cards, history, 2, i, 1, player);
                }
            }
            
        }
    }
    uint32_t ind = 0;
    double temp = 0;
    for (uint32_t i=0; i<NUM_INFO; i++){
        for (uint32_t j=0; j<NUM_ACTIONS; j++){
            if (temp < brStrategy[i][j]){  
                temp = brStrategy[i][j];
                ind = j;
            }
        }
        for (uint32_t j=0; j<NUM_ACTIONS; j++){
            if (j == ind){  
                brStrategy[i][j] = 1;
            }
            else{
                brStrategy[i][j] = 0;
            }
        }
    }
}

double vanilla_cfr(unsigned short cards[2], char history[MAX_HISTORY_LENGTH], int pot, int traversing_player){
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
        //strcpy(infoset,strcat(cardStr,tempHistory));
        int infoIndex = getInfoIndex(infoset);
        compute_strategy(infoIndex,&tempStrategy); //want to put infoset instead of cards, need a map from infoset to int
        for (int b=0; b<NUM_ACTIONS; b++){
            strcpy(tempHistory, history);
            sprintf(actionStr, "%d", b);
            strcpy(next_history, strcat(tempHistory, actionStr));
            strcpy(temp,next_history);
            pot += b;
            util[b] = vanilla_cfr(cards, next_history, pot, traversing_player);
            node_util += tempStrategy[b] * util[b];
        }
        free(tempStrategy);
        for (int c=0; c<NUM_ACTIONS; c++){
            tempRegrets[infoIndex][c] += util[c] - node_util;
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
        tempUtil = vanilla_cfr(cards, next_history, pot, traversing_player);
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
        for (unsigned short g = 0; g < NUM_CARDS; g++)
        {
            for (unsigned short h = 0; h < NUM_CARDS; h++)
            {
                if (g!=h){
                    for (int i=0; i<NUM_PLAYERS; i++){ //Number of players is two
                        cards[0] = g;
                        cards[1] = h;
                        util[i] += ((double)1/6)*vanilla_cfr(cards, history, 2, i);
                    }
                }
            }
        }
        for (int i=0; i<NUM_INFO; i++){
            regrets[i][0] += tempRegrets[i][0];
            regrets[i][1] += tempRegrets[i][1];
            tempRegrets[i][0] = 0;
            tempRegrets[i][1] = 0;
        }
    }
    printf("Average game value: %f\n", (util[0]/iterations));

    double myStrat[NUM_INFO][NUM_ACTIONS] = {0};
    double *tempStrategy;
    tempStrategy = NULL;
    for (int i=0; i<NUM_INFO; i++){
        printf("%d: ", i);
        print_avg_strategy(i);
        compute_avg_strategy(i, &tempStrategy);
        myStrat[i][0] = tempStrategy[0];
        myStrat[i][1] = tempStrategy[1];
        printf("[%f, %f]\n",tempStrategy[0],tempStrategy[1]);
    }
    double nash[NUM_INFO][NUM_ACTIONS] = {{0.9,0.1},{0.666666,0.333334},{1,0},{1,0}, \
                                            {1,0},{1,0},{0.666666,0.333334},{0.5666666,0.4333334},\
                                            {0.7,0.3},{0,1},{0,1},{0,1}};

    printf("EV: %f\n",ev(myStrat, myStrat, 0));
    //printf("EV: %f\n",ev(nash, nash, 0));
    free(tempStrategy);
}

int main() {
    // Initialise the random number generator
    srand(time(NULL));
    cfr(1000000); //iterations
    return 0;
}