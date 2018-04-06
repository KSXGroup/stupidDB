#ifndef _BPTREE_HPP_
#define _BPTREE_HPP_
#include <fstream>
#include <iostream>
#include <cstring>
#include "dbException.hpp"
#define IOB std::ios_base::in | std::ios_base::out | std::ios_base::binary
const size_t MAX_FILENAME_LEN = 30;
const size_t MAX_BLOCK_SIZE = 500;
const int INVALID_OFFSET = -1;
const int INTERN_NODE = 1;
const int LEAF_NODE = 2;
const int FIRST_NODE_OFFSET = MAX_FILENAME_LEN * sizeof(char) * 2 + 2 * sizeof(size_t);
const char DB_SUFFIX[10] = ".ksxdb";
const char IDX_SUFFIX[10] = ".ksxidx";

template<typename Key, typename Compare>
class BPTree;
struct dBDataType;

template<typename Key, typename Compare = std::less<Key> >
class BPTree{
private:
    struct BPTNode{
        struct treeData{
            Key k = Key();
            size_t data = 0;
        };

        BPTNode(const int &ndt):nodeType(ndt){}
        BPTNode(const BPTNode &other){
            nodeType = other.nodeType;
            sz = other.sz;
            for(int i = 0; i < sz; ++i) data[i] = other.data[i];
        }

        //DATA
        int nodeType = INTERN_NODE;
        size_t sz = 0;
        treeData data[MAX_BLOCK_SIZE];
    };

private:
    int keyCompare(const Key &a, const Key &b){
        Compare cmp;
        if(cmp(a, b)) return 1; // less
        if(cmp(b, a)) return 0; // more
        return 2; //equal;
    }

private:
    char idxFileName[MAX_FILENAME_LEN];
    char dbFileName[MAX_FILENAME_LEN];
    size_t dataLen = 0;
    size_t dataSize = 0;
    std::fstream fidx;
    std::fstream fdb;
    BPTNode* currentNode = nullptr;

    //file IO
    bool writeIdx(){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        fidx.write(idxFileName, sizeof(char) * MAX_FILENAME_LEN);
        fidx.write(dbFileName, sizeof(char) * MAX_FILENAME_LEN);
        fidx.write((const char*)&dataLen, sizeof(size_t));
        fidx.write((const char*)&dataSize, sizeof(size_t));
        fidx.close();
        return 1;
    }

    bool readIdx(){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        char tmp[MAX_FILENAME_LEN];
        fidx.read(tmp, sizeof(char) * MAX_FILENAME_LEN);
        if(strcmp(idxFileName, tmp) != 0) throw fileNotMatch();
        strcpy(idxFileName, tmp);
        fidx.read(dbFileName, sizeof(char) * MAX_FILENAME_LEN);
        fidx.read((char*)&dataLen, sizeof(size_t));
        fidx.read((char*)&dataSize, sizeof(size_t));
        fidx.close();
        return 1;
    }

    //dont forget to delete after use readNode()!
    BPTNode *readNode(size_t offset){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        if(!fidx.is_open()) return nullptr;
        BPTNode *tmp = new BPTNode(INTERN_NODE);
        fidx.seekg(offset);
        fidx.read((char*)&(tmp->nodeType), sizeof(int));
        fidx.read((char*)&(tmp->sz), sizeof(size_t));
        fidx.read((char*)tmp->data, sizeof(tmp->data));
        fidx.close();
        return tmp;
    }
    bool writeNode(size_t offset, const BPTNode *p){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        if(!fidx.is_open()) return 0;
        fidx.seekp(offset);
        fidx.write((const char*)&(p->nodeType), sizeof(int));
        fidx.write((const char*)&(p->sz), sizeof(size_t));
        fidx.write((const char*)p->data, sizeof(p->data));
        fidx.close();
        return 1;
    }

    bool importIdxFile(const size_t dl){
        if(fidx.is_open()) return 0;
        fidx.open(idxFileName, IOB);
        if(!fidx){
            if(dl == 0) throw ImportFileNotExist();
            if(currentNode) delete currentNode;
            currentNode = new BPTNode(LEAF_NODE);
            //create new file
            fidx.open(idxFileName, std::ios_base::out);
            fidx.close();
            fdb.open(dbFileName, std::ios_base::out);
            fdb.close();
            writeIdx();
            writeNode(FIRST_NODE_OFFSET, currentNode);
            return 1;
        }
        else{
            readIdx();
            currentNode = readNode(FIRST_NODE_OFFSET);
            return 1;
        }
    }

    bool openDbFile(){}
    bool closeDbFile(){}

    //Node merge, Node split
    void mergeNode(){}
    void splitNode(){}    


public:
    BPTree(const char* s, size_t dl = 0): dataLen(dl){
        memset(idxFileName, 0,sizeof(idxFileName));
        for(size_t i = 0; i <= strlen(s); ++i) idxFileName[i] = s[i];
        for(size_t i = 0; i <= strlen(s); ++i) dbFileName[i] = s[i];
        strcat(idxFileName, IDX_SUFFIX);
        strcat(dbFileName, DB_SUFFIX);
        importIdxFile(dataLen);
    }

    ~BPTree(){
        if(currentNode) delete currentNode;
    }

    //Insert, Remove, Find
    BPTNode *insertData(){}
    bool removeData(){}

};

#endif
