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
        for(int i = 0; i < 1000000; ++i){
            if(rand() % 2){
            tre.insertData(i, 100000 + i);}
            else tre.insertData(i, 100000 - i);
        }
        size_t *res = nullptr;
        //res = tre.find(1);
        if(!res) cerr << "NOTHING !\n";
        else cerr << *res <<"\n";
}
