#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>

// Number of actions for each player at each stage of the game
#define NUM_ACTIONS 3

#define MAX_HISTORY_LENGTH 9 //Think this can be reduced

// Number of unique valued cards in a deck
#define NUM_CARDS 3

// Number of unique suits in a deck
#define NUM_SUITS 2

// Number of decision points spanning all the infosets in the game. 36*5*6=1080
#define NUM_INFO 936

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

int getInfoIndex(char infoStr[MAX_HISTORY_LENGTH], char flopStr[MAX_HISTORY_LENGTH], int flopLength){
    //Perfect mapping of infoset to index, DOMAIN SPECIFIC TO VANILLA LEDUC POKER
    uint32_t infoLength = strlen(infoStr);
    int temp = 0;
    int index = 0;
    if (flopLength != 0){
        if (infoStr[0]<flopStr[0]){
            temp = -1;
        }
        infoStr[infoLength-1]='\0';
        infoLength--;
    }
    switch(infoLength){ //shouldnt be anything for "",0,1
        case 4:
            index = ((int)(infoStr[0])-'0')*156+31*4+1; //5*152 + 120 + 1
            break; // 012 -> 5
        case 3:
            index = ((int)(infoStr[0])-'0')*156+31*(((int)(infoStr[2]-'0'))+1)+1;
            break; // 01 -> 3, 12 -> 4
        case 2:
            index = ((int)(infoStr[0]-'0'))*156+31*(((int)(infoStr[1]-'0')))+1; //36+30
            break; // 0 -> 1, 1 -> 2
        case 1:
            index = ((int)(infoStr[0]-'0'))*156;
            break; // '' -> 0
    }
    switch(flopLength){
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
            //assert(false);
            break;
    }
    return index;
}
uint32_t validActInf(int index){
    int a = 0, info = 0, flopInfo = 0;
    bool flopped = false;

    
    a = index % 156;
    if (a == 0){
        info = 0;
    }
    else{
        info = ((a-1) / 31) + 1;

        a = (a-1) % 31;
        if (a > 0){
            flopped = true;
        }
    }
    flopInfo = (a-1) % 6 ;

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
    
    card = index / 156;
    a = index % 156;
    if (a == 0){
        info = 0;
    }
    else{
        info = ((a-1) / 31) + 1;

        a = (a-1) % 31;
        if (a > 0){
            flopped = true;
        }
    }
    flopCard = (a-1) / 6;
    flopInfo = (a-1) % 6 ;
    
    sprintf(tempStr, "%d + ", card);
    if (flopped){
        if (card<=flopCard){
            flopCard++;
        }
        sprintf(flopStr, " + %d + ", flopCard);
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
    if (flopped){
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
    }
    strcat(tempStr,flopStr);
    int i = 0;
    for (i = 0; i<strlen(tempStr);i++){
        (*infoStr)[i]=tempStr[i];
    }
    (*infoStr)[i]= '\0';
}

// Helper function to compute the current strategy for a given player and card
void *compute_strategy(int infoIndex, double **tempStrategy, uint32_t validActions, double realizationWeight, int t)
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
        if (realizationWeight != -1){
            strategy_sum[infoIndex][i] += realizationWeight * strategy[infoIndex][i] * pow(((double)t/(t+1)),2);
        }
    }
    for (uint32_t i = 0; i < validActions; i++)
    {
        (*tempStrategy)[i] = strategy[infoIndex][i];
    }
}

void *compute_avg_strategy(int infoIndex, double **tempStrategy)
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
    for (uint32_t i = 0; i < NUM_ACTIONS; i++){
        normalizing_sum += strategy_sum[infoIndex][i];
    }

    for (uint32_t i = 0; i < NUM_ACTIONS; i++){
        if (normalizing_sum > 0){
            avg_strategy[i] = strategy_sum[infoIndex][i] / normalizing_sum;
        }   
        else{
            avg_strategy[i] = 1.0 / NUM_ACTIONS;
        }
    }
    if (NUM_ACTIONS == 2){
        printf("[%f, %f]\n", avg_strategy[0], avg_strategy[1]);
    }
    else if (NUM_ACTIONS == 3){
        printf("[%f, %f, %f]\n", avg_strategy[0], avg_strategy[1], avg_strategy[2]);
    }
}

