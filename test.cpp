#include <iostream>
#include <ctime>
#include "time.h"
#include "BPTree.hpp"
struct tentimes
{
    char arr[30];
};
int p = 0;
tentimes sam;
int main(){
        srand(114514);
        BPTree<int, size_t> tre("bryan");
        for(int i = 1; i <= 1000; ++i){
            //if(rand() % 2){
            tre.insertData(i, i);//}
            //else
            //tre.insertData(-i, 100000 + i);
        }
        size_t *res = nullptr;
        for(int i = 1; i <= 1000; i+=2){
        /*res = tre.findU(i);
        if(!res) cout << "NOTHING !\t";
        else cout << *res <<"\t";
        if(res) delete res;
        res = nullptr;*/
        tre.modifyData(i, i + 100);
        //tre.insertData(i, i);
        res = tre.findU(i);
        //res = tre.findU(i);
         if(!res) cerr <<i<<" : "<<"NOTHING !\t";
        else{
             cerr << i<<" : "<< *res <<"\t";
             delete res;
         }
        }
        cout << "\n";
        //tre.dfs();
  }
