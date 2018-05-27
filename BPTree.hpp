#ifndef _BPTREE_HPP_
#define _BPTREE_HPP_
#include <fstream>
#include <iostream>
#include <cstring>
#include "queue.h"
#include <assert.h>
#include "vector.hpp"
#include "dbException.hpp"
#define IOB std::ios_base::in | std::ios_base::out | std::ios_base::binary
#define TIOB std::ios_base::trunc | std::ios_base::in | std::ios_base::out | std::ios_base::binary
#define OFFSET_TYPE unsigned long long
//file io
const OFFSET_TYPE MAX_FILENAME_LEN = 30;
const OFFSET_TYPE MAX_BLOCK_SIZE = 252;
const OFFSET_TYPE FIRST_NODE_OFFSET = MAX_FILENAME_LEN * sizeof(char) * 2 + 2 * sizeof( OFFSET_TYPE );
const OFFSET_TYPE INVALID_OFFSET = -1;
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

template<typename A, typename B>
struct mypair{
    A first;
    B second;
    mypair(A st, B nd):first(st), second(nd){}
};


template<typename Key, typename T, typename Compare>
class BPTree;

template<typename Key, typename T, typename Compare = std::less<Key> >
class BPTree{
private:

    struct treeData{
        Key k = Key();
        OFFSET_TYPE data = INVALID_OFFSET;
        treeData() = default;
    };

    struct retVal{
        treeData retDta;
        int status = INVALID;
        retVal() = default;
        retVal(const Key &_k, const OFFSET_TYPE &_d, const int &_status):status(_status){
            retDta.data = _d;
            retDta.k = _k;
        }
        retVal(const treeData &td, const int &_status): retDta(td), status(_status){}
    };


    struct BPTNode{
        BPTNode() = default;
        BPTNode(const int &ndt):nodeType(ndt){}
        //DATA
        int nodeType = DELETED;
        OFFSET_TYPE sz = 0;
        OFFSET_TYPE nodeOffset = INVALID_OFFSET;
        OFFSET_TYPE nextNode = INVALID_OFFSET;
        OFFSET_TYPE prevNode = INVALID_OFFSET;
        treeData data[MAX_BLOCK_SIZE];
    };

private:
    inline int keyCompare(const Key &a, const Key &b){
        Compare cmp;
        if(cmp(a, b)) return 1; // less
        if(cmp(b, a)) return 0; // more
        return 2; //equal;
    }

    inline OFFSET_TYPE binSearch(const BPTNode *p, const Key &k){
        OFFSET_TYPE lo = 0, hi = p->sz - 1, mid = 0;
        while(hi != lo){
            mid = (lo + hi + 1) >> 1;
            if(keyCompare(k, p->data[mid].k) == 1) hi = mid - 1;
            else lo = mid;
        }
        return lo;
    }

    inline OFFSET_TYPE binSearchForRange(const BPTNode *p, const Key &k){
        OFFSET_TYPE lo = 0, hi = p->sz - 1, mid = 0, ans = -1;
        while(lo <= hi){
            mid = (lo + hi) >> 1;
            if(keyCompare(p->data[mid].k, k) == 1) lo = mid + 1;
            else{
               ans = mid;
               hi = mid - 1;
               if (hi == -1) return ans;
            }
        }
        return ans;
    }

private:
    char idxFileName[MAX_FILENAME_LEN];
    char idxFileMgr[MAX_FILENAME_LEN];
    char dbFileName[MAX_FILENAME_LEN];
    char dbFileMgr[MAX_FILENAME_LEN];
    OFFSET_TYPE dataSize = 0;
    OFFSET_TYPE rootOffset = 0;
    std::fstream fidx;
    std::fstream fdb;
    std::fstream fmgr;
    stl::queue< OFFSET_TYPE > QidxMgr;
    stl::queue< OFFSET_TYPE > QdbMgr;
    BPTNode* currentNode = nullptr;

//*******************file IO****************************//