void save_strategy(double player[NUM_INFO][NUM_ACTIONS]){
    FILE *fptr;
    uint32_t validActions = 0;
    fptr = fopen("leducstrat.txt","w");
    if(fptr == NULL){
        printf("File error!\n");
        exit(1);
    }
    for (uint32_t i=0; i<NUM_INFO; i++){
        validActions = validActInf(i);
        for (uint32_t j=0; j<validActions; j++){
            if (j == validActions-1){
                fprintf(fptr,"%f\n",player[i][j]);
            }
            else{
                fprintf(fptr,"%f,",player[i][j]);
            }
        }
    }
    fclose(fptr);
}

void load_strategy(double **player){
    FILE *fptr;
    uint32_t validActions = 0;
    fptr = fopen("leducstrat.txt","r");
    if(fptr == NULL){
        printf("File error!");
        exit(1);
    }
    double *data = (double *) malloc(NUM_INFO * 3 * sizeof(double));
    if (data == NULL) {
        printf("Error allocating memory\n");
        fclose(fptr);
        return;
    }
    // set the pointers for each row of the 2D array
    for (int i = 0; i < NUM_INFO; i++) {
        player[i] = &data[i * 3];
    }
    int row = 0;
    while (!feof(fptr) && row < NUM_INFO) {
        double value1=0, value2=0, value3=0;
        validActions = validActInf(row);
        if (validActions==2){
            fscanf(fptr, "%lf,%lf", &value1, &value2);
            player[row][0] = value1;
            player[row][1] = value2;
        }
        else{
            fscanf(fptr, "%lf,%lf,%lf", &value1, &value2, &value3);
            player[row][0] = value1;
            player[row][1] = value2;
            player[row][2] = value3;
        }
        row++;
    }
    fclose(fptr);
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

int getPayoff(uint32_t cards[3], char oldHistory[MAX_HISTORY_LENGTH], char flopHistory[MAX_HISTORY_LENGTH], int pot){
    int plays = strlen(oldHistory);
    char history[MAX_HISTORY_LENGTH] = {'\0'};
    char currentHistory[2*MAX_HISTORY_LENGTH]={'\0'};
    strcat(currentHistory,oldHistory);
    strcat(currentHistory,flopHistory);
    bool flopped = false;
    strcpy(history, oldHistory);
    if (strlen(flopHistory)>1){
        plays = strlen(currentHistory);
        strcpy(history, flopHistory);
        flopped = true;
    }
    int acting_player = plays % 2;
    int opponent_player = 1 - acting_player;
    
    //printf("history: %s, acting: %d, plays: %d\n", currentHistory,acting_player, plays);
    if (currentHistory[plays-1] == '0' && (currentHistory[plays-2] == '1' || currentHistory[plays-2] == '2')){ //not showdown
        int prevBet = 0;
        if (flopped){
            prevBet = ((currentHistory[plays-2]-'0')>0)*4;
        }
        else{
            prevBet = ((currentHistory[plays-2] - '0')>0)*2;
        }
        if (acting_player==0)
            return (pot-prevBet)/2;
        else
            return -(pot-prevBet)/2;
    }
    if (cards[acting_player]%3 == cards[2]%3){
        return pot/2;
    }
    else if (cards[opponent_player]%3 == cards[2]%3){
        return -pot/2;
    }
    if (cards[acting_player]%3 > cards[opponent_player]%3){
        return pot/2;
    }
    else{
        return -pot/2;
    }
}

double cbr(uint32_t cards[3], double p0, double p1,  char history[MAX_HISTORY_LENGTH], int pot, uint32_t traversing_player, char flopHistory[MAX_HISTORY_LENGTH], double player[NUM_INFO][NUM_ACTIONS]){
    uint32_t plays = strlen(history);
    uint32_t acting_player = plays % 2;
    uint32_t opponent_player = 1 - acting_player;
    char currentHistory[2*MAX_HISTORY_LENGTH]={'\0'};
    strcat(currentHistory,history);
    strcat(currentHistory,flopHistory);
    //printf("cur: %s\n",currentHistory);
    if (isTerminal(currentHistory)){
        //printf("cur: %s\n",currentHistory);
        return getPayoff(cards,history,flopHistory,pot);
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
        plays = strlen(currentHistory);
        acting_player = plays % 2;
        opponent_player = 1 - acting_player;
    }
    double tempStrategy[NUM_ACTIONS] = {0};
    double util[NUM_ACTIONS] = {0};
    double node_util = 0;
    strcpy(tempHistory, history);
    sprintf(cardStr, "%hu", cards[acting_player]);
    strcat(infoset,cardStr);
    strcat(infoset, tempHistory);
    int infoIndex = 0;
    uint32_t validActions = 2;
    if (flopped){
        infoIndex = getInfoIndex(infoset,flopInfoset,strlen(flopInfoset));
        if (flopHistory[strlen(flopHistory)-1]=='1'){
            validActions = 3;
        }
    }else{
        infoIndex = getInfoIndex(infoset,NULL,0);
        uint32_t tplays = strlen(history);
        if (tplays>0){
            if (history[tplays-1]=='1'){
                validActions = 3;
            }
        }
    }
    for (uint32_t c=0; c<validActions; c++){
        tempStrategy[c] = player[infoIndex][c];
    }
    for (uint32_t b=0; b<validActions; b++){
        sprintf(actionStr, "%hu", b);
        strcpy(tempHistory, "");
        strcat(tempHistory,history);
        strcat(tempHistory,flopHistory);

        if (flopped){
            strcpy(tempHistory, flopHistory);
            strcat(tempHistory, actionStr);
            strcpy(next_history, tempHistory);
            pot += b*4; 
            if (acting_player == 0){
                util[b] = -1*cbr(cards, p0*tempStrategy[b], p1, history, pot, traversing_player, next_history, player);
            }
            else{
                util[b] = -1*cbr(cards, p0, p1*tempStrategy[b], history, pot, traversing_player, next_history, player);
            }
        }
        else{
            strcpy(tempHistory, history);
            strcat(tempHistory, actionStr);
            strcpy(next_history, tempHistory);
            pot += b*2; 
            if (acting_player == 0){
                util[b] = -1*cbr(cards, p0*tempStrategy[b], p1, next_history, pot, traversing_player, flopHistory, player);
            }
            else{  
                util[b] = -1*cbr(cards, p0, p1*tempStrategy[b], next_history, pot, traversing_player, flopHistory, player);
            }
        }
        node_util += tempStrategy[b] * util[b];
    }
    
    for (uint32_t c=0; c<validActions; c++){
        if (acting_player == 0){
            brStrategy[infoIndex][c] += p0 * util[c];
        }
        else{
            brStrategy[infoIndex][c] += p1 * util[c];
        }
    }
    return node_util;
}

void best_response(double player[NUM_INFO][NUM_ACTIONS]){
    uint32_t total_cards = 0;
    total_cards = NUM_CARDS*NUM_SUITS;
    // Initialise cards
    char history[MAX_HISTORY_LENGTH]={'\0'};
    char flopHistory[MAX_HISTORY_LENGTH]={'\0'};
    double util[2] = {0};
    for (uint32_t i = 0; i < NUM_INFO; i++){
        for (uint32_t j = 0; j < NUM_ACTIONS; j++){
            brStrategy[i][j] = 0;
        }
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
                            util[i] += ((double)1/120)*cbr(cards, 1, 1, history, 2, i, flopHistory, player);
                        }
                    }
                }
            }
        }
    }
    printf("BR game value: %f\n", (util[0]));
    uint32_t validActions = 0, ind = 0;
    double temp = 0;
    for (int i=0; i<NUM_INFO; i++){
        validActions = validActInf(i);
        temp = -100000000;
        for (int j=0; j<validActions; j++){
            if (temp < brStrategy[i][j]){  
                temp = brStrategy[i][j];
                ind = j;
            }
        }
        for (int j=0; j<validActions; j++){
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
    uint32_t plays = strlen(history);
    uint32_t acting_player = plays % 2;
    uint32_t opponent_player = 1 - acting_player;
    char currentHistory[2*MAX_HISTORY_LENGTH]={'\0'};
    strcat(currentHistory,history);
    strcat(currentHistory,flopHistory);
    //printf("cur: %s\n",currentHistory);
    if (isTerminal(currentHistory)){
        //printf("cur: %s\n",currentHistory);
        return getPayoff(cards,history,flopHistory,pot);
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
        plays = strlen(currentHistory);
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
        infoIndex = getInfoIndex(infoset,flopInfoset,strlen(flopInfoset));
        if (flopHistory[strlen(flopHistory)-1]=='1'){
            validActions = 3;
        }
    }else{
        infoIndex = getInfoIndex(infoset,NULL,0);
        uint32_t tplays = strlen(history);
        if (tplays>0){
            if (history[tplays-1]=='1'){
                validActions = 3;
            }
        }
    }
    double util = 0;
    double tempStrat[NUM_ACTIONS] = {0};
    if (traversing_player==0){
        for (uint32_t i=0; i<validActions; i++){
            tempStrat[i] = p1[infoIndex][i];
        }
    }
    else{
        for (uint32_t i=0; i<validActions; i++){
            tempStrat[i] = p2[infoIndex][i];
        }
    }
    if (infoIndex>NUM_INFO){
        printf("his: %s + %s ind: %d\n",infoset,flopInfoset, infoIndex);
    }
    double vals[NUM_ACTIONS] = {0};
    for (uint32_t b=0; b<validActions; b++){
        sprintf(actionStr, "%hu", b);
        strcpy(tempHistory, "");
        strcat(tempHistory,history);
        strcat(tempHistory,flopHistory);

        if (flopped){
            strcpy(tempHistory, flopHistory);
            strcat(tempHistory, actionStr);
            strcpy(next_history, tempHistory);
            pot += b*4;
            vals[b] = calc_ev(cards, history, pot, 1-traversing_player, next_history, p1, p2);
        }
        else{
            strcpy(tempHistory, history);
            strcat(tempHistory, actionStr);
            strcpy(next_history, tempHistory);
            pot += b*2; 
            vals[b] = calc_ev(cards, next_history, pot, 1-traversing_player, flopHistory, p1, p2);
        }
        util += vals[b]*tempStrat[b]; 
        //printf("util: %f\n",util);
    } 
    return -util;
}

double ev(double p1[NUM_INFO][NUM_ACTIONS], double p2[NUM_INFO][NUM_ACTIONS], uint32_t traversing_player){
    uint32_t total_cards = 0;
    total_cards = NUM_CARDS*NUM_SUITS;
    // Initialise cards
    uint32_t deck[NUM_CARDS*NUM_SUITS] = {0};
    for (uint32_t i = 0; i < total_cards; i++)
    {
        deck[i] = i;
    }
    double evalue = 0;
    uint32_t cards[NUM_PLAYERS+1] = {0}; //+1 for flop card
    char history[MAX_HISTORY_LENGTH]={'\0'};
    char flopHistory[MAX_HISTORY_LENGTH]={'\0'};
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
                        //printf("%f\n",evalue);
                    }
                }
            }
        }
    }
    return evalue;
}

