#ifndef _BPTREE_HPP_
#define _BPTREE_HPP_
#include <fstream>
#include <iostream>
#include <cstring>

const size_t MAX_FILENAME_LEN = 30;
const char DB_SUFFIX[10] = ".ksxdb";
const char IDX_SUFFIX[10] = ".ksxidx";

template<typename Key, typename Compare>
class BPTree;
class BPTLeaf;
class BPTNode;
struct dBDataType;

template<typename Key, typename Compare = std::less<Key>>
class BPTree{
private:
    char fileName[MAX_FILENAME_LEN];
    std::fstream fidx;
    std::fstream fdb;
    //BPTNode currentNode;

    //file IO
    std::fstream& openIdxFile(){}
    bool closeIdxFile(){}
    std::fstream& openDbFile(){}
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
        for(size_t i = 0; i < strlen(s); ++i) fileName[i] = s[i];
        strcat(fileName, IDX_SUFFIX);
        std::cout << fileName ;
    }

    //Insert, Remove, Find
    BPTNode* insertData(){}
    bool removeData(){}

};



#endif
