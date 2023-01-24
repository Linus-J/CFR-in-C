#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

//Figure out what the pot doing and how to reutrn money when a player folds pre and post flop


// Number of actions for each player at each stage of the game
#define NUM_ACTIONS 3

#define MAX_HISTORY_LENGTH 9 //Think this can be reduced

// Number of unique valued cards in a deck
#define NUM_CARDS 3

// Number of unique suits in a deck
#define NUM_SUITS 2

// Number of decision points spanning all the infosets in the game. 36*5*6=1080
#define NUM_INFO 1080

// Number of players in the game
#define NUM_PLAYERS 2

const char *CARDS[] = {"Jh", "Qh", "Kh", "Js", "Qs", "Ks",0};

const char *FLOPS[] = {"00", "11", "011", "121", "0121",0};

const char *TERMS[] = {"10", "120", "0120", "010", "1110", "11120", "110120", "11010", "11121", "1111", "11011", "110121", "1100", "12110", "121120", "1210120", "121010", "12111", "121121", "121011", "1210121", "12100", "01110", "011120", "0110120", "011010", "01111", "011121", "011011", "0110121", "01100", "012110", "0121120", "01210120", "0121010", "012111", "0121121", "0121011", "01210121", "012100", "0010", "00120", "000120", "00010", "0011", "00121", "00011", "000121", "0000",0};

// Initialise the strategy and regret arrays to 0
double strategy[NUM_INFO][NUM_ACTIONS] = {0};
double strategy_sum[NUM_INFO][NUM_ACTIONS] = {0};
double regrets[NUM_INFO][NUM_ACTIONS] = {0};
double tempRegrets[NUM_INFO][NUM_ACTIONS] = {0};

// Final computed strategies
double brStrategy[NUM_INFO][NUM_ACTIONS] = {0};
//double finalStrategy[NUM_INFO][NUM_ACTIONS] = {0};

int getInfoIndex(char infoStr[MAX_HISTORY_LENGTH], char flopStr[MAX_HISTORY_LENGTH]){
    //Perfect mapping of infoset to index, DOMAIN SPECIFIC TO VANILLA LEDUC POKER
    uint32_t infoLength = strlen(infoStr);
    uint32_t flopLength = 0;
    int temp = 0;
    int index = 0;
    if (flopStr[0] != '\0'){
        if (infoStr[0]<flopStr[0]){
            temp = -1;
        }
        flopLength = strlen(flopStr);
    }
    switch(infoLength){
        case 5:
        case 4:
            index = ((int)(infoStr[0])-'0')*180+30*5; //5*180 + 150 + 30
            break; // 012 -> 5
        case 3:
            index = ((int)(infoStr[0])-'0')*180+30*(((int)(infoStr[2]-'0'))+2);
            break; // 01 -> 3, 12 -> 4
        case 2:
            index = ((int)(infoStr[0]-'0'))*180+30*(((int)(infoStr[1]-'0'))+1); //36+30
            break; // 0 -> 1, 1 -> 2
        case 1:
            index = ((int)(infoStr[0]-'0'))*180;
            break; // '' -> 0
    }

    switch(flopLength){
        case 5:
        case 4:
            index += ((int)(flopStr[0])-'0'+temp)*6+6;
            break; // 012 -> 6
        case 3:
            index += ((int)(flopStr[0])-'0'+temp)*6+((int)(flopStr[2]-'0'))+3;
            break; // 01 -> 4, 12 -> 5
        case 2:
            index += ((int)(flopStr[0]-'0')+temp)*6+((int)(flopStr[1]-'0'))+2;
            break; // 0 -> 2, 1 -> 3
        case 1:
            index += ((int)(flopStr[0]-'0')+temp)*6+1;
            break; // '' -> 1
        default:
            break;
    }
    return index;
}
uint32_t validActInf(int index){
    int a = 0, info = 0, flopInfo = 0;
    bool flopped = false;
    a = index % 180;
    info = a / 30;
    a = a % 30;
    if (a > 5){
        flopped = true;
    }
    flopInfo = a % 6;
    uint32_t validActions = 2;
    if (flopped){
        if (flopInfo == 2 || flopInfo == 3){
            validActions = 3;
        }
    }else{
        if (info == 2 || info == 3){
            validActions = 3;
        }
    }
    return validActions;
}
void reverseInfoIndex(int index, char **infoStr){
    free(*infoStr);
    *infoStr = NULL;
    *infoStr = malloc(sizeof(char) * (MAX_HISTORY_LENGTH*2+5));
    int card = 0, a = 0, info = 0, flopCard = 0, flopInfo = 0;
    bool flopped = false;
    char tempStr[MAX_HISTORY_LENGTH*2+5] = {'\0'}, flopStr[MAX_HISTORY_LENGTH+5] = {'\0'};
    card = index / 180;
    a = index % 180;
    info = a / 30;
    a = a % 30;
    if (a > 5){
        flopped = true;
    }
    flopCard = a / 6;
    flopInfo = a % 6;

    sprintf(tempStr, "%d + ", card);
    if (flopped){
        if (card<flopCard){
            flopCard++;
        }
        sprintf(flopStr, "+ %d + ", flopCard);
    }
    switch (info){
        case 5:
            strcat(tempStr,"012");
            break;
        case 4:
            strcat(tempStr,"12");
            break;
        case 3:
            strcat(tempStr,"01");
            break; 
        case 2:
            strcat(tempStr,"1");
            break;
        case 1:
            strcat(tempStr,"0");
            break;
        case 0:
            strcat(tempStr,"");
            break; 
    }
    switch (flopInfo){
        case 5:
            strcat(flopStr,"012");
            break;
        case 4:
            strcat(flopStr,"12");
            break;
        case 3:
            strcat(flopStr,"01");
            break; 
        case 2:
            strcat(flopStr,"1");
            break;
        case 1:
            strcat(flopStr,"0");
            break;
        case 0:
            strcat(flopStr,"");
            break; 
    }
    strcat(tempStr,flopStr);
    int i = 0;
    for (i = 0; i<strlen(tempStr);i++){
        (*infoStr)[i]=tempStr[i];
    }
    (*infoStr)[i]= '\0';
}

