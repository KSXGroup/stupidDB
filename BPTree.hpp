#ifndef _BPTREE_HPP_
#define _BPTREE_HPP_
#include <fstream>
#include <iostream>
#include <cstring>
#include <queue>
#include "dbException.hpp"
#define IOB std::ios_base::in | std::ios_base::out | std::ios_base::binary
#define TIOB std::ios_base::trunc | std::ios_base::in | std::ios_base::out | std::ios_base::binary
const size_t MAX_FILENAME_LEN = 30;
const size_t MAX_BLOCK_SIZE = 500;
const size_t FIRST_NODE_OFFSET = MAX_FILENAME_LEN * sizeof(char) * 2 + 2 * sizeof(size_t);
const int INVALID_OFFSET = -1;
const int INTERN_NODE = 1;
const int LEAF_NODE = 2;
const char DB_SUFFIX[10] = ".ksxdb";
const char IDX_SUFFIX[10] = ".ksxidx";
const char IDX_MGR_SUFFIX[10] = ".idxmgr";
const char DB_MGR_SUFFIX[10] = ".dbmgr";

template<typename Key, typename Compare>
class BPTree;
struct dBDataType;

template<typename Key, typename T, typename Compare = std::less<Key> >
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
        size_t nodeOffset = 0;
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
    char idxFileMgr[MAX_FILENAME_LEN];
    char dbFileName[MAX_FILENAME_LEN];
    char dbFileMgr[MAX_FILENAME_LEN];
    size_t dataSize = 0;
    size_t rootOffset = 0;
    std::fstream fidx;
    std::fstream fdb;
    std::fstream fmgr;
    std::queue<size_t> QidxMgr;
    std::queue<size_t> QdbMgr;
    BPTNode* currentNode = nullptr;