double vanilla_cfr(uint32_t cards[3], double p0, double p1,  char history[MAX_HISTORY_LENGTH], int pot, uint32_t traversing_player, int t, char flopHistory[MAX_HISTORY_LENGTH]){
    uint32_t plays = strlen(history);
    uint32_t acting_player = plays % 2;
    uint32_t opponent_player = 1 - acting_player;
    char currentHistory[2*MAX_HISTORY_LENGTH]={'\0'};
    strcat(currentHistory,history);
    strcat(currentHistory,flopHistory);
    //printf("cur: %s\n",currentHistory);
    if (isTerminal(currentHistory)){
        //printf("cur: %s\n",currentHistory);
        return getPayoff(cards,history,flopHistory,pot);
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
        plays = strlen(currentHistory);
        acting_player = plays % 2;
        opponent_player = 1 - acting_player;
    }
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
        infoIndex = getInfoIndex(infoset,flopInfoset,strlen(flopInfoset));
        if (flopHistory[strlen(flopHistory)-1]=='1'){
            validActions = 3;
        }
    }else{
        infoIndex = getInfoIndex(infoset,NULL,0);
        uint32_t tplays = strlen(history);
        if (tplays>0){
            if (history[tplays-1]=='1'){
                validActions = 3;
            }
        }
    }
    if (acting_player==0){
        compute_strategy(infoIndex,&tempStrategy,validActions,p0,t); 
    }
    else{
        compute_strategy(infoIndex,&tempStrategy,validActions,p1,t); 
    }
    for (uint32_t b=0; b<validActions; b++){
        sprintf(actionStr, "%hu", b);
        strcpy(tempHistory, "");
        strcat(tempHistory,history);
        strcat(tempHistory,flopHistory);

        if (flopped){
            strcpy(tempHistory, flopHistory);
            strcat(tempHistory, actionStr);
            strcpy(next_history, tempHistory);
            pot += b*4; 
            if (acting_player == 0){
                util[b] = -1*vanilla_cfr(cards, p0*tempStrategy[b], p1, history, pot, traversing_player, t, next_history);
            }
            else{
                util[b] = -1*vanilla_cfr(cards, p0, p1*tempStrategy[b], history, pot, traversing_player, t, next_history);
            }
        }
        else{
            strcpy(tempHistory, history);
            strcat(tempHistory, actionStr);
            strcpy(next_history, tempHistory);
            pot += b*2; 
            if (acting_player == 0){
                util[b] = -1*vanilla_cfr(cards, p0*tempStrategy[b], p1, next_history, pot, traversing_player, t, flopHistory);
            }
            else{  
                util[b] = -1*vanilla_cfr(cards, p0, p1*tempStrategy[b], next_history, pot, traversing_player, t, flopHistory);
            }
        }
        node_util += tempStrategy[b] * util[b];
    }
    free(tempStrategy);
    for (uint32_t c=0; c<validActions; c++){
        tempRegrets[infoIndex][c] += (util[c] - node_util) * (acting_player == 0 ? p1 : p0);
    }
    return node_util;
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
                                //printf("C:[%hu, %hu], F:[%hu]\n",f,g,h);
                                util[i] += ((double)1/120)*vanilla_cfr(cards, 1, 1, history, 2, i, t, flopHistory);
                                //printf("%f\n",util[i]);
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
    char *infoStr;
    infoStr = NULL;
    infoStr = malloc(sizeof(char) * (MAX_HISTORY_LENGTH*2+5));
    uint32_t validActions = 0;
    int temp = 0;
    for (uint32_t i=0; i<NUM_INFO; i++){
        validActions = validActInf(i);
        reverseInfoIndex(i, &infoStr);
        printf("%s: ", infoStr);
        print_avg_strategy(i);
    }
    free(infoStr);
    //Calc br etc
    double myStrat[NUM_INFO][NUM_ACTIONS] = {0};
    double *tempStrategy;
    tempStrategy = NULL;
    for (uint32_t i=0; i<NUM_INFO; i++){
        validActions = validActInf(i);
        //compute_strategy(i, &tempStrategy, validActions, -1, iterations);
        compute_avg_strategy(i,&tempStrategy);
        for (uint32_t j=0; j<NUM_ACTIONS; j++){
            myStrat[i][j] = tempStrategy[j];
        }
    }
    free(tempStrategy);

    printf("Average game value: %f\n", (util[0]/iterations));
    //save_strategy(myStrat);
    
    best_response(myStrat);

    // printf("EV: %f\n",ev(myStrat, brStrategy, 0));
    // printf("EV: %f\n",ev(brStrategy, myStrat, 0));
    printf("EV: %f\n",ev(myStrat, myStrat, 0));

    // infoStr = NULL;
    // infoStr = malloc(sizeof(char) * (MAX_HISTORY_LENGTH*2+5));
    // for (uint32_t i=0; i<NUM_INFO; i++){
    //     validActions = validActInf(i);
    //     reverseInfoIndex(i, &infoStr);
    //     printf("%s: ", infoStr);
    //     printf("[%f, %f, %f]\n", brStrategy[i][0], brStrategy[i][1], brStrategy[i][2]);
    // }
    // free(infoStr);
}