// Helper function to compute the current strategy for a given player and card
void *compute_strategy(int infoIndex, double **tempStrategy, uint32_t validActions)
{
    free(*tempStrategy);
    *tempStrategy = malloc(validActions * sizeof(double));
    // Compute the normalizing factor
    
    double normalizing_factor = 0;
    for (uint32_t i = 0; i < validActions; i++)
    {
        if (regrets[infoIndex][i]>0){
            strategy[infoIndex][i] = regrets[infoIndex][i];
        }
        else{
            strategy[infoIndex][i] = 0;
        }
        normalizing_factor += strategy[infoIndex][i];
    }

    for (uint32_t i = 0; i < validActions; i++)
    {
        if (normalizing_factor > 0){ // If the normalizing factor is > 0, compute the strategy as the regret-matched strategy
            strategy[infoIndex][i] /= normalizing_factor;
        }
        else{ // Otherwise, the strategy is uniformly random
            strategy[infoIndex][i] = 1.0 / validActions;
        }
    }
    for (uint32_t i = 0; i < validActions; i++)
    {
        (*tempStrategy)[i] = strategy[infoIndex][i];
    }
}

void *compute_avg_strategy(int infoIndex, double **tempStrategy, uint32_t validActions)
{
    double avg_strategy[NUM_ACTIONS] = {0};
    double normalizing_sum = 0;
    free(*tempStrategy);
    *tempStrategy = malloc(validActions * sizeof(double));
    for (uint32_t i = 0; i < validActions; i++){
        normalizing_sum += strategy_sum[infoIndex][i];
    }

    for (uint32_t i = 0; i < validActions; i++){
        if (normalizing_sum > 0){
            avg_strategy[i] = strategy_sum[infoIndex][i] / normalizing_sum;
        }   
        else{
            avg_strategy[i] = 1.0 / validActions;
        }
    }
    for (uint32_t i = 0; i < validActions; i++)
    {
        (*tempStrategy)[i] = avg_strategy[i];
    }
}