//*******************file IO****************************//

    //Dont forget to DELETE afte using allocNewNode()!
    BPTNode *allocNode(const int &nodeType){
        if(fmgr.is_open()) fmgr.close();
        BPTNode *tmp = new BPTNode(nodeType);
        size_t offset = 0;
        fmgr.open(idxFileMgr, IOB);
        if(!QidxMgr.empty()){
            offset = QidxMgr.front();
            QidxMgr.pop();
            writeNode(tmp, tmp->nodeOffset);
            return tmp;
        }
        else{
            std::cerr << "write to end \n";
            writeNode(tmp);
            return tmp;
        }
        delete tmp;
        tmp = nullptr;
        return tmp;
    }

    bool deleteNode(size_t offset){
        QidxMgr.push(offset);
    }

    bool writeIdx(){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        fidx.write(idxFileName, sizeof(char) * MAX_FILENAME_LEN);
        fidx.write(dbFileName, sizeof(char) * MAX_FILENAME_LEN);
        fidx.write((const char*)&dataSize, sizeof(size_t));
        fidx.write((const char*)&rootOffset, sizeof(size_t));
        fidx.close();
        return 1;
    }

    bool readIdx(){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        char tmp[MAX_FILENAME_LEN];
        size_t offset = 0;
        fidx.read(tmp, sizeof(char) * MAX_FILENAME_LEN);
        if(strcmp(idxFileName, tmp) != 0) throw fileNotMatch();
        strcpy(idxFileName, tmp);
        fidx.read(dbFileName, sizeof(char) * MAX_FILENAME_LEN);
        fidx.read((char*)&dataSize, sizeof(size_t));
        fidx.read((char*)&rootOffset, sizeof(size_t));
        fidx.close();

        //I will read mgr file into Q here
        fmgr.open(idxFileMgr, IOB);
        while(!fmgr.eof()){
            fmgr.read((char*)&offset, sizeof(size_t));
            QidxMgr.push(offset);
        }
        fmgr.close();
        fmgr.open(dbFileMgr, IOB);
        while(!fmgr.eof()){
            fmgr.read((char*)&offset, sizeof(size_t));
            QdbMgr.push(offset);
        }
        fmgr.close();
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
    bool writeNode(BPTNode *p, size_t offset = 0){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        if(offset <= 0){
            fidx.seekg(0, std::ios_base::end);
            offset = fidx.tellg() + 1;
        }
        p->nodeOffset = offset;
        if(!fidx.is_open()) return 0;
        fidx.seekp(offset);
        fidx.write((const char*)&(p->nodeType), sizeof(int));
        fidx.write((const char*)&(p->sz), sizeof(size_t));
        fidx.write((const char*)&(p->nodeOffset), sizeof(size_t));
        fidx.write((const char*)p->data, sizeof(p->data));
        fidx.close();
        return 1;
    }

    bool importIdxFile(const size_t dl){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        if(!fidx){
            if(dl == 0) throw ImportFileNotExist();
            if(currentNode) delete currentNode;
            fidx.clear();
            currentNode = allocNode(LEAF_NODE); //written to end when alloc
            //create new file if index not exist
            fidx.open(idxFileName, std::ios_base::out);
            fidx.close();
            fdb.open(dbFileName, std::ios_base::out);
            fdb.close();
            fmgr.open(idxFileMgr, std::ios_base::out);
            fmgr.close();
            fmgr.open(dbFileMgr, std::ios_base::out);
            fmgr.close();
            rootOffset = currentNode->nodeOffset;
            writeIdx();
            return 1;
        }
        else{
            readIdx();
            if(currentNode) delete currentNode;
            currentNode = readNode(rootOffset);
            return 1;
        }
    }

    //bool openDbFile(){}
    //bool closeDbFile(){}

    //Node merge, Node split
    void mergeNode(){}
    void splitNode(){}    

    //private find
    BPTNode *treeFind(const Key &k){
        int cmpres = 0, cmpresLast = 0;
        if(currentNode){
            writeNode(currentNode, currentNode->nodeOffset);
            delete currentNode;
            currentNode = nullptr;
        }
        currentNode = readNode(rootOffset);
        while(currentNode->nodeType != LEAF_NODE){
            cmpresLast = keyCompare(k,currentNode->data[0]);
            if(cmpresLast == 1 || cmpresLast == 2){
                BPTNode *tmp = readNode(currentNode->data[0].data);
                delete currentNode;
                currentNode = tmp;
                continue;
            }
            for(size_t i = 1; i < currentNode->sz; ++i){
                cmpres = keyCompare(k, currentNode->data[i].k);
                if(cmpresLast == 0 && (cmpres == 1 || cmpres == 2)){
                    BPTNode *tmp = readNode(currentNode->data[i].data);
                    delete currentNode;
                    currentNode = tmp;
                    break;
                }
                else cmpresLast = cmpres;
            }
        }
        for(int i = 0; i < currentNode->sz; ++i){
            cmpres = keyCompare(k, currentNode->data[i]);
            if(cmpres == 2){
                //readData(); // TODO READ DATA
                //return read data
            }
        }

        //return null

    }

    BPTNode *treeInsert(const Key &k, void *dataPtr){
        //----------------- copy from treeFind() -----------------------//
        int cmpres = 0, cmpresLast = 0;
        if(currentNode){
            writeNode(currentNode, currentNode->nodeOffset);
            delete currentNode;
            currentNode = nullptr;
        }
        currentNode = readNode(rootOffset);
        while(currentNode->nodeType != LEAF_NODE){
            cmpresLast = keyCompare(k,currentNode->data[0]);
            if(cmpresLast == 1 || cmpresLast == 2){
                BPTNode *tmp = readNode(currentNode->data[0].data);
                delete currentNode;
                currentNode = tmp;
                continue;
            }
            for(size_t i = 1; i < currentNode->sz; ++i){
                cmpres = keyCompare(k, currentNode->data[i].k);
                if(cmpresLast == 0 && (cmpres == 1 || cmpres == 2)){
                    BPTNode *tmp = readNode(currentNode->data[i].data);
                    delete currentNode;
                    currentNode = tmp;
                    break;
                }
                else cmpresLast = cmpres;
            }
        }
        //----------------- copy from treeFind() -----------------------//
        //for(size_t i = 0; i < currentNode -> sz; ++i){}......
    }


public:
    BPTree(const char* s){
        memset(idxFileName, 0,sizeof(idxFileName));
        for(size_t i = 0; i <= strlen(s); ++i) idxFileName[i] = s[i];
        for(size_t i = 0; i <= strlen(s); ++i) dbFileName[i] = s[i];
        for(size_t i = 0; i <= strlen(s); ++i) idxFileMgr[i] = s[i];
        for(size_t i = 0; i <= strlen(s); ++i) dbFileMgr[i] = s[i];
        strcat(idxFileName, IDX_SUFFIX);
        strcat(dbFileName, DB_SUFFIX);
        strcat(idxFileMgr, IDX_MGR_SUFFIX);
        strcat(dbFileMgr, DB_MGR_SUFFIX);
        importIdxFile(sizeof(T));
    }

    ~BPTree(){
        if(currentNode) delete currentNode;
        currentNode = nullptr;
        size_t offset = 0;

        //Dump Q into files
        fmgr.open(idxFileMgr, TIOB);
        while(!QidxMgr.empty()){
            offset = QidxMgr.front();
            fmgr.write((char*)&offset, sizeof(size_t));
            QidxMgr.pop();
        }
        fmgr.close();

        fmgr.open(dbFileMgr, TIOB);
        while(!QdbMgr.empty()){
            offset = QdbMgr.front();
            fmgr.write((char*)&offset, sizeof(size_t));
            QdbMgr.pop();
        }
        fmgr.close();
        writeIdx();
    }

    //Insert, Remove, Find
    BPTNode *insertData(){}
    bool removeData(){}

};

#endif