void playHand(){
    // Initialise cards
    uint32_t cards[NUM_PLAYERS+1] = {5,1,0}; //+1 for flop card
    char history[MAX_HISTORY_LENGTH]={'\0'};
    char flopHistory[MAX_HISTORY_LENGTH]={'\0'};
    char currentHistory[2*MAX_HISTORY_LENGTH]={'\0'};
    int pot = 2;
    int actions[MAX_HISTORY_LENGTH]={0,1,2,1,0,1,0};
    while(!isTerminal(currentHistory)){
        uint32_t plays = strlen(history);
        uint32_t acting_player = plays % 2;
        uint32_t opponent_player = 1 - acting_player;
        printf("%s + %s\n",history, flopHistory);
        bool flopped = (isFlop(history) || flopHistory[0]!='\0');
        char next_history[MAX_HISTORY_LENGTH+2]={'\0'};
        char tempHistory[MAX_HISTORY_LENGTH]={'\0'};
        char infoset[MAX_HISTORY_LENGTH+2]={'\0'};
        char flopInfoset[MAX_HISTORY_LENGTH+2]={'\0'};
        char actionStr[2]={'\0'};
        char cardStr[2]={'\0'};
        if (flopped){
            plays = strlen(currentHistory);
            acting_player = plays % 2;
            opponent_player = 1 - acting_player;
        }
        uint32_t validActions = 2;
        if (flopped){
            if (flopHistory[strlen(flopHistory)-1]=='1'){
                validActions = 3;
            }
        }else{
            uint32_t tplays = strlen(history); //maybe redundant variable
            if (tplays>0){
                if (history[tplays-1]=='1'){
                    validActions = 3;
                }
            }
        }
        int action = actions[plays];
        sprintf(actionStr, "%hu", action);
        printf("acting_player: %hu, acts: %d, %d\n",acting_player,validActions, action);
        strcpy(tempHistory, "");
        if (flopped){
            strcat(flopHistory, actionStr);
            pot += action*4; 
        }
        else{
            strcat(history, actionStr);
            pot += action*2; 
        }
        strcpy(currentHistory, "");
        strcat(currentHistory,history);
        strcat(currentHistory,flopHistory);
    }
    printf("%s + %s\n",history, flopHistory);
    printf("Pot: %d\n",pot);
    printf("Cards: %d, %d\n",cards[0], cards[1]);
    printf("Pay: %d\n",getPayoff(cards,history,flopHistory,pot));
}

