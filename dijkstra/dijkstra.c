// 61418816 山内脩吾
#include <stdio.h>
#define NNODE 6
#define INF 100
int cost[NNODE][NNODE] = {
  {0,2,5,1,INF,INF},
  {2,0,3,2,INF,INF},
  {5,3,0,3,1,5},
  {1,2,3,0,1,INF},
  {INF,INF,1,1,0,2},
  {INF,INF,5,INF,2,0}
};
int dist[NNODE];
int prev[NNODE];
int check[NNODE];
int checknum;
void calc_costs(int n);
void output();
void calc_costs(int n){
  int i;
  for (i = 0; i < NNODE; i++) check[i] = 0;
  check[n] = 1;
  for (i = 0; i < NNODE; i++) {
    if(cost[n][i] != INF) {
      dist[i] = cost[n][i];
      prev[i] = 0;
    } else dist[i] = INF;
  }
  do{
    int k;
    for (i = 0; i < NNODE; i++){
      int min;
      min = INF;
      if(min > dist[i] && check[i] == 0) {
	k = i;
      }
    }
    check[k] = 1;
    for (i = 0; i < NNODE; i++){
      if(check[i] == 0 && cost[k][i] != INF) {
	if (dist[i] > dist[k] + cost[k][i]){
	  dist[i] = dist[k] + cost[k][i];
	  prev[i] = k;
	}
      }
    }
    checknum = 0;
    for (i = 0; i < NNODE;i++) if(check[i] == 1) checknum++;
  }while(checknum < NNODE);
    output(n);
}
void output(int n){
  char c = 'A';
  printf("root node %c:\n", c+n);
  int i;
  for (i= 0; i < NNODE; i++){
     printf("[%c,%c,%d]", c + i, c + prev[i], dist[i]);
  }
  printf("\n");
}



int main(int argc, char *argv[])
{
  int i;
  for (i = 0; i < NNODE; i++){
    calc_costs(i);
  }
  return 0;
}
