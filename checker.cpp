#include <iostream>
#include <fstream>
#include <cstring>
#include "BPTree.hpp"
using namespace std;
int main(){
    struct node{
        int a;
        size_t b;
    };
    fstream fidx;
    int type = 0;
    size_t next = 0,prev = 0, size = 0, offset = 0, posd = 0, posn = 0;
    int cnter = 0;
    char s[20], tmp[MAX_FILENAME_LEN], dbFileName[MAX_FILENAME_LEN];
    node tmpdata[MAX_BLOCK_SIZE];
    size_t dataSize, rootOffset;
    cin >> s;
    strcat(s, IDX_SUFFIX);
    fidx.open(s, IOB);
    fidx.seekg(0, ios_base::end);
    cout << "tellg() " << fidx.tellg() << "\n";
    fidx.seekg(0);
    fidx.read(tmp, sizeof(char) * MAX_FILENAME_LEN);
    fidx.read(dbFileName, sizeof(char) * MAX_FILENAME_LEN);
    fidx.read((char*)&dataSize, sizeof(size_t));
    fidx.read((char*)&rootOffset, sizeof(size_t));
    cout << "FILENAME : " << tmp << "\n";
    cout << "DB_FILENAME : " << dbFileName << "\n";
    cout << "DATA_SIZE : " << dataSize << "\n";
    cout << "OFFSET OF ROOT NODE : " << rootOffset << "\n";
    posn = 0;
    while(fidx.tellg() != -1){
        cout << "\n";
        cout << "NODE #" << ++posn << "\n";
        cout << "FSTREAM START AT tellg() = " << fidx.tellg() << "\n";
        fidx.read((char*)&(type), sizeof(int));
        fidx.read((char*)&(next), sizeof(size_t));
        fidx.read((char*)&(prev), sizeof(size_t));
        fidx.read((char*)&(size), sizeof(size_t));
        fidx.read((char*)&(offset), sizeof(size_t));
        fidx.read((char*)tmpdata, sizeof(tmpdata));
        if(type == DELETED) continue;
        cout << "NODE TYPE : " << ((type == 1) ? "INTERN NODE\n" : ((type == 2) ? "LEAF NODE\n" : (type == 3 ? "DELETED\n" : "INVALIED\n")));
        cout << "PREV NODE OFFSET : " << prev << "\n";
        cout << "NEXT NODE OFFSET : " << next << "\n";
        cout << "NODE SIZE : " << size << "\n";
        cout << "SELF OFFSET : " << offset << "\n";
        cout << "LIST DATA : \n";
        for(int i = 0; i < size; ++i){
            cout << "NODEDATA#" << i << " : " << " KEY : " << tmpdata[i].a << "   DATA : " << tmpdata[i].b << "\n";
        }
        cout << "NOW tellg()" << fidx.tellg() << "\n";
        cout << "\n";
        type = 0;
        next = 0;
        size = 0;
        offset = 0;
    }
    cout << "\n\n\n\n\n";
    cout << "DUMP " << dbFileName << " TO SCREEN : \n";
    fidx.close();
    fidx.open(dbFileName, IOB);
    posd = 0;
     while(fidx.tellg()!=-1){
        cout << "DBDATA #" << posd++ << " : ";
        size_t y = 0;
        //cout << fidx.tellg() << " : ";
        fidx.read((char*)&y, sizeof(size_t));
        cout << y << "\t\t\t";
        cnter++;
        if(cnter == 4){
            cout << "\n";
            cnter = 0;
        }
    }
    cout << "\n";
}

