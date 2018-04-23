#include <iostream>
#include "BPTree.hpp"
int main(){
        BPTree<int, size_t> tre("bryan");
        for(int i = 0; i < 20; ++i){
            tre.insertData(i, 100000 + i);
        }
        size_t *res = nullptr;
        //res = tre.find(1);
        if(!res) cerr << "NOTHING !\n";
        else cerr << *res <<"\n";
}