void print_avg_strategy(int infoIndex, uint32_t validActions)
{
    double avg_strategy[NUM_ACTIONS] = {0};
    double normalizing_sum = 0;
    for (uint32_t i = 0; i < validActions; i++){
        normalizing_sum += strategy_sum[infoIndex][i];
    }

    for (uint32_t i = 0; i < validActions; i++){
        if (normalizing_sum > 0){
            avg_strategy[i] = strategy_sum[infoIndex][i] / normalizing_sum;
        }   
        else{
            avg_strategy[i] = 1.0 / validActions;
        }
    }
    if (validActions == 2){
        printf("[%f, %f]\n", avg_strategy[0], avg_strategy[1]);
    }
    else if (validActions == 3){
        printf("[%f, %f, %f]\n", avg_strategy[0], avg_strategy[1], avg_strategy[2]);
    }
}

bool isTerminal(char history[2*MAX_HISTORY_LENGTH]){
    uint32_t i = 0;
    while(TERMS[i]) {
        if(strcmp(TERMS[i], history) == 0) {
            return true;
        }
        i++;
    }
    return false;
}

bool isFlop(char history[MAX_HISTORY_LENGTH]){
    uint32_t i = 0;
    while(FLOPS[i]) {
        if(strcmp(FLOPS[i], history) == 0) {
            return true;
        }
        i++;
    }
    return false;
}

bool isValid(char history[MAX_HISTORY_LENGTH], char actionStr[2]){
    uint32_t i = 0;
    int plays = strlen(history);
    if (plays < 1){
        return (actionStr[0]=='0'||actionStr[0]=='1');
    }
    else if (history[plays-1] == '0'){
        return true;
    }
    else if (history[plays-1] == '1'){
        return true;
    }
    else{  
        return (actionStr[0]=='0'||actionStr[0]=='1');
    }
    return false;
}

int32_t getPayoff(uint32_t cards[3], char oldHistory[MAX_HISTORY_LENGTH], char flopHistory[MAX_HISTORY_LENGTH], uint32_t traversing_player, int pot){
    int plays = strlen(oldHistory);
    int acting_player = plays % 2;
    int opponent_player = 1 - acting_player;
    bool flopped = false;
    char history[MAX_HISTORY_LENGTH] = {0};
    strcpy(history, oldHistory);
    if (strlen(flopHistory)>1){
        plays = strlen(flopHistory);
        strcpy(history, flopHistory);
    }
    if (history[plays-1] == '0' && (history[plays-2] == '1' || history[plays-2] == '2')){ //not showdown
        int prevBet = 0;
        if (flopped){
            prevBet = history[plays-2]*2;
        }
        else{
            prevBet = history[plays-2];
        }
        if (acting_player == traversing_player){
            return (pot-prevBet)/2;
        }
        else{
            return -(pot-prevBet)/2;
        }
    }
    //showdown
    if (acting_player == traversing_player){
        if (cards[acting_player]%4 == cards[2]%4){
            return pot/2;
        }
        else if (cards[opponent_player]%4 == cards[2]%4){
            return -pot/2;
        }
        if (cards[acting_player]%4 > cards[opponent_player]%4){
            return pot/2;
        }
        else{
            return -pot/2;
        }
    }
    else{
        if (cards[acting_player]%4 == cards[2]%4){
            return -pot/2;
        }
        else if (cards[opponent_player]%4 == cards[2]%4){
            return pot/2;
        }
        if (cards[acting_player]%4 > cards[opponent_player]%4){
            return -pot/2;
        }
        else{
            return pot/2;
        }
    }
}