    //Dont forget to DELETE afte using allocNewNode()!
    BPTNode *allocNode(const int &nodeType){
        if(fmgr.is_open()) fmgr.close();
        BPTNode *tmp = new BPTNode(nodeType);
        OFFSET_TYPE offset = 0;
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

    inline bool deleteNode(BPTNode *p, OFFSET_TYPE offset){
        QidxMgr.push(offset);
        return 1;
    }

    bool writeIdx(){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        fidx.write(idxFileName, sizeof(char) * MAX_FILENAME_LEN);
        fidx.write(dbFileName, sizeof(char) * MAX_FILENAME_LEN);
        fidx.write((const char*)&dataSize, sizeof( OFFSET_TYPE ));
        fidx.write((const char*)&rootOffset, sizeof( OFFSET_TYPE ));
        fidx.close();
        return 1;
    }

    bool readIdx(){
        if(fidx.is_open()) fidx.close();
        fidx.open(idxFileName, IOB);
        char tmp[MAX_FILENAME_LEN];
        OFFSET_TYPE offset = 0;
        fidx.read(tmp, sizeof(char) * MAX_FILENAME_LEN);
        if(strcmp(idxFileName, tmp) != 0) throw fileNotMatch();
        strcpy(idxFileName, tmp);
        fidx.read(dbFileName, sizeof(char) * MAX_FILENAME_LEN);
        fidx.read((char*)&dataSize, sizeof( OFFSET_TYPE ));
        fidx.read((char*)&rootOffset, sizeof( OFFSET_TYPE ));
        fidx.close();

        //I will read mgr file into Q here
        OFFSET_TYPE fsize = 0;
        fmgr.open(idxFileMgr, IOB);
        fmgr.seekg(0, std::ios_base::end);
        fsize = fmgr.tellg();
        fmgr.seekg(0);
        if(fsize != 0){
            while(fmgr.tellg() != fsize){
                fmgr.read((char*)&offset, sizeof( OFFSET_TYPE ));
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
                fmgr.read((char*)&offset, sizeof( OFFSET_TYPE ));
                QdbMgr.push(offset);
            }
        }
        fmgr.close();
        return 1;
    }

    //dont forget to delete after use readNode()!
    BPTNode *readNode(OFFSET_TYPE offset){
        if(fidx.is_open() || fidx.fail()) fidx.close();
        fidx.open(idxFileName, IOB);
        if(!fidx.is_open() || fidx.fail() ) return nullptr;
        BPTNode *tmp = new BPTNode;
        fidx.seekg(offset);
        fidx.read((char*)&(tmp->nodeType), sizeof(int));
        fidx.read((char*)&(tmp->nextNode), sizeof( OFFSET_TYPE ));
        fidx.read((char*)&(tmp->prevNode), sizeof( OFFSET_TYPE ));
        fidx.read((char*)&(tmp->sz), sizeof( OFFSET_TYPE ));
        fidx.read((char*)&(tmp->nodeOffset), sizeof( OFFSET_TYPE ));
        fidx.read((char*)(tmp->data), sizeof(treeData) * MAX_BLOCK_SIZE);
        fidx.close();
        return tmp;
    }
    bool writeNode(BPTNode *p, OFFSET_TYPE offset = 0){
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
        fidx.write((const char*)&(p->nextNode), sizeof( OFFSET_TYPE ));
        fidx.write((const char*)&(p->prevNode), sizeof( OFFSET_TYPE ));
        fidx.write((const char*)&(p->sz), sizeof( OFFSET_TYPE ));
        fidx.write((const char*)&(p->nodeOffset), sizeof( OFFSET_TYPE ));
        fidx.write((const char*)(p->data), sizeof(treeData) * MAX_BLOCK_SIZE);
        fidx.close();
        return 1;
    }

    inline void changeToRoot(){
        if(currentNode && currentNode->nodeOffset == rootOffset) return;
        if(currentNode){
            writeNode(currentNode, currentNode->nodeOffset);
            delete currentNode;
            currentNode = nullptr;
        }
        currentNode = readNode(rootOffset);
    }

   T *readData(OFFSET_TYPE offset){
        T *tmp = new T();
        if(fdb.fail() || fdb.is_open()) fdb.close();
        fdb.open(dbFileName, IOB);
        fdb.seekg(offset);
        fdb.read((char*)tmp, sizeof(T));
        fdb.close();
        return tmp;
   }

   OFFSET_TYPE writeData(const T *dataPtr){
       if(fdb.is_open() || fdb.fail()) fdb.close();
       OFFSET_TYPE offset = 0;
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

   OFFSET_TYPE modData(const T *dataPtr, const OFFSET_TYPE &offset){
       if(fdb.is_open() || fdb.fail()) fdb.close();
       fdb.open(dbFileName, IOB);
       if(fdb.fail()) return INVALID_OFFSET;
       fdb.seekp(offset);
       fdb.write((const char*)dataPtr, sizeof(T));
       fdb.close();
       return offset;
   }

   bool deleteData(OFFSET_TYPE offset){
       //DBG
       /*OFFSET_TYPE p = -1;
       fdb.close();
       fdb.open(dbFileName, IOB);
       fdb.seekp(offset);
       fdb.write((char*)&p, sizeof( OFFSET_TYPE ));
       fdb.close();*/
       QdbMgr.push(offset);
       return 1;
   }

    bool importIdxFile(const OFFSET_TYPE dl){
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
        for(OFFSET_TYPE i = 0; i < r->sz; ++i) l->data[l->sz + i] = r->data[i];
        l->nextNode = r->nextNode;
        if(r->nextNode != (OFFSET_TYPE)(-1)){
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
        for(OFFSET_TYPE i = 0 ; i < nxt->sz - 1; ++i) nxt->data[i] = nxt->data[i + 1];
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
        for(OFFSET_TYPE i = n->sz - 1; i >= 0 && i <= n->sz - 1; --i) n->data[i + 1] = n->data[i];
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
        for(OFFSET_TYPE i = (MAX_BLOCK_SIZE >> 1) ; i < MAX_BLOCK_SIZE; ++i) ntmp->data[i - (MAX_BLOCK_SIZE >> 1)] = p->data[i];
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
       OFFSET_TYPE pos = 0;
       const BPTNode *tmpn = nullptr;
       treeData tmpr = treeData();
       if(st->nodeType == LEAF_NODE){
           pos = binSearch(st, k);
           cmpres = keyCompare(k, st->data[pos].k);
           if(cmpres == 2){
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
       pos = binSearch(st, k);
       cmpres = keyCompare(k, st->data[pos].k);
       if(cmpres == 0 || cmpres == 2){
           tmpn = readNode(st->data[pos].data);
           tmpr = treeFind(k, tmpn);
       }
       delete st;
       st = nullptr;
       return tmpr;
    }

   retVal treeInsert(const Key &k, const T &dta, BPTNode *&st){
       int cmpres = 0;
       size_t pos = st->sz;
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
           pos = binSearch(st, k);
           cmpres = keyCompare(k, st->data[pos].k);
           if(cmpres == 2){
               delete st;
               st = nullptr;
               return retVal(Key(), 0, INVALID);
           }
           else if(cmpres == 0){
               for(OFFSET_TYPE j = st->sz - 1; j >= pos + 1 && j <= st->sz; --j) st->data[j + 1] = st->data[j];
               treeData ins;
               ins.data = writeData(&dta);
               ins.k = k;
               st->data[pos + 1] = ins;
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
       pos = binSearch(st, k);
       cmpres = keyCompare(k, st->data[pos].k);
       if(cmpres == 0){
           BPTNode *btmp = readNode(st->data[pos].data);
           retVal dtmp = treeInsert(k, dta, btmp);
           if(dtmp.status == NOTHING){
               st->data[pos].k = dtmp.retDta.k;
               writeNode(st, st->nodeOffset);
               retVal itmp = retVal(st->data[0], NOTHING);
               delete st;
               st = nullptr;
               return itmp;
           }
           else if(dtmp.status == SPLITED){
               for(OFFSET_TYPE j = st->sz - 1; j >= pos + 1 && j <= st->sz; --j) st->data[j + 1] = st->data[j];
               st->data[pos + 1] = dtmp.retDta;
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
           for(OFFSET_TYPE i = st->sz - 1; i >= 0 && i <= st->sz - 1; --i) st->data[i + 1] = st->data[i];
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
           for(OFFSET_TYPE i = st->sz - 1; i >= 1 && i <= st->sz; --i) st->data[i + 1] = st->data[i];
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
    //private remove
   retVal treeRemove(const Key &k, BPTNode *st){
       int cmpres = -1;
       retVal tmpr;
       BPTNode *tmpn = nullptr, *tmpLeft = nullptr, *tmpRight = nullptr;
       OFFSET_TYPE posFa = st->sz, posSon = 0;
       if(st->sz == 0){
           tmpr.status = INVALID;
           return tmpr;
       }

       if(st->nodeType == LEAF_NODE){
           posFa = binSearch(st, k);
           cmpres = keyCompare(k, st->data[posFa].k);
           if(cmpres != 2){
               tmpr.status = NOTEXIST;
               return tmpr;
           }
           deleteData(st->data[posFa].data);
           for(OFFSET_TYPE i = posFa; i < st->sz - 1; ++i) st->data[i] = st->data[i + 1];
           st->sz--;
           writeNode(st, st->nodeOffset);
           tmpr.status = NOTHING;
           return tmpr;
       }
       posFa = binSearch(st, k);
       cmpres = keyCompare(k, st->data[posFa].k);
       if(cmpres != 0 && cmpres != 2){
           tmpr.status = NOTEXIST;
           return tmpr;
       }
       tmpn = readNode(st->data[posFa].data);
       posSon = tmpn->sz;
       if(tmpn->nodeType == LEAF_NODE){
           posSon = binSearch(tmpn, k);
           cmpres = keyCompare(k, tmpn->data[posSon].k);
           if(cmpres != 2){
               delete tmpn;
               tmpn = nullptr;
               tmpr.status = NOTEXIST;
               return tmpr;
           }
           deleteData(tmpn->data[posSon].data);
           for(OFFSET_TYPE i = posSon; i < tmpn->sz - 1; ++i) tmpn->data[i] = tmpn->data[i + 1];
           tmpn->sz--;
           if(keyCompare(st->data[posFa].k,tmpn->data[0].k) != 2){
                st->data[posFa] = tmpn->data[0];
                st->data[posFa].data = tmpn->nodeOffset;
                writeNode(st, st->nodeOffset);
           }
           writeNode(tmpn, tmpn->nodeOffset);
       }
       else{
           tmpr = treeRemove(k, tmpn);
           if(keyCompare(st->data[posFa].k, tmpn->data[0].k) != 2){
               st->data[posFa].k = tmpn->data[0].k;
               writeNode(st, st->nodeOffset);
           }
       }
       if(tmpr.status == MERGELEFT || tmpr.status == MERGERIGHT || tmpn->nodeType == LEAF_NODE){
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
                   for(OFFSET_TYPE i = posFa; i < st->sz - 1; ++i) st->data[i] = st->data[i + 1];
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
                   for(OFFSET_TYPE i = posFa + 1; i < st->sz - 1; ++i) st->data[i] = st->data[i + 1];
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
               if(tmpLeft) delete tmpLeft;
               if(tmpRight) delete tmpRight;
               delete tmpn;
               tmpn = tmpRight = tmpLeft = nullptr;
               return tmpr;
           }
           else{
               writeNode(st, st->nodeOffset);
               delete tmpn;
               tmpn = nullptr;
               tmpr.status = NOTHING;
               return tmpr;
           }
       }
       else if(tmpr.status != NOTEXIST || tmpr.status != INVALID) tmpr.status = NOTHING;
       delete tmpn;
       tmpn = nullptr;
       return tmpr;

   }

   //private modify
   int treeModify(const Key &k, const T &dta, const BPTNode *st){
       const BPTNode *tmpn = nullptr;
       int cmpres = 0;
       OFFSET_TYPE posFa = st->sz, posSon = 0, res = 0;
       int ret = -1;
       if(st->sz == 0) return NOTEXIST;
       if(st->nodeType == LEAF_NODE){
           posFa = binSearch(st, k);
           cmpres = keyCompare(k, st->data[posFa].k);
           if(cmpres != 2) return NOTEXIST;
           else res = modData(&dta,st->data[posFa].data);
           if(res == -1) return INVALID;
           else return NOTHING;
       }
       posFa = binSearch(st, k);
       cmpres = keyCompare(k, st->data[posFa].k);
       if(cmpres == 1) return INVALID;
       tmpn = readNode(st->data[posFa].data);
       if(tmpn->nodeType == LEAF_NODE){
           posSon = binSearch(tmpn, k);
           if(keyCompare(k, tmpn->data[posSon].k) != 2){
               delete tmpn;
               tmpn = nullptr;
               return NOTEXIST;
           }
           else{
               res = modData(&dta, tmpn->data[posSon].data);
               delete tmpn;
               tmpn = nullptr;
               return res == -1 ? INVALID : NOTHING;
           }
       }
       else{
           ret = treeModify(k, dta, tmpn);
           delete tmpn;
           tmpn = nullptr;
           return ret;
       }

   }

   void treeFindRange(const Key &kl, const Key &kr, const BPTNode *st, sjtu::vector<Key> &vec){
       if(keyCompare(kl, kr) == 0){
           delete st;
           st = nullptr;
           return;
       }
       const BPTNode *tmpn = nullptr;
       //T *dtaptr = nullptr;
       OFFSET_TYPE pos = 0;
       bool sonflag = 0;
       if(st->nodeType == LEAF_NODE){
           pos = binSearchForRange(st, kl);
           while(1){
               for(;pos < st->sz && keyCompare(st->data[pos].k, kr) != 0;++pos) vec.push_back(st->data[pos].k);
               if(keyCompare(st->data[pos].k, kr) != 0 && st->nextNode != (OFFSET_TYPE)(-1)){
                   tmpn = readNode(st->nextNode);
                   delete st;
                   st = tmpn;
                   tmpn = nullptr;
                   pos = 0;
               }
               else{
                  delete st;
                  st = nullptr;
                  return;
               }
           }
       }
       else{
           pos = binSearch(st, kl);
           tmpn = readNode(st->data[pos].data);
           if(tmpn->nodeType == LEAF_NODE) sonflag = 1;
           treeFindRange(kl, kr, tmpn, vec);
           if(!sonflag) delete tmpn;
           tmpn = nullptr;
           return;
       }
   }


   void treeFindRangeForData(const Key &kl, const Key &kr, const BPTNode *st, sjtu::vector<T> &vec){
       if(keyCompare(kl, kr) == 0){
            delete st;
            st = nullptr;
            return;
       }
       const BPTNode *tmpn = nullptr;
       T *dtaptr = nullptr;
       OFFSET_TYPE pos = 0;
       bool sonflag = 0;
       if(st->nodeType == LEAF_NODE){
           pos = binSearchForRange(st, kl);
           if(pos == -1){
               delete st;
               st = nullptr;
               return;
           }
           while(1){
               for(;pos < st->sz && keyCompare(st->data[pos].k, kr) != 0;++pos){
                   dtaptr = readData(st->data[pos].data);
                   vec.push_back(*dtaptr);
                   delete dtaptr;
                   dtaptr = nullptr;
               }
               if(keyCompare(st->data[pos].k, kr) != 0 && st->nextNode != (OFFSET_TYPE)(-1)){
                   tmpn = readNode(st->nextNode);
                   delete st;
                   st = tmpn;
                   tmpn = nullptr;
                   pos = 0;
               }
               else{
                  delete st;
                  st = nullptr;
                  return;
               }
           }
       }
       else{
           pos = binSearch(st, kl);
           tmpn = readNode(st->data[pos].data);
           if(tmpn->nodeType == LEAF_NODE) sonflag = 1;
           treeFindRangeForData(kl, kr, tmpn, vec);
           if(!sonflag) delete tmpn;
           tmpn = nullptr;
           return;
       }
   }

   void treeClear(){
       QidxMgr.clear();
       QdbMgr.clear();
       fidx.close();
       fdb.close();
       fmgr.close();
       fmgr.open(idxFileMgr, TIOB);
       fmgr.close();
       fmgr.open(dbFileMgr, TIOB);
       fmgr.close();
       fmgr.open(idxFileName, TIOB);
       fmgr.close();
       fmgr.open(dbFileName , TIOB);
       fmgr.close();
       dataSize = 0;
       writeIdx();
       currentNode = allocNode(LEAF_NODE);
       rootOffset = currentNode->nodeOffset;
       writeIdx();
   }

   //for debug only
   void treeDfs(const BPTNode *&st){
        const BPTNode *tmpn = nullptr;
        const T *tmpd = nullptr;
       if(st->nodeType == INTERN_NODE){
           for(OFFSET_TYPE i = 0; i < st->sz; ++i){
               tmpn = readNode(st->data[i].data);
               treeDfs(tmpn);
           }
           delete st;
           st = nullptr;
           return;
       }
       if(st->nodeType == LEAF_NODE){
           for(OFFSET_TYPE i = 0; i < st->sz; ++i){
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
        for(OFFSET_TYPE i = 0; i <= strlen(s); ++i) idxFileName[i] = s[i];
        for(OFFSET_TYPE i = 0; i <= strlen(s); ++i) dbFileName[i] = s[i];
        for(OFFSET_TYPE i = 0; i <= strlen(s); ++i) idxFileMgr[i] = s[i];
        for(OFFSET_TYPE i = 0; i <= strlen(s); ++i) dbFileMgr[i] = s[i];
        strcat(idxFileName, IDX_SUFFIX);
        strcat(dbFileName, DB_SUFFIX);
        strcat(idxFileMgr, IDX_MGR_SUFFIX);
        strcat(dbFileMgr, DB_MGR_SUFFIX);
        importIdxFile(sizeof(T));
    }

    BPTree() = default;

    ~BPTree(){
        if(currentNode) delete currentNode;
        currentNode = nullptr;
        OFFSET_TYPE offset = 0;

        //Dump Q into files
        if(fmgr.is_open() || fmgr.fail()) fmgr.close();
        fmgr.open(idxFileMgr, TIOB);
        while(!QidxMgr.empty()){
            offset = QidxMgr.front();
            fmgr.write((char*)&offset, sizeof( OFFSET_TYPE ));
            QidxMgr.pop();
        }
        fmgr.close();
        fmgr.open(dbFileMgr, TIOB);
        while(!QdbMgr.empty()){
            offset = QdbMgr.front();
            fmgr.write((char*)&offset, sizeof( OFFSET_TYPE ));
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
            dataSize++;
            writeIdx();
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
        else{
            dataSize++;
            writeIdx();
            return 1;
        }
    }

    bool removeData(const Key &k){
        changeToRoot();
        retVal rt;
        if(currentNode->sz == 0) return 0;
        rt = treeRemove(k, currentNode);
        if(rt.status == INVALID || rt.status == NOTEXIST) return 0;
        else{
            dataSize--;
            writeIdx();
            return 1;
        }
    }

    bool modifyData(const Key &k , const T &dta){
        changeToRoot();
        int ret = -1;
        if(currentNode->sz == 0) return 0;
        ret = treeModify(k, dta, currentNode);
        if(ret ==  INVALID || ret == NOTEXIST) return 0;
        else return 1;
    }

    T *findU(const Key &k){
        changeToRoot();
        if(currentNode->sz == 0) return nullptr;
        const BPTNode *crt = currentNode;
        currentNode = nullptr;
        T *trt = nullptr;
        if(crt->sz == 0) return trt;
        treeData rt = treeFind(k, crt);
        if(rt.data != -1) trt = readData(rt.data);
        return trt;
    }


    void findR(const Key &kl, const Key &kr, sjtu::vector<Key> &vec){
        if(dataSize == 0) return;
        changeToRoot();
        const BPTNode *fst = currentNode;
        treeFindRange(kl, kr, fst, vec);
        currentNode = nullptr;
    }

    void findRD(const Key &kl, const Key &kr, sjtu::vector<T> &vec){
        if(dataSize == 0) return;
        changeToRoot();
        const BPTNode *fst = currentNode;
        treeFindRangeForData(kl, kr, fst, vec);
        currentNode = nullptr;
    }

    void dfs(){
        const BPTNode *p = nullptr;
        changeToRoot();
        p = currentNode;
        treeDfs(p);
        currentNode = nullptr;
    }

    OFFSET_TYPE size() const{
        return dataSize;
    }

    void trunc(){
        treeClear();
    }

};

#endif
