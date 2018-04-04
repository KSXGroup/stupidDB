#ifndef _BPTREE_HPP_
#define _BPTREE_HPP_
#include <fstream>
#include <iostream>
#include <cstring>
#include "dbException.hpp"

const size_t MAX_FILENAME_LEN = 30;
const int INVALID_OFFSET = -1;
const char DB_SUFFIX[10] = ".ksxdb";
const char IDX_SUFFIX[10] = ".ksxidx";

template<typename Key, typename Compare>
class BPTree;
struct dBDataType;

template<typename Key, typename Compare = std::less<Key>>
class BPTree{
private:
    struct BPTLeaf{};
    struct BPTNode{};

private:
    char fileName[MAX_FILENAME_LEN];
    std::fstream fidx;
    std::fstream fdb;
    BPTNode currentNode;

    //file IO
    bool importIdxFile(){
        fidx.open(fileName, std::ios_base::in | std::ios_base::out | std::ios_base::binary);
        if(!fidx){
            fidx.open(fileName, std::ios_base::out);
            fidx.write(fileName, MAX_FILENAME_LEN);
            fidx.close();
        }
    }
    bool closeIdxFile(){}
    bool openDbFile(){}
    bool closeDbFile(){}
    BPTNode *readNode(){}
    bool writeNode(){}
    BPTLeaf *readLeaf(){}
    bool writeLeaf(){}

    //Node merge, Node split
    void mergeNode(){}
    void splitNode(){}    


public:
    BPTree(const char* s){
        memset(fileName, 0,sizeof(fileName));
        for(size_t i = 0; i <= strlen(s); ++i) fileName[i] = s[i];
        strcat(fileName, IDX_SUFFIX);
        importIdxFile();
    }

    //Insert, Remove, Find
    BPTNode *insertData(){}
    bool removeData(){}

};

#endif
