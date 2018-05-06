#include <iostream>
#include <ctime>
#include "time.h"
#include "BPTree.hpp"
struct tentimes
{
    int arr[20];
};
int p = 0;
int main(){
        srand(114514);
        BPTree<int, size_t> tre("bryan");
        for(int i = 1; i <= 10; ++i){
            //if(rand() % 2){
            tre.insertData(i, 100 + i);//}
            //else
            //tre.insertData(-i, 100000 + i);
        }
        //size_t *res = nullptr;
        for(int i = 2; i <= 10; i+=2){
        /*res = tre.findU(i);
        if(!res) cout << "NOTHING !\t";
        else cout << *res <<"\t";
        if(res) delete res;
        res = nullptr;*/
        tre.removeData(i);
        /*res = tre.findU(i);
         if(!res) cout << "NOTHING !\t";
         else cout << *res <<"\t";*/
        }
        tre.dfs();
        cout << "\n";
  }
