#ifndef _BPTREE_HPP_
#define _BPTREE_HPP_
#include <fstream>
#include <iostream>
#include <cstring>
#include <queue>
#include "dbException.hpp"
#define IOB std::ios_base::in | std::ios_base::out | std::ios_base::binary
#define TIOB std::ios_base::trunc | std::ios_base::in | std::ios_base::out | std::ios_base::binary
//file io
const size_t MAX_FILENAME_LEN = 30;
const size_t MAX_BLOCK_SIZE = 20 ;
const size_t FIRST_NODE_OFFSET = MAX_FILENAME_LEN * sizeof(char) * 2 + 2 * sizeof(size_t);
const int INVALID_OFFSET = 0;
//node type
const int INTERN_NODE = 1;
const int LEAF_NODE = 2;
//ret type
const int INVALID = 110;
const int NOTHING = 111;
const int SPLITED = 112;
const int BORROWED = 113;
const int MERGED = 114;
//file suffix
const char DB_SUFFIX[10] = ".ksxdb";
const char IDX_SUFFIX[10] = ".ksxidx";
const char IDX_MGR_SUFFIX[10] = ".idxmgr";
const char DB_MGR_SUFFIX[10] = ".dbmgr";

template<typename Key, typename T, typename Compare>
class BPTree;

template<typename Key, typename T, typename Compare = std::less<Key> >
class BPTree{
private:

    struct treeData{
        Key k = Key();
        size_t data = 0;
    };

    struct retVal{
        treeData retDta;
        int status = INVALID;
        retVal() = default;
        retVal(const Key &_k, const size_t &_d, const int &_status):status(_status){
            retDta.data = _d;
            retDta.k = _k;
        }
        retVal(const treeData &td, const int &_status): retDta(td), status(_status){}
    };


    struct BPTNode{

        BPTNode(const int &ndt):nodeType(ndt){}
        BPTNode(const BPTNode &other){
            nodeType = other.nodeType;
            sz = other.sz;
            for(int i = 0; i < sz; ++i) data[i] = other.data[i];
        }

        //DATA
        int nodeType = INTERN_NODE;
        size_t sz = 0;
        size_t nodeOffset = INVALID_OFFSET;
        size_t nextNode = INVALID_OFFSET;
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
            tmp->nodeOffset = offset;
            writeNode(tmp, tmp->nodeOffset);
            return tmp;
        }
        else{
            std::cerr << "write to end \n";
            writeNode(tmp);
            cerr << tmp->nodeOffset << "\n";
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
        size_t fsize = 0;
        fmgr.open(idxFileMgr, IOB);
        fmgr.seekg(0, std::ios_base::end);
        fsize = fmgr.tellg();
        fmgr.seekg(0);
        if(fsize != 0){
            while(fmgr.tellg() != fsize){
                fmgr.read((char*)&offset, sizeof(size_t));
                QidxMgr.push(offset);
            }
            fmgr.close();
        }
        fmgr.open(dbFileMgr, IOB);
        fmgr.seekg(0, std::ios_base::end);
        fsize = fmgr.tellg();
        fmgr.seekg(0);
        if(fsize != 0){
            while(fmgr.tellg() != fsize){
                fmgr.read((char*)&offset, sizeof(size_t));
                QdbMgr.push(offset);
            }
            fmgr.close();
        }
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
        //cerr <<"read Node "<< tmp->nodeType << "\n";
        fidx.read((char*)&(tmp->nextNode), sizeof(size_t));
        fidx.read((char*)&(tmp->sz), sizeof(size_t));
        fidx.read((char*)&(tmp->nodeOffset), sizeof(size_t));
        fidx.read((char*)tmp->data, sizeof(tmp->data));
        fidx.close();
        return tmp;
    }
    bool writeNode(BPTNode *p, size_t offset = 0){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        if(offset == 0){
            fidx.seekg(0, std::ios_base::end);
            cerr << " tellg " << fidx.tellg() <<"\n";
            offset = fidx.tellg();
        }
        p->nodeOffset = offset;
        if(!fidx.is_open()) return 0;
        fidx.seekp(offset);
        fidx.write((const char*)&(p->nodeType), sizeof(int));
        fidx.write((const char*)&(p->nextNode), sizeof(size_t));
        fidx.write((const char*)&(p->sz), sizeof(size_t));
        fidx.write((const char*)&(p->nodeOffset), sizeof(size_t));
        fidx.write((const char*)p->data, sizeof(p->data));
        fidx.close();
        return 1;
    }

