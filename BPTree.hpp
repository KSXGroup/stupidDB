#ifndef _BPTREE_HPP_
#define _BPTREE_HPP_
#include <fstream>
#include <iostream>
#include <cstring>
#include <queue>
#include <assert.h>
#include "dbException.hpp"
#define IOB std::ios_base::in | std::ios_base::out | std::ios_base::binary
#define TIOB std::ios_base::trunc | std::ios_base::in | std::ios_base::out | std::ios_base::binary
//debug
size_t write_cnt = 0;
//file io
const size_t MAX_FILENAME_LEN = 30;
const size_t MAX_BLOCK_SIZE = 4;
const size_t FIRST_NODE_OFFSET = MAX_FILENAME_LEN * sizeof(char) * 2 + 2 * sizeof(size_t);
const size_t INVALID_OFFSET = -1;
//node type
const int INTERN_NODE = 1;
const int LEAF_NODE = 2;
const int DELETED = 3;
//ret type
const int NOTEXIST = 109;
const int INVALID = 110;
const int NOTHING = 111;
const int SPLITED = 112;
const int BORROWEDLEFT = 113;
const int BORROWEDRIGHT = 114;
const int MERGELEFT = 115;
const int MERGERIGHT = 116;
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
        size_t data = INVALID_OFFSET;
        treeData() = default;
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
        BPTNode() = default;
        BPTNode(const int &ndt):nodeType(ndt){}
        BPTNode(const BPTNode &other){
            nodeType = other.nodeType;
            sz = other.sz;
            for(int i = 0; i < sz; ++i) data[i] = other.data[i];
        }

        //DATA
        int nodeType = DELETED;
        size_t sz = 0;
        size_t nodeOffset = INVALID_OFFSET;
        size_t nextNode = INVALID_OFFSET;
        size_t prevNode = INVALID_OFFSET;
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