double calc_best_response(uint32_t cards[3], char history[MAX_HISTORY_LENGTH], int pot, uint32_t traversing_player, char flopHistory[MAX_HISTORY_LENGTH], double prob, double player[NUM_INFO][NUM_ACTIONS]){
    uint32_t plays = strlen(history);
    uint32_t acting_player = plays % 2;
    uint32_t opponent_player = 1 - acting_player;
    char currentHistory[2*MAX_HISTORY_LENGTH]={'\0'};
    strcat(currentHistory,history);
    strcat(currentHistory,flopHistory);
    if (isTerminal(currentHistory)){
        return getPayoff(cards,history,flopHistory,traversing_player,pot);
    }
    bool flopped = (isFlop(history) || flopHistory[0]!='\0');
    char next_history[MAX_HISTORY_LENGTH+2]={'\0'};
    char tempHistory[MAX_HISTORY_LENGTH]={'\0'};
    char infoset[MAX_HISTORY_LENGTH+2]={'\0'};
    char flopInfoset[MAX_HISTORY_LENGTH+2]={'\0'};
    char actionStr[2]={'\0'};
    char cardStr[2]={'\0'};
    
    strcpy(tempHistory, flopHistory);
    sprintf(cardStr, "%hu", cards[2]);
    strcat(flopInfoset,cardStr);
    strcat(flopInfoset, tempHistory);
    if (flopped){
        plays = strlen(flopHistory);
        acting_player = plays % 2;
        opponent_player = 1 - acting_player;
    }
    if (acting_player == traversing_player){
        double vals[NUM_ACTIONS] = {0};
        double brVal = 0;
        strcpy(tempHistory, history);
        sprintf(cardStr, "%hu", cards[acting_player]);
        strcat(infoset,cardStr);
        strcat(infoset, tempHistory);
        int infoIndex = 0;
        uint32_t validActions = 2;
        if (flopped){
            infoIndex = getInfoIndex(infoset,flopInfoset);
            if (flopHistory[strlen(flopHistory)-1]=='1'){
                validActions = 3;
            }
        }else{
            infoIndex = getInfoIndex(infoset,"\0");
            uint32_t tplays = strlen(history);
            if (tplays>0){
                if (history[tplays-1]=='1'){
                    validActions = 3;
                }
            }
        }
        for (uint32_t b=0; b<validActions; b++){
            sprintf(actionStr, "%hu", b);
            strcpy(tempHistory, "");
            strcat(tempHistory,history);
            strcat(tempHistory,flopHistory);

            if (flopped){
                strcpy(tempHistory, flopHistory);
                strcpy(next_history, strcat(tempHistory, actionStr));
                pot += b*2;
                vals[b] = calc_best_response(cards, history, pot, traversing_player, next_history, prob, player);
            }
            else{
                strcpy(tempHistory, history);
                strcpy(next_history, strcat(tempHistory, actionStr));
                pot += b; 
                vals[b] = calc_best_response(cards, next_history, pot, traversing_player, flopHistory, prob, player);
            }
            if (b==0){
                brVal = vals[b];
            }
            else{
                brVal = fmax(brVal,vals[b]);
            }
        }
        for (uint32_t c=0; c<validActions; c++){
            brStrategy[infoIndex][c] += prob*vals[c];
        }
        return -brVal;
    }
    else{
        double tempStrat[NUM_ACTIONS] = {0};
        double vals[NUM_ACTIONS] = {0};
        double node_util = 0;
        strcpy(tempHistory, history);
        sprintf(cardStr, "%hu", cards[acting_player]);
        strcat(infoset,cardStr);
        strcat(infoset, tempHistory);
        int infoIndex = 0;
        uint32_t validActions = 2;
        if (flopped){
            infoIndex = getInfoIndex(infoset,flopInfoset);
            if (flopHistory[strlen(flopHistory)-1]=='1'){
                validActions = 3;
            }
        }else{
            infoIndex = getInfoIndex(infoset,"\0");
            uint32_t tplays = strlen(history);
            if (tplays>0){
                if (history[tplays-1]=='1'){
                    validActions = 3;
                }
            }
        } 
        for (uint32_t i=0; i<validActions; i++){
            tempStrat[i] = player[infoIndex][i];
        }
        for (uint32_t b=0; b<validActions; b++){
            sprintf(actionStr, "%hu", b);
            strcpy(tempHistory, "");
            strcat(tempHistory,history);
            strcat(tempHistory,flopHistory);

            if (flopped){
                strcpy(tempHistory, flopHistory);
                strcpy(next_history, strcat(tempHistory, actionStr));
                pot += b*2; 
                vals[b] = calc_best_response(cards, history, pot, traversing_player, next_history, tempStrat[b]*prob, player);
            }
            else{
                strcpy(tempHistory, history);
                strcpy(next_history, strcat(tempHistory, actionStr));
                pot += b; 
                vals[b] = calc_best_response(cards, next_history, pot, traversing_player, flopHistory, tempStrat[b]*prob, player);
            }
            node_util += tempStrat[b] * vals[b];
        }
        return -node_util;
    }
}