    inline void changeToRoot(){
        if(currentNode){
            writeNode(currentNode, currentNode->nodeOffset);
            delete currentNode;
            currentNode = nullptr;
        }
        currentNode = readNode(rootOffset);
        cerr << "CHANGE TO ROOT, ROOT OFFSET IS " << rootOffset << "\n";
    }

   T *readData(size_t offset){
        T *tmp = nullptr;
        fdb.open(dbFileName, IOB);
        fdb.read((char*)tmp, sizeof(T));
        return tmp;
   }

   size_t writeData(const T *dataPtr){
       if(fdb.is_open()) fdb.close();
       size_t offset = 0;
       fdb.open(dbFileName, IOB);
       if(!fdb) return 0;
       if(!QdbMgr.empty()){
            offset = QdbMgr.front();
            QdbMgr.pop();
       }
       else{
           fdb.seekg(0, std::ios_base::end);
           offset = fdb.tellg();
       }
       fdb.write((const char*)dataPtr, sizeof(T));
       return offset;
   }

   size_t deleteData(size_t offset){
       QdbMgr.push(offset);
   }

    bool importIdxFile(const size_t dl){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        if(!fidx){
            if(dl == 0) throw ImportFileNotExist();
            if(currentNode) delete currentNode;
            fidx.clear();
            //create new file if index not exist
            fidx.open(idxFileName, std::ios_base::out);
            fidx.close();
            fdb.open(dbFileName, std::ios_base::out);
            fdb.close();
            fmgr.open(idxFileMgr, std::ios_base::out);
            fmgr.close();
            fmgr.open(dbFileMgr, std::ios_base::out);
            fmgr.close();
            writeIdx();
            currentNode = allocNode(LEAF_NODE); //written to end when alloc
            rootOffset = currentNode->nodeOffset;
            cerr << "create " << currentNode->nodeType << " with offset " << rootOffset << "\n";
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

    //Node merge, Node split
    bool mergeAble(BPTNode *p){}
    bool borrowAble(BPTNode *p){}
    bool splitAble(BPTNode *p){
        if(p->sz == MAX_BLOCK_SIZE) return 1;
        else return 0;
    }
    //merge to left
    void mergeNode(BPTNode *l){
    }
    //split
    treeData splitNode(BPTNode *p){
        BPTNode *ntmp = allocNode(p->nodeType);
        cerr << "0# NODE SPLITED AND NEW NODE OFFSET IS : "  << ntmp->nodeOffset << " \n";
        ntmp->nextNode = p->nextNode;
        p->nextNode = ntmp->nodeOffset;
        for(size_t i = (MAX_BLOCK_SIZE >> 1) ; i < MAX_BLOCK_SIZE; ++i) ntmp->data[i - (MAX_BLOCK_SIZE >> 1)] = p->data[i];
        p->sz = (MAX_BLOCK_SIZE >> 1) ;
        ntmp->sz = MAX_BLOCK_SIZE - (MAX_BLOCK_SIZE >> 1);
        cerr << "1# NODE SPLITED AND NEW NODE OFFSET IS : "  << ntmp->nodeOffset << " \n";
        writeNode(p, p->nodeOffset);
        writeNode(ntmp, ntmp->nodeOffset);
        if(p->nodeOffset == rootOffset){
            BPTNode *tmpRoot = allocNode(INTERN_NODE);
            cerr << "1# ROOT NODE SPLITED AND NEW NODE OFFSET IS : "  << tmpRoot->nodeOffset << " \n";
            tmpRoot->sz = 2;
            tmpRoot->data[0].k = p->data[0].k;
            tmpRoot->data[0].data = p->nodeOffset;
            tmpRoot->data[1].k = ntmp->data[0].k;
            tmpRoot->data[1].data = ntmp->nodeOffset;
            writeNode(tmpRoot, tmpRoot->nodeOffset);
            rootOffset = tmpRoot->nodeOffset;
            writeIdx();
            delete tmpRoot;
            tmpRoot = nullptr;
        }
        treeData rtmp = ntmp->data[0];
        rtmp.data = ntmp->nodeOffset;
        delete ntmp;
        ntmp = nullptr;
        return rtmp;
    }

    //private find
   treeData treeFind(const Key &k){
        int cmpres = 0;
        changeToRoot();
        if(currentNode->sz == 0) return treeData();
        cmpres = keyCompare(k,currentNode->data[0].k);
        if(cmpres == 1) return treeData(); //if less than first element, return

        while(currentNode->nodeType != LEAF_NODE){
            cmpres = keyCompare(k,currentNode->data[0].k);
            if(cmpres == 2){
                BPTNode *tmp = readNode(currentNode->data[0].data);
                delete currentNode;
                currentNode = tmp;
                tmp = nullptr;
                continue;
            }
            for(size_t i = currentNode->sz - 1; i >= 0 && i <= currentNode->sz; --i){
                if(i >= currentNode->sz) break;
                cmpres = keyCompare(k, currentNode->data[i].k);
                if(cmpres == 2 || cmpres == 0){
                    BPTNode *tmp = currentNode;
                    currentNode = readNode(currentNode->data[i].data);
                    delete tmp;
                    tmp =nullptr;
                    break;
                }
            }
        }
        for(int i = 0; i < currentNode->sz; ++i){
            cmpres = keyCompare(k, currentNode->data[i].k);
            if(cmpres == 2) return treeData(currentNode->data[i]);
        }
        return treeData();
    }

   retVal treeInsert(const Key &k, const T &dta, BPTNode *st){
       int cmpres = 0;
       if(st->nodeType == LEAF_NODE){
           if(st->sz == 0){
               st->data[0].k = k;
               st->data[0].data = writeData(&dta);
               st->sz++;
               writeNode(st, st->nodeOffset);
               retVal itmp = retVal(st->data[0], NOTHING);
               delete st;
               st = nullptr;
               return itmp;
           }
           for(size_t i = st->sz - 1; i >= 0 && i <= st->sz; --i){
               cmpres = keyCompare(k, st->data[i].k);
               if(cmpres == 2) return retVal(Key(), 0, INVALID);
               else if(cmpres == 0){
                   for(size_t j = st->sz - 1; j >= i + 1 && j <= st->sz; --j) st->data[j + 1] = st->data[j];
                   treeData ins;
                   ins.data = writeData(&dta);
                   ins.k = k;
                   st->data[i + 1] = ins;
                   st->sz++;
                   writeNode(st, st->nodeOffset);
                   if(splitAble(st)){
                       retVal itmp = retVal(splitNode(st), SPLITED);
                       delete st;
                       st = nullptr;
                       return itmp;
                   }
                   else{
                       retVal itmp = retVal(st->data[0], NOTHING);
                       delete st;
                       st = nullptr;
                       return itmp;
                   }
               }
           }
       }
       for(size_t i = st->sz - 1; i >= 0 && i <= st->sz; --i){
           cmpres = keyCompare(k, st->data[i].k);
           if(cmpres == 0){
               BPTNode *btmp = readNode(st->data[i].data);
               retVal dtmp = treeInsert(k, dta, btmp);
               if(dtmp.status == NOTHING){
                   st->data[0].k = dtmp.retDta.k;
                   writeNode(st, st->nodeOffset);
                   retVal itmp = retVal(st->data[0], NOTHING);
                   delete st;
                   st = nullptr;
                   return itmp;
               }
               else if(dtmp.status == SPLITED){
                   for(size_t j = st->sz - 1; j >= i + 1 && j <= st->sz; --j) st->data[j + 1] = st->data[j];
                   st->data[i + 1] = dtmp.retDta;
                   st->sz++;
                   writeNode(st, st->nodeOffset);
                   if(splitAble(st)){
                       retVal itmp = retVal(splitNode(st), SPLITED);
                       delete st;
                       st = nullptr;
                       return itmp;
                   }
                   else{
                       retVal itmp = retVal(st->data[0], NOTHING);
                       delete st;
                       st = nullptr;
                       return itmp;
                   }
               }
               else if(dtmp.status == INVALID){
                   retVal itmp = retVal(Key(), 0, INVALID);
                   delete st;
                   st = nullptr;
                   return itmp;
               }
           }
           else if(cmpres == 2) return retVal(Key(),0,INVALID);
       }
   }

   retVal treeInsertFirst(const Key &tk, const T &dta, BPTNode *st){
       if(st->nodeType == LEAF_NODE){
           if(st->sz == 0){
               st->data[0].k = tk;
               st->data[0].data = writeData(&dta);
               st->sz++;
               writeNode(st, st->nodeOffset);
               retVal itmp = retVal(st->data[0], NOTHING);
               delete st;
               st = nullptr;
               return itmp;
           }
           for(size_t i = st->sz - 1; i >= 0 && i <= st->sz; --i) st->data[i + 1] = st->data[i];
           st->data[0].k = tk;
           st->data[0].data = writeData(&dta);
           st->sz++;
           if(splitAble(st)){
               retVal itmp =  retVal(splitNode(st),SPLITED);
               delete st;
               st = nullptr;
               return itmp;
           }
           else{
               retVal itmp = retVal(st->data[0], NOTHING);
               delete st;
               st = nullptr;
               return itmp;
           }
       }
       if(keyCompare(tk, currentNode->data[0].k) != 1) return retVal(Key(), 0, INVALID);
       BPTNode *ntmp = readNode(st->data[0].data);
       st->data[0].k = tk;
       retVal rtmp = treeInsertFirst(tk, dta, ntmp);
       if(rtmp.status == SPLITED){
           for(size_t i = st->sz - 1; i >= 1 && i <= st->sz; --i) st->data[i + 1] = st->data[i];
           st->data[1] = rtmp.retDta;
           if(splitAble(st)){
               retVal itmp= retVal(splitNode(st), SPLITED);
               delete st;
               st = nullptr;
               return itmp;
           }
           else{
               retVal itmp = retVal(st->data[0], NOTHING);
               writeNode(st, st->nodeOffset);
               delete st;
               st = nullptr;
               return itmp;
           }
       }
       else if(rtmp.status == NOTHING){
           st->data[0].k = rtmp.retDta.k;
           writeNode(st, st->nodeOffset);
           retVal itmp = retVal(st->data[0], NOTHING);
           delete st;
           st = nullptr;
           return itmp;
       }
       else if(rtmp.status == INVALID){
           writeNode(st, st->nodeOffset);
           delete st;
           st = nullptr;
           return retVal(Key(), 0, INVALID);
       }
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
    bool insertData(const Key &k, const T &dta){
        changeToRoot();
        retVal rt;
        if(currentNode->sz == 0){
            rt = treeInsert(k, dta, currentNode);
            currentNode = nullptr;
            return 1;
        }
        int cmpres = keyCompare(k, currentNode->data[0].k);
        if(cmpres == 1){
            rt = treeInsertFirst(k, dta, currentNode);
            currentNode = nullptr;
        }
        else if(cmpres == 0) {
            rt = treeInsert(k, dta, currentNode);
            currentNode = nullptr;
        }
        if(rt.status == INVALID) return 0;
        else return 1;
    }

    T *find(const Key &k){
        T *trt = nullptr;
        treeData rt = treeFind(k);
        if(rt.data == INVALID_OFFSET){}
        else trt = readData(rt.data);
        return trt;
    }
    //bool removeData(){}

};

#endif