//*******************necessary function *****************//
    inline size_t min(const size_t &a, const size_t &b){
        return a < b ? a : b;
    }

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
            writeNode(tmp);
            return tmp;
        }
        delete tmp;
        tmp = nullptr;
        return tmp;
    }

    inline bool deleteNode(BPTNode *p, size_t offset){
        p->nodeType = DELETED;
        writeNode(p, p->nodeOffset);
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
        }
        fmgr.close();
        fmgr.open(dbFileMgr, IOB);
        fmgr.seekg(0, std::ios_base::end);
        fsize = fmgr.tellg();
        fmgr.seekg(0);
        if(fsize != 0){
            while(fmgr.tellg() != fsize){
                fmgr.read((char*)&offset, sizeof(size_t));
                QdbMgr.push(offset);
            }
        }
        fmgr.close();
        return 1;
    }

    //dont forget to delete after use readNode()!
    BPTNode *readNode(size_t offset){
        if(fidx.is_open() || fidx.fail()) fidx.close();
        fidx.open(idxFileName, IOB);
        if(!fidx.is_open() || fidx.fail() ) return nullptr;
        BPTNode *tmp = new BPTNode;
        fidx.seekg(offset);
        fidx.read((char*)&(tmp->nodeType), sizeof(int));
        fidx.read((char*)&(tmp->nextNode), sizeof(size_t));
        fidx.read((char*)&(tmp->prevNode), sizeof(size_t));
        fidx.read((char*)&(tmp->sz), sizeof(size_t));
        fidx.read((char*)&(tmp->nodeOffset), sizeof(size_t));
        fidx.read((char*)(tmp->data), sizeof(treeData) * MAX_BLOCK_SIZE);
        fidx.close();
        return tmp;
    }
    bool writeNode(BPTNode *p, size_t offset = 0){
        if(fidx.is_open() || fidx.fail()) fidx.close();
        fidx.open(idxFileName, IOB);
        if(offset == 0){
            fidx.seekg(0, std::ios_base::end);
            offset = fidx.tellg();
        }
        p->nodeOffset = offset;
        if(fidx.fail()){
            fidx.close();
            return 0;
        }
        fidx.seekp(offset);
        fidx.write((const char*)&(p->nodeType), sizeof(int));
        fidx.write((const char*)&(p->nextNode), sizeof(size_t));
        fidx.write((const char*)&(p->prevNode), sizeof(size_t));
        fidx.write((const char*)&(p->sz), sizeof(size_t));
        fidx.write((const char*)&(p->nodeOffset), sizeof(size_t));
        fidx.write((const char*)(p->data), sizeof(treeData) * MAX_BLOCK_SIZE);
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
    }

   T *readData(size_t offset){
        T *tmp = new T();
        if(fdb.fail() || fdb.is_open()) fdb.close();
        fdb.open(dbFileName, IOB);
        fdb.seekg(offset);
        fdb.read((char*)tmp, sizeof(T));
        fdb.close();
        return tmp;
   }

   size_t writeData(const T *dataPtr){
       if(fdb.is_open() || fdb.fail()) fdb.close();
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
       fdb.seekp(offset);
       fdb.write((const char*)dataPtr, sizeof(T));
       fdb.close();
       return offset;
   }

   size_t deleteData(size_t offset){
       //DBG
       size_t p = -1;
       fdb.close();
       fdb.open(dbFileName, IOB);
       fdb.seekp(offset);
       fdb.write((char*)&p, sizeof(size_t));
       fdb.close();
       QdbMgr.push(offset);
   }

    bool importIdxFile(const size_t dl){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        if(!fidx){
            if(dl == 0) throw ImportFileNotExist();
            if(currentNode) delete currentNode;
            fidx.close();
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

    bool splitAble(const BPTNode *p){
        if(p->sz == MAX_BLOCK_SIZE) return 1;
        else return 0;
    }
    //merge with right
    bool mergeNode(BPTNode *l, BPTNode *r){
        if(l == nullptr || r == nullptr || l->sz + r->sz >= MAX_BLOCK_SIZE) assert(0);
        for(size_t i = 0; i < r->sz; ++i) l->data[l->sz + i] = r->data[i];
        l->nextNode = r->nextNode;
        if(r->nextNode != (long long)(-1)){
            BPTNode *tmpRight = readNode(r->nextNode);
            tmpRight->prevNode = l->nodeOffset;
            writeNode(tmpRight, tmpRight->nodeOffset);
            delete tmpRight;
            tmpRight = nullptr;
        }
        l->sz += r->sz;
        r->sz = 0;
        writeNode(l, l->nodeOffset);
        deleteNode(r, r->nodeOffset);
        return 1;
    }

    retVal borrowFromRight(BPTNode *n, BPTNode *nxt){
        retVal tmpr;
        tmpr.status = INVALID;
        if(n->sz >= (MAX_BLOCK_SIZE >> 1)) return tmpr;
        if(nxt->sz <=(MAX_BLOCK_SIZE >> 1)) return tmpr;
        n->data[n->sz] = nxt->data[0];
        n->sz++;
        for(size_t i = 0 ; i < nxt->sz - 1; ++i) nxt->data[i] = nxt->data[i + 1];
        nxt->sz--;
        writeNode(n, n->nodeOffset);
        writeNode(nxt, nxt->nodeOffset);
        tmpr.retDta = nxt->data[0];
        tmpr.retDta.data = nxt->nodeOffset;
        tmpr.status = BORROWEDRIGHT;
        return tmpr;
    }

    retVal borrowFromLeft(BPTNode *prev, BPTNode *n){
        retVal tmpr;
        tmpr.status = INVALID;
        if(n->sz >= (MAX_BLOCK_SIZE >> 1)) return tmpr;
        if(prev->sz <= (MAX_BLOCK_SIZE >> 1)) return tmpr;
        for(size_t i = n->sz - 1; i >= 0 && i <= n->sz - 1; --i) n->data[i + 1] = n->data[i];
        n->data[0] = prev->data[prev->sz - 1];
        n->sz++;
        prev->sz--;
        writeNode(n, n->nodeOffset);
        writeNode(prev, prev->nodeOffset);
        tmpr.retDta = n->data[0];
        tmpr.retDta.data = n->nodeOffset;
        tmpr.status = BORROWEDLEFT;
        return tmpr;
    }

    //split
    treeData splitNode(BPTNode *p){
        BPTNode *ntmp = allocNode(p->nodeType), *tmpNext = nullptr;
        if(p->nextNode != -1) tmpNext = readNode(p->nextNode);
        ntmp->nextNode = p->nextNode;
        ntmp->prevNode = p->nodeOffset;
        p->nextNode = ntmp->nodeOffset;
        if(tmpNext){
            tmpNext->prevNode = ntmp->nodeOffset;
            writeNode(tmpNext, tmpNext->nodeOffset);
            delete tmpNext;
            tmpNext = nullptr;
        }
        for(size_t i = (MAX_BLOCK_SIZE >> 1) ; i < MAX_BLOCK_SIZE; ++i) ntmp->data[i - (MAX_BLOCK_SIZE >> 1)] = p->data[i];
        p->sz = (MAX_BLOCK_SIZE >> 1) ;
        ntmp->sz = MAX_BLOCK_SIZE - (MAX_BLOCK_SIZE >> 1);
        writeNode(p, p->nodeOffset);
        writeNode(ntmp, ntmp->nodeOffset);
        if(p->nodeOffset == rootOffset){
            BPTNode *tmpRoot = allocNode(INTERN_NODE);
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
   treeData treeFind(const Key &k, const BPTNode *&st){
       int cmpres = 0;
       size_t pos = st->sz;
       const BPTNode *tmpn = nullptr;
       treeData tmpr = treeData();
       if(st->nodeType == LEAF_NODE){
           for(size_t i = st->sz - 1; i >= 0 && i < st->sz; --i){
               cmpres = keyCompare(k, st->data[i].k);
               if(cmpres == 2){
                    pos = i;
                    break;
               }
           }
           if(pos < st->sz){
               tmpr = st->data[pos];
               delete st;
               st = nullptr;
               return tmpr;
           }
           else{
               delete st;
               st = nullptr;
               return tmpr;
           }
       }
       pos = st->sz;
       for(size_t i = st->sz - 1; i >= 0 && i < st->sz; --i){
          cmpres = keyCompare(k, st->data[i].k);
          if(cmpres == 0 || cmpres == 2){
               pos = i;
               break;
          }
       }
       if(pos < st->sz){
           tmpn = readNode(st->data[pos].data);
           tmpr = treeFind(k, tmpn);
       }
       delete st;
       st = nullptr;
       return tmpr;
    }

   retVal treeInsert(const Key &k, const T &dta, BPTNode *&st){
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
               if(cmpres == 2){
                   delete st;
                   st = nullptr;
                   return retVal(Key(), 0, INVALID);
               }
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
                   st->data[i].k = dtmp.retDta.k;
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
           else if(cmpres == 2){
               delete st;
               st = nullptr;
               return retVal(Key(),0,INVALID);
           }
       }
   }

   retVal treeInsertFirst(const Key &tk, const T &dta, BPTNode *&st){
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
           for(size_t i = st->sz - 1; i >= 0 && i <= st->sz - 1; --i) st->data[i + 1] = st->data[i];
           st->data[0].k = tk;
           st->data[0].data = writeData(&dta);
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
       if(keyCompare(tk, st->data[0].k) != 1) return retVal(Key(), 0, INVALID);
       BPTNode *ntmp = readNode(st->data[0].data);
       st->data[0].k = tk;
       retVal rtmp = treeInsertFirst(tk, dta, ntmp);
       if(rtmp.status == SPLITED){
           for(size_t i = st->sz - 1; i >= 1 && i <= st->sz; --i) st->data[i + 1] = st->data[i];
           st->data[1] = rtmp.retDta;
           st->sz++;
           writeNode(st, st->nodeOffset);
           if(splitAble(st)){
               retVal itmp= retVal(splitNode(st), SPLITED);
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
       else if(rtmp.status == NOTHING){
           st->data[0].k = rtmp.retDta.k;
           writeNode(st, st->nodeOffset);
           retVal itmp = retVal(st->data[0], NOTHING);
           delete st;
           st = nullptr;
           return itmp;
       }
       else if(rtmp.status == INVALID){
           delete st;
           st = nullptr;
           return retVal(Key(), 0, INVALID);
       }
   }

   retVal treeRemove(const Key &k, BPTNode *st){
       retVal tmpr;
       BPTNode *tmpn = nullptr, *tmpLeft = nullptr, *tmpRight = nullptr;
       size_t posFa = st->sz, posSon = 0;
       int cmpres = -1;
       if(st->sz == 0){
           tmpr.status = INVALID;
           return tmpr;
       }

       if(st->nodeType == LEAF_NODE){
           //assert(st->nodeOffset == rootOffset);
           for(size_t i = st->sz - 1; i >= 0 && i <= st->sz - 1; --i){
               cmpres = keyCompare(k, st->data[i].k);
               if(cmpres == 2){
                   posFa = i;
                   break;
               }
           }
           if(posFa == st->sz){
               tmpr.status = NOTEXIST;
               return tmpr;
           }
           deleteData(st->data[posFa].data);
           for(size_t i = posFa; i < st->sz - 1; ++i) st->data[i] = st->data[i + 1];
           st->sz--;
           writeNode(st, st->nodeOffset);
           tmpr.status = NOTHING;
           return tmpr;
       }

       for(size_t i = st->sz - 1; i >= 0 && i <= st->sz - 1; --i){
           cmpres = keyCompare(k, st->data[i].k);
           if(cmpres == 0 || cmpres == 2){
               posFa = i;
               break;
           }
       }
       if(posFa == st->sz){
           tmpr.status = NOTEXIST;
           return tmpr;
       }
       tmpn = readNode(st->data[posFa].data);
       posSon = tmpn->sz;
       if(tmpn->nodeType == LEAF_NODE){
           for(size_t i = tmpn->sz - 1; i >= 0 && i <= tmpn->sz - 1; --i){
               cmpres = keyCompare(k, tmpn->data[i].k);
               if(cmpres == 2){
                   posSon = i;
                   break;
               }
           }
           if(posSon == tmpn->sz){
               delete tmpn;
               tmpn = nullptr;
               tmpr.status = NOTEXIST;
               return tmpr;
           }
           deleteData(tmpn->data[posSon].data);
           for(size_t i = posSon; i < tmpn->sz - 1; ++i) tmpn->data[i] = tmpn->data[i + 1];
           tmpn->sz--;
           writeNode(tmpn, tmpn->nodeOffset);
       }
       else tmpr = treeRemove(k, tmpn);
       if(tmpr.status == MERGELEFT || tmpr.status == MERGERIGHT || tmpn->nodeType == LEAF_NODE){
           if(tmpr.status == MERGELEFT){
               st->data[posFa] = tmpn->data[0];
               st->data[posFa].data = tmpn->nodeOffset;
               writeNode(st, st->nodeOffset);
           }
           if(tmpn->sz < (MAX_BLOCK_SIZE >> 1)){
               if(posFa +  1 <= st->sz - 1) tmpRight = readNode(st->data[posFa + 1].data);
               if(posFa - 1 <= st->sz - 1) tmpLeft = readNode(st->data[posFa - 1].data);

               if(tmpRight && tmpRight->sz > (MAX_BLOCK_SIZE >> 1)){
                   tmpr = borrowFromRight(tmpn, tmpRight);
                   st->data[posFa + 1].k = tmpRight->data[0].k;
                   writeNode(st,st->nodeOffset);
               }
               else if(tmpLeft && tmpLeft->sz > (MAX_BLOCK_SIZE >> 1)){
                   tmpr = borrowFromLeft(tmpLeft, tmpn);
                   st->data[posFa].k = tmpn->data[0].k;
                   writeNode(st, st->nodeOffset);
               }
               else if(tmpLeft && tmpLeft->sz <= (MAX_BLOCK_SIZE >> 1)){
                   mergeNode(tmpLeft, tmpn);
                   for(size_t i = posFa; i < st->sz - 1; ++i) st->data[i] = st->data[i + 1];
                   st->sz--;
                   writeNode(st, st->nodeOffset);
                   if(st->sz == 1 && st->nodeOffset == rootOffset){
                       rootOffset = st->data[0].data;
                       writeIdx();
                       deleteNode(st, st->nodeOffset);
                   }
                   tmpr.status = MERGELEFT;
                   tmpr.retDta = st->data[0];
                   tmpr.retDta.data = st->nodeOffset;
               }
               else if(tmpRight && tmpRight->sz <= (MAX_BLOCK_SIZE >> 1)){
                   mergeNode(tmpn, tmpRight);
                   for(size_t i = posFa + 1; i < st->sz - 1; ++i) st->data[i] = st->data[i + 1];
                   st->sz--;
                   writeNode(st, st->nodeOffset);
                   if(st->sz == 1 && st->nodeOffset == rootOffset){
                       rootOffset = st->data[0].data;
                       writeIdx();
                       deleteNode(st ,st->nodeOffset);
                   }
                   tmpr.status = MERGERIGHT;
                   tmpr.retDta = st->data[0];
                   tmpr.retDta.data = st->nodeOffset;
               }
               if(tmpn ->nodeType == LEAF_NODE){
                   st->data[posFa] = tmpn->data[0];
                   st->data[posFa].data = tmpn->nodeOffset;
                   writeNode(st, st->nodeOffset);
               }
               if(tmpLeft) delete tmpLeft;
               if(tmpRight) delete tmpRight;
               delete tmpn;
               tmpn = tmpRight = tmpLeft = nullptr;
               return tmpr;
           }
           else{
               delete tmpn;
               tmpn = nullptr;
               tmpr.status = NOTHING;
               return tmpr;
           }
       }
       else if(tmpr.status == BORROWEDLEFT){
           st->data[posFa] = tmpn->data[0];
           st->data[posFa].data = tmpn->nodeOffset;
           writeNode(st, st->nodeOffset);
           delete tmpn;
           tmpn = nullptr;
           tmpr.status = NOTHING;
           return tmpr;
       }
       else if(tmpr.status == BORROWEDRIGHT){
           delete tmpn;
           tmpn = nullptr;
           tmpr.status = NOTHING;
           return tmpr;
       }
       else if(tmpr.status == NOTEXIST || tmpr.status == NOTHING || tmpr.status == INVALID){
           delete tmpn;
           tmpn = nullptr;
           return tmpr;
       }
       else{
           tmpr.status = INVALID;
           delete tmpn;
           tmpn = nullptr;
           return tmpr;
       }

   }

   void treeDfs(const BPTNode *&st){
        const BPTNode *tmpn = nullptr;
        const T *tmpd = nullptr;
       if(st->nodeType == INTERN_NODE){
           for(size_t i = 0; i < st->sz; ++i){
               tmpn = readNode(st->data[i].data);
               treeDfs(tmpn);
           }
           delete st;
           st = nullptr;
           return;
       }
       if(st->nodeType == LEAF_NODE){
           for(size_t i = 0; i < st->sz; ++i){
               tmpd = readData(st->data[i].data);
               cout << *tmpd << "\t";
               delete tmpd;
               tmpd = nullptr;
           }
           delete st;
           st = nullptr;
           return;
       }
   }

public:
    BPTree(const char* s){
        memset(idxFileName, 0, sizeof(idxFileName));
        memset(dbFileName, 0, sizeof(dbFileName));
        memset(idxFileMgr, 0, sizeof(idxFileMgr));
        memset(dbFileMgr, 0, sizeof(dbFileMgr));
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
        if(fmgr.is_open() || fmgr.fail()) fmgr.close();
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
            return 1;
        }
        int cmpres = keyCompare(k, currentNode->data[0].k);
        if(cmpres == 1){
            rt = treeInsertFirst(k, dta, currentNode);
        }
        else if(cmpres == 0) {
            rt = treeInsert(k, dta, currentNode);
        }
        if(rt.status == INVALID) return 0;
        else return 1;
    }

    bool removeData(const Key &k){
        changeToRoot();
        retVal rt;
        if(currentNode->sz == 0) return 0;
        rt = treeRemove(k, currentNode);
        if(rt.status == INVALID) return 0;
        else return 1;
    }

    T *findU(const Key &k){
        changeToRoot();
        const BPTNode *crt = currentNode;
        currentNode = nullptr;
        T *trt = nullptr;
        treeData rt = treeFind(k, crt);
        if(rt.data != -1) trt = readData(rt.data);
        return trt;
    }

    void dfs(){
        const BPTNode *p = nullptr;
        changeToRoot();
        p = currentNode;
        treeDfs(p);
        currentNode = nullptr;
    }

};

#endif