void best_response(double player[NUM_INFO][NUM_ACTIONS]){
    uint32_t total_cards = 0;
    total_cards = NUM_CARDS*NUM_SUITS;
    // Initialise cards
    uint32_t deck[NUM_CARDS*NUM_SUITS] = {0};
    char history[MAX_HISTORY_LENGTH]={'\0'};
    char flopHistory[MAX_HISTORY_LENGTH]={'\0'};
    for (uint32_t i = 0; i < total_cards; i++)
    {
        deck[i] = i;
    }
    uint32_t cards[NUM_PLAYERS+1] = {0};
    for (uint32_t f = 0; f < total_cards; f++)
    {
        for (uint32_t g = 0; g < total_cards; g++)
        {
            if (g!=f){
            for (uint32_t h = 0; h < total_cards; h++)
                {
                    if (g!=h && f!=h){
                        for (uint32_t i=0; i<NUM_PLAYERS; i++){ //Number of players is two
                            cards[0] = f;
                            cards[1] = g;
                            cards[2] = h;
                            calc_best_response(cards, history, 2, i, flopHistory, 1, player);
                        }
                    }
                }
            }
        }
    }
    uint32_t validActions = 0, temp = 0, ind = 0;
    for (uint32_t i=0; i<NUM_INFO; i++){
        validActions = validActInf(i);
        for (uint32_t j=0; j<validActions; j++){
            if (temp < brStrategy[i][j]){  
                temp = brStrategy[i][j];
                ind = j;
            }
        }
        for (uint32_t j=0; j<validActions; j++){
            if (j == ind){  
                brStrategy[i][j] = 1;
            }
            else{
                brStrategy[i][j] = 0;
            }
        }
    }
}

double calc_ev(uint32_t cards[3], char history[MAX_HISTORY_LENGTH], int pot, uint32_t traversing_player, char flopHistory[MAX_HISTORY_LENGTH], double p1[NUM_INFO][NUM_ACTIONS], double p2[NUM_INFO][NUM_ACTIONS]){
    char currentHistory[2*MAX_HISTORY_LENGTH]={'\0'};
    strcat(currentHistory,history);
    strcat(currentHistory,flopHistory);
    if (isTerminal(currentHistory)){
        return getPayoff(cards,history,flopHistory,traversing_player,pot);
    }
    bool flopped = (isFlop(history) || flopHistory[0]!='\0');
    char next_history[MAX_HISTORY_LENGTH+2]={'\0'};
    char tempHistory[MAX_HISTORY_LENGTH]={'\0'};
    char infoset[MAX_HISTORY_LENGTH+2]={'\0'};
    char flopInfoset[MAX_HISTORY_LENGTH+2]={'\0'};
    char actionStr[2]={'\0'};
    char cardStr[2]={'\0'};
    
    strcpy(tempHistory, flopHistory);
    sprintf(cardStr, "%hu", cards[2]);
    strcat(flopInfoset,cardStr);
    strcat(flopInfoset, tempHistory);
    uint32_t plays = strlen(history);
    uint32_t acting_player = plays % 2;
    uint32_t opponent_player = 1 - acting_player;
    if (flopped){
        plays = strlen(flopHistory);
        acting_player = plays % 2;
        opponent_player = 1 - acting_player;
    }

    strcpy(tempHistory, history);
    sprintf(cardStr, "%hu", cards[acting_player]);
    strcat(infoset,cardStr);
    strcat(infoset, tempHistory);
    int infoIndex = 0;
    uint32_t validActions = 2;
    if (flopped){
        infoIndex = getInfoIndex(infoset,flopInfoset);
        if (flopHistory[strlen(flopHistory)-1]=='1'){
            validActions = 3;
        }
    }else{
        infoIndex = getInfoIndex(infoset,"\0");
        uint32_t tplays = strlen(history);
        if (tplays>0){
            if (history[tplays-1]=='1'){
                validActions = 3;
            }
        }
    }
    double util = 0;
    double tempStrat[NUM_ACTIONS] = {0};
    if (acting_player == traversing_player){
        for (uint32_t i=0; i<validActions; i++){
            tempStrat[i] = p1[infoIndex][i];
        }
    }
    else{
        for (uint32_t i=0; i<validActions; i++){
            tempStrat[i] = p2[infoIndex][i];
        }
    }
    double vals[NUM_ACTIONS] = {0};
    for (uint32_t b=0; b<validActions; b++){
        sprintf(actionStr, "%hu", b);
        strcpy(tempHistory, "");
        strcat(tempHistory,history);
        strcat(tempHistory,flopHistory);

        if (flopped){
            strcpy(tempHistory, flopHistory);
            strcpy(next_history, strcat(tempHistory, actionStr));
            pot += b*2;
            vals[b] = calc_ev(cards, history, pot, traversing_player, next_history, p1, p2);
        }
        else{
            strcpy(tempHistory, history);
            strcpy(next_history, strcat(tempHistory, actionStr));
            pot += b; 
            vals[b] = calc_ev(cards, next_history, pot, traversing_player, flopHistory, p1, p2);
        }
    }
}