int main() {
    // Initialise the random number generator
    srand(time(NULL));
    //playHand();

    cfr(100000); //iterations
    
    // uint32_t validActions = 0;
    // double myStrat[NUM_INFO][NUM_ACTIONS] = {0};
    // // double **my_array = (double **) malloc(NUM_INFO * sizeof(double *));
    // // load_strategy(my_array);
    // for (int i=0; i<NUM_INFO; i++){
    //     validActions = validActInf(i);
    //     printf("%d: ",i);
    //     if (validActions == 2){
    //         //printf("[%f, %f]\n",my_array[i][0],my_array[i][1]);
    //         myStrat[i][0]=0.5;
    //         myStrat[i][1]=0.5;
    //     }else{
    //         //printf("[%f, %f, %f]\n",my_array[i][0],my_array[i][1],my_array[i][2]);
    //         myStrat[i][0]=((double)1/3);
    //         myStrat[i][1]=((double)1/3);
    //         myStrat[i][2]=((double)1/3);
    //     }
    // }
    // best_response(myStrat);

    // printf("EV: %f\n",ev(myStrat, brStrategy, 0));
    // printf("EV: %f\n",ev(brStrategy, myStrat, 0));
    // for (int i=0; i<NUM_INFO; i++){
    //     validActions = validActInf(i);
    //     printf("%d: ",i);
    //     if (validActions == 2){
    //         printf("[%f, %f]\n",brStrategy[i][0],brStrategy[i][1]);
    //     }else{
    //         printf("[%f, %f, %f]\n",brStrategy[i][0],brStrategy[i][1],brStrategy[i][2]);
    //     }
    // }
    // free(my_array[0]);
    // free(my_array);

    // int temp = 0;
    // char *infoStr;
    // infoStr = NULL;
    // infoStr = malloc(sizeof(char) * (MAX_HISTORY_LENGTH*2+5));
    // char s1[] = "40";
    // char s2[] = "";
    // temp = getInfoIndex(s1,s2);
    // printf("%d\n", temp);

    // reverseInfoIndex(temp, &infoStr);
    // printf("%d -> %s\n", temp, infoStr);
    // reverseInfoIndex(609, &infoStr);
    // printf("609 -> %s\n", infoStr);
    // reverseInfoIndex(610, &infoStr);
    // printf("610 -> %s\n", infoStr);
    // reverseInfoIndex(611, &infoStr);
    // printf("611 -> %s\n", infoStr);
    // reverseInfoIndex(612, &infoStr);
    // printf("612 -> %s\n", infoStr);


    // for (int i=0;i<NUM_INFO;i++){
    //     reverseInfoIndex(i, &infoStr);
    //     //printf("%s: ", infoStr);
    //     printf("%d -> %s\n",i, infoStr);
    // }
    // free(infoStr);
    return 0;
}