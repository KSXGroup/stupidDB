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
    size_t next = 0, size = 0, offset = 0;
    char s[20], tmp[MAX_FILENAME_LEN], dbFileName[MAX_FILENAME_LEN];
    node tmpdata[MAX_BLOCK_SIZE];
    size_t dataSize, rootOffset;
    cin >> s;
    strcat(s, IDX_SUFFIX);
    fidx.open(s, IOB);
    fidx.seekg(0, ios_base::end);
    cerr << "tellp() " << fidx.tellp() << "\n";
    fidx.seekg(0);
    fidx.read(tmp, sizeof(char) * MAX_FILENAME_LEN);
    fidx.read(dbFileName, sizeof(char) * MAX_FILENAME_LEN);
    fidx.read((char*)&dataSize, sizeof(size_t));
    fidx.read((char*)&rootOffset, sizeof(size_t));
    cerr << "FILENAME : " << tmp << "\n";
    cerr << "DB_FILENAME : " << dbFileName << "\n";
    cerr << "DATA_SIZE : " << dataSize << "\n";
    cerr << "OFFSET OF ROOT NODE : " << rootOffset << "\n";
    for(int i = 1; i <= 100; ++i){
        cerr << "\n";
        cerr << "NODE #" << i << "\n";
        cerr << "FSTREAM START AT tellg() = " << fidx.tellg() << "\n";
        fidx.read((char*)&(type), sizeof(int));
        fidx.read((char*)&(next), sizeof(size_t));
        fidx.read((char*)&(size), sizeof(size_t));
        fidx.read((char*)&(offset), sizeof(size_t));
        fidx.read((char*)tmpdata, sizeof(tmpdata));
        cerr << "NODE TYPE : " << ((type == 1) ? "INTERN NODE\n" : ((type == 2) ? "LEAF NODE\n" : "INVALID\n"));
        cerr << "NEXT NODE OFFSET : " << next << "\n";
        cerr << "NODE SIZE : " << size << "\n";
        cerr << "SELF OFFSET : " << offset << "\n";
        cerr << "\n";
        type = 0;
        next = 0;
        size = 0;
        offset = 0;
    }
    cerr << "\n\n\n\n\n";
    cerr << "DUMP " << dbFileName << " TO SCREEN : \n";
    fidx.close();
    fidx.open(dbFileName, IOB);
    for(int i = 1; i <= 150; ++i){
        cerr << "DATA #" << i << " : ";
        size_t y = 0;
        fidx.read((char*)&y, sizeof(size_t));
        cerr << y << "\t";
    }
}