double ev(double p1[NUM_INFO][NUM_ACTIONS], double p2[NUM_INFO][NUM_ACTIONS], uint32_t traversing_player){
    uint32_t total_cards = NUM_CARDS*NUM_SUITS, evalue = 0;
    // Initialise cards
    uint32_t deck[NUM_CARDS*NUM_SUITS] = {0};
    char history[MAX_HISTORY_LENGTH]={'\0'};
    char flopHistory[MAX_HISTORY_LENGTH]={'\0'};
    for (uint32_t i = 0; i < total_cards; i++)
    {
        deck[i] = i;
    }
    uint32_t cards[NUM_PLAYERS+1] = {0};
    for (uint32_t f = 0; f < total_cards; f++)
    {
        for (uint32_t g = 0; g < total_cards; g++)
        {
            if (g!=f){
            for (uint32_t h = 0; h < total_cards; h++)
                {
                    if (g!=h && f!=h){
                        cards[0] = f;
                        cards[1] = g;
                        cards[2] = h;
                        evalue += ((double)1/120)*calc_ev(cards, history, 2, traversing_player, flopHistory, p1, p2);
                    }
                }
            }
        }
    }
    return evalue;
}

double vanilla_cfr(uint32_t cards[3], char history[MAX_HISTORY_LENGTH], int pot, uint32_t traversing_player, int t, char flopHistory[MAX_HISTORY_LENGTH]){
    uint32_t plays = strlen(history);
    uint32_t acting_player = plays % 2;
    uint32_t opponent_player = 1 - acting_player;
    char currentHistory[2*MAX_HISTORY_LENGTH]={'\0'};
    strcat(currentHistory,history);
    strcat(currentHistory,flopHistory);
    if (isTerminal(currentHistory)){
        return getPayoff(cards,history,flopHistory,traversing_player,pot);
    }
    bool flopped = (isFlop(history) || flopHistory[0]!='\0');
    char next_history[MAX_HISTORY_LENGTH+2]={'\0'};
    char tempHistory[MAX_HISTORY_LENGTH]={'\0'};
    char infoset[MAX_HISTORY_LENGTH+2]={'\0'};
    char flopInfoset[MAX_HISTORY_LENGTH+2]={'\0'};
    char actionStr[2]={'\0'};
    char cardStr[2]={'\0'};
    
    strcpy(tempHistory, flopHistory);
    sprintf(cardStr, "%hu", cards[2]);
    strcat(flopInfoset,cardStr);
    strcat(flopInfoset, tempHistory);
    if (flopped){
        plays = strlen(flopHistory);
        acting_player = plays % 2;
        opponent_player = 1 - acting_player;
    }
    if (acting_player == traversing_player){
        double *tempStrategy;
        tempStrategy = NULL;
        double util[NUM_ACTIONS] = {0};
        double node_util = 0;
        strcpy(tempHistory, history);
        sprintf(cardStr, "%hu", cards[acting_player]);
        strcat(infoset,cardStr);
        strcat(infoset, tempHistory);
        int infoIndex = 0;
        uint32_t validActions = 2;
        if (flopped){
            infoIndex = getInfoIndex(infoset,flopInfoset);
            if (flopHistory[strlen(flopHistory)-1]=='1'){
                validActions = 3;
            }
        }else{
            infoIndex = getInfoIndex(infoset,"\0");
            uint32_t tplays = strlen(history);
            if (tplays>0){
                if (history[tplays-1]=='1'){
                    validActions = 3;
                }
            }
        }       
        
        compute_strategy(infoIndex,&tempStrategy, validActions);
        for (uint32_t b=0; b<validActions; b++){
            sprintf(actionStr, "%hu", b);
            strcpy(tempHistory, "");
            strcat(tempHistory,history);
            strcat(tempHistory,flopHistory);

            if (flopped){
                strcpy(tempHistory, flopHistory);
                strcpy(next_history, strcat(tempHistory, actionStr));
                pot += b*2; 
                util[b] = vanilla_cfr(cards, history, pot, traversing_player, t, next_history);
            }
            else{
                strcpy(tempHistory, history);
                strcpy(next_history, strcat(tempHistory, actionStr));
                pot += b; 
                util[b] = vanilla_cfr(cards, next_history, pot, traversing_player, t, flopHistory);
            }
            node_util += tempStrategy[b] * util[b];
            //}
        }
        free(tempStrategy);
        for (uint32_t c=0; c<validActions; c++){
            tempRegrets[infoIndex][c] += util[c] - node_util;
        }
        return node_util;
    }
    else{ 
        double *tempStrategy;
        tempStrategy = NULL;
        strcpy(tempHistory, history);
        sprintf(cardStr, "%hu", cards[acting_player]);
        strcat(infoset,cardStr);
        strcat(infoset, tempHistory);
        int infoIndex = 0;
        uint32_t validActions = 2;
        if (flopped){
            infoIndex = getInfoIndex(infoset,flopInfoset);
            if (flopHistory[strlen(flopHistory)-1]=='1'){
                validActions = 3;
            }
        }else{
            infoIndex = getInfoIndex(infoset,"\0");
            uint32_t tplays = strlen(history); //maybe redundant variable
            if (tplays>0){
                if (history[tplays-1]=='1'){
                    validActions = 3;
                }
            }
        }

        compute_strategy(infoIndex,&tempStrategy,validActions);
        double tempUtil = 0;
        double randNum = ((double)rand())/RAND_MAX;
        if (randNum < tempStrategy[0]){
            if (flopped){
                sprintf(actionStr, "%hu", 0);
                strcpy(tempHistory, flopHistory);
                strcpy(next_history, strcat(tempHistory, actionStr));
                tempUtil = vanilla_cfr(cards, history, pot, traversing_player, t, next_history);
            }
            else{
                sprintf(actionStr, "%hu", 0);
                strcpy(next_history, strcat(tempHistory, actionStr));
                tempUtil = vanilla_cfr(cards, next_history, pot, traversing_player, t, flopHistory);
            }
        }
        else if (randNum < tempStrategy[0]+tempStrategy[1]){
            if (flopped){
                sprintf(actionStr, "%hu", 1);
                strcpy(tempHistory, flopHistory);
                strcpy(next_history, strcat(tempHistory, actionStr));
                pot += 2;
                tempUtil = vanilla_cfr(cards, history, pot, traversing_player, t, next_history);
            }
            else{
                sprintf(actionStr, "%hu", 1);
                strcpy(next_history, strcat(tempHistory, actionStr));
                pot += 1;
                tempUtil = vanilla_cfr(cards, next_history, pot, traversing_player, t, flopHistory);
            }
        }
        else{
            if (flopped){
                sprintf(actionStr, "%hu", 2);
                strcpy(tempHistory, flopHistory);
                strcpy(next_history, strcat(tempHistory, actionStr));
                pot += 4;
                tempUtil = vanilla_cfr(cards, history, pot, traversing_player, t, next_history);
            }
            else{
                sprintf(actionStr, "%hu", 2);
                strcpy(next_history, strcat(tempHistory, actionStr));
                pot += 2;
                tempUtil = vanilla_cfr(cards, next_history, pot, traversing_player, t, flopHistory);
            }
        }
        for (uint32_t a=0; a<validActions; a++){
            strategy_sum[infoIndex][a] += pow(((double)t/(t+1)),2)*tempStrategy[a];
        }
        free(tempStrategy);
        return tempUtil;
    }
}

