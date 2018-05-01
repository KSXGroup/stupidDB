#include <iostream>
#include <ctime>
#include "time.h"
#include "BPTree.hpp"
struct tentimes
{
    int arr[20];
};
int main(){
        srand(time(0));
        BPTree<int, size_t> tre("bryan");
        for(int i = 500; i >= 1; --i){
            //if(rand() % 2){
            tre.insertData(i, rand() + i);//}
            //else
            //tre.insertData(-i, 100000 + i);
        }
        size_t *res = nullptr;
        for(int i = 1; i <= 500; ++i){
        res = tre.findU(i);
        if(!res) cerr << "NOTHING !\n";
        else cerr << *res <<"\n";
        if(res) delete res;
        res = nullptr;
        }
}
