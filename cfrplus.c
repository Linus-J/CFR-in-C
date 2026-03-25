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

// Initialise the strategy and regret arrays to 0
double strategy[NUM_INFO][NUM_ACTIONS] = {0};
double strategy_sum[NUM_INFO][NUM_ACTIONS] = {0};
double regrets[NUM_INFO][NUM_ACTIONS] = {0};
double tempRegrets[NUM_INFO][NUM_ACTIONS] = {0};

double brStrategy[NUM_INFO][NUM_ACTIONS] = {0};

int getInfoIndex(char infoStr[MAX_HISTORY_LENGTH]){
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

void *compute_strategy(int infoIndex, double **tempStrategy, double realizationWeight, double t)
{
    free(*tempStrategy);
    *tempStrategy = malloc(NUM_ACTIONS * sizeof(double));
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
        if (normalizing_factor > 0){
            strategy[infoIndex][i] /= normalizing_factor;
        }
        else{
            strategy[infoIndex][i] = 1.0 / NUM_ACTIONS;
        }
        if (realizationWeight != -1){
            strategy_sum[infoIndex][i] += realizationWeight * strategy[infoIndex][i] * pow(((double)t/(t+1)),2);
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

void save_strategy(double player[NUM_INFO][NUM_ACTIONS]){
    FILE *fptr;
    fptr = fopen("cfrplus.txt","w");

    if(fptr == NULL)
    {
        printf("File error!\n");
        exit(1);
    }
    for (uint32_t i=0; i<NUM_INFO; i++){
        fprintf(fptr,"%f,%f\n",player[i][0],player[i][1]);
    }
    fclose(fptr);
}

void load_strategy(double **player){
    FILE *fptr;
    fptr = fopen("cfrplus.txt","r");
    if(fptr == NULL){
        printf("File error!");
        exit(1);
    }
    double *data = (double *) malloc(NUM_INFO * 2 * sizeof(double));
    if (data == NULL) {
        printf("Error allocating memory\n");
        fclose(fptr);
        return;
    }
    for (int i = 0; i < NUM_INFO; i++) {
        player[i] = &data[i * 2];
    }
    int row = 0;
    while (!feof(fptr) && row < NUM_INFO) {
        double value1=0, value2=0;        
        if (fscanf(fptr, "%lf,%lf\n", &value1, &value2) != 2) {
            printf("Error reading strategy from file at row %d\n", row);
            break;
        }
        player[row][0] = value1;
        player[row][1] = value2;
        row++;
    }
    fclose(fptr);
}

double calcEv(unsigned short cards[2], char history[MAX_HISTORY_LENGTH], int pot, uint32_t traversing_player, double p1[NUM_INFO][NUM_ACTIONS], double p2[NUM_INFO][NUM_ACTIONS]){
    int plays = strlen(history);
    int acting_player = plays % 2;
    int opponent_player = 1 - acting_player;
    if (plays >= 2){
        if (history[plays-1] == '0' && history[plays-2] == '1'){
            return 1;
        }
        if ((history[plays-1] == '0' && history[plays-2] == '0') || (history[plays-1] == '1' && history[plays-2] == '1')){
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
    sprintf(cardStr, "%hu", cards[acting_player]);
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

    int pot_before = pot;
    for (int b=0; b<NUM_ACTIONS; b++){
        pot = pot_before;
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

double cbr(unsigned short cards[2], double p0, double p1, char history[MAX_HISTORY_LENGTH], int pot, int traversing_player, double player[NUM_INFO][NUM_ACTIONS]){
    int plays = strlen(history);
    int acting_player = plays % 2;
    int opponent_player = 1 - acting_player;
    if (plays >= 2){
        if (history[plays-1] == '0' && history[plays-2] == '1'){
            return 1;
        }
        if ((history[plays-1] == '0' && history[plays-2] == '0') || (history[plays-1] == '1' && history[plays-2] == '1')){
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
    tempStrategy[0] = player[infoIndex][0];
    tempStrategy[1] = player[infoIndex][1];
    int pot_before = pot;
    for (int b=0; b<NUM_ACTIONS; b++){
        pot = pot_before;
        strcpy(tempHistory, history);
        sprintf(actionStr, "%d", b);
        strcpy(next_history, strcat(tempHistory, actionStr));
        strcpy(temp,next_history);
        pot += b;
        if (acting_player == 0){
            util[b] = -1*cbr(cards, p0*tempStrategy[b], p1, next_history, pot, traversing_player, player);
        }
        else{
            util[b] = -1*cbr(cards, p0, p1*tempStrategy[b], next_history, pot, traversing_player, player);
        }
        
        node_util += tempStrategy[b] * util[b];
    }
    if (acting_player == 0){
        brStrategy[infoIndex][0] += p0 * util[0];
        brStrategy[infoIndex][1] += p0 * util[1];
    }
    else{
        brStrategy[infoIndex][0] += p1 * util[0];
        brStrategy[infoIndex][1] += p1 * util[1];
    }
    return node_util;
}

void best_response(double player[NUM_INFO][NUM_ACTIONS]){
    uint32_t total_cards = 0;
    total_cards = NUM_CARDS;
    unsigned short cards[2] = {0};
    char history[MAX_HISTORY_LENGTH]={'\0'};
    char flopHistory[MAX_HISTORY_LENGTH]={'\0'};
    for (uint32_t i = 0; i < NUM_INFO; i++){
        for (uint32_t j = 0; j < NUM_ACTIONS; j++){
            brStrategy[i][j] = 0;
        }
    }
    for (uint32_t g = 0; g < total_cards; g++){
        for (uint32_t h = 0; h < total_cards; h++){
            if (g!=h){
                for (uint32_t i=0; i<2; i++){
                    cards[0] = g;
                    cards[1] = h;
                    cbr(cards, 1, 1, history, 2, i, player);
                }
            }
        }
    }
    uint32_t ind = 0;
    double temp = 0;
    for (uint32_t i=0; i<NUM_INFO; i++){
        temp = -100000;
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

double vanilla_cfr(unsigned short cards[2], double p0, double p1, char history[MAX_HISTORY_LENGTH], int pot, int traversing_player, int t){
    int plays = strlen(history);
    int acting_player = plays % 2;
    int opponent_player = 1 - acting_player;
    if (plays >= 2){
        if (history[plays-1] == '0' && history[plays-2] == '1'){
            return 1;
        }
        if ((history[plays-1] == '0' && history[plays-2] == '0') || (history[plays-1] == '1' && history[plays-2] == '1')){
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
    char temp[MAX_HISTORY_LENGTH+2]={'\0'};
    char tempHistory[MAX_HISTORY_LENGTH]={'\0'};
    char infoset[MAX_HISTORY_LENGTH+2]={'\0'};
    char actionStr[2]={'\0'};
    char cardStr[2]={'\0'};

    double *tempStrategy;
    tempStrategy = NULL;
    double util[NUM_ACTIONS] = {0};
    double node_util = 0;
    strcpy(tempHistory, history);
    sprintf(cardStr, "%hu", cards[acting_player]);
    strcat(infoset,cardStr);
    strcat(infoset, tempHistory);
    int infoIndex = getInfoIndex(infoset);
    if (acting_player==0){
        compute_strategy(infoIndex,&tempStrategy,p0,t); 
    }
    else{
        compute_strategy(infoIndex,&tempStrategy,p1,t); 
    }

    int pot_before = pot;
    for (int b=0; b<NUM_ACTIONS; b++){
        pot = pot_before;
        strcpy(tempHistory, history);
        sprintf(actionStr, "%d", b);
        strcpy(next_history, strcat(tempHistory, actionStr));
        strcpy(temp,next_history);
        pot += b;
        if (acting_player == 0){
            util[b] = -1*vanilla_cfr(cards, p0*tempStrategy[b], p1, next_history, pot, traversing_player, t);
        }
        else{
            util[b] = -1*vanilla_cfr(cards, p0, p1*tempStrategy[b], next_history, pot, traversing_player, t);
        }
        
        node_util += tempStrategy[b] * util[b];
    }
    free(tempStrategy);
    for (int c=0; c<NUM_ACTIONS; c++){
        tempRegrets[infoIndex][c] += (util[c] - node_util) * (acting_player == 0 ? p1 : p0);

    }

    return node_util;
}

void cfr(int iterations)
{
    unsigned short deck[NUM_CARDS] = {0};
    for (unsigned short i = 0; i < NUM_CARDS; i++)
    {
        deck[i] = i;
    }
    double util[2] = {0};
    unsigned short cards[2] = {0};
    int temp = 0;
    char history[MAX_HISTORY_LENGTH]={'\0'};
    for (int t=1; t<iterations + 1; t++){
        for (unsigned short g = 0; g < NUM_CARDS; g++)
        {
            for (unsigned short h = 0; h < NUM_CARDS; h++)
            {
                if (g!=h){
                    for (int i=0; i<2; i++){
                        cards[0] = g;
                        cards[1] = h;
                        util[i] += ((double)1/6)*vanilla_cfr(cards, 1, 1, history, 2, i, t);
                    }
                }
            }
        }
        for (int i=0; i<NUM_INFO; i++){
            regrets[i][0] += fmax(tempRegrets[i][0],0);
            regrets[i][1] += fmax(tempRegrets[i][1],0);
            tempRegrets[i][0] = 0;
            tempRegrets[i][1] = 0;
        }
    }
    printf("Average game value: %f\n", (util[0]/iterations));
    double myStrat[NUM_INFO][NUM_ACTIONS] = {0};
    double *tempStrategy;
    tempStrategy = NULL;
    for (int i=0; i<NUM_INFO; i++){
        compute_avg_strategy(i, &tempStrategy);
        myStrat[i][0] = tempStrategy[0];
        myStrat[i][1] = tempStrategy[1];
    }
    best_response(myStrat);
    save_strategy(myStrat);
    printf("EV: %f\n",ev(myStrat, brStrategy, 0));
    printf("EV: %f\n",ev(brStrategy, myStrat, 0));
    free(tempStrategy);
}

int main() {
    // Initialise the random number generator
    srand(time(NULL));
    cfr(100000);
    return 0;
}