void cfr(int iterations)
{
    uint32_t total_cards = 0;
    total_cards = NUM_CARDS*NUM_SUITS;
    // Initialise cards
    uint32_t deck[NUM_CARDS*NUM_SUITS] = {0};
    for (uint32_t i = 0; i < total_cards; i++)
    {
        deck[i] = i;
    }
    double util[NUM_ACTIONS] = {0};
    uint32_t cards[NUM_PLAYERS+1] = {0}; //+1 for flop card
    char history[MAX_HISTORY_LENGTH]={'\0'};
    char flopHistory[MAX_HISTORY_LENGTH]={'\0'};
    for (int t=1; t<iterations + 1; t++){
        if (t%10000==0){
            printf("Iteration: %d\n", t);
        }
        for (uint32_t f = 0; f < total_cards; f++)
        {
            for (uint32_t g = 0; g < total_cards; g++)
            {
                if (g!=f){
                for (uint32_t h = 0; h < total_cards; h++)
                    {
                        if (g!=h && f!=h){
                            for (uint32_t i=0; i<NUM_PLAYERS; i++){ //Number of players is two
                                cards[0] = f;
                                cards[1] = g;
                                cards[2] = h;
                                util[i] += ((double)1/120)*vanilla_cfr(cards, history, 2, i, t, flopHistory);
                            }
                        }
                    }
                }
            }
        }
        for (uint32_t i=0; i<NUM_INFO; i++){
            if (tempRegrets[i][0]>0){
                regrets[i][0] += ((double)pow(t,1.5)/(pow(t,1.5)+1))*tempRegrets[i][0];
            }
            else{
                regrets[i][0] += 0.5*tempRegrets[i][0];
            }
            if (tempRegrets[i][1]>0){
                regrets[i][1] += ((double)pow(t,1.5)/(pow(t,1.5)+1))*tempRegrets[i][1];
            }
            else{
                regrets[i][1] += 0.5*tempRegrets[i][1];
            }
            if (tempRegrets[i][2]>0){
                regrets[i][2] += ((double)pow(t,1.5)/(pow(t,1.5)+1))*tempRegrets[i][2];
            }
            else{
                regrets[i][2] += 0.5*tempRegrets[i][2];
            }
            tempRegrets[i][0] = 0;
            tempRegrets[i][1] = 0;
            tempRegrets[i][2] = 0;
        }
    }
    printf("Average game value: %f\n", (util[0]/iterations));
    char *infoStr;
    infoStr = NULL;
    infoStr = malloc(sizeof(char) * (MAX_HISTORY_LENGTH*2+5));
    uint32_t validActions = 0;
    for (uint32_t i=0; i<NUM_INFO; i++){
        validActions = validActInf(i);
        reverseInfoIndex(i, &infoStr);
        printf("%s: ", infoStr);
        print_avg_strategy(i, validActions);
    }
    free(infoStr);
    //Calc br etc
    double myStrat[NUM_INFO][NUM_ACTIONS] = {0};
    double *tempStrategy;
    tempStrategy = NULL;
    for (uint32_t i=0; i<NUM_INFO; i++){
        validActions = validActInf(i);
        compute_avg_strategy(i, &tempStrategy, validActions);
        for (uint32_t j=0; j<validActions; j++){
            myStrat[i][j] = tempStrategy[j];
        }
    }
    free(tempStrategy);
    best_response(myStrat);
    printf("EV: %f\n",ev(myStrat, brStrategy, 0));
}

int main() {
    // Initialise the random number generator
    srand(time(NULL));
    cfr(100); //iterations
    return 0;
}