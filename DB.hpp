#ifndef _STUPID_DB_HPP_
#define _STUPID_DB_HPP_
#include "BPTree.hpp"
template <typename D, typename K>
class DB{
private:
    BPTree<K, D, std::less<K> > *tree = nullptr;
public:
    DB(){}
    ~DB(){
        if(tree) delete tree;
        tree = nullptr;
    }
    void init(const char *fileName){
        tree = new BPTree<D, K>(fileName);
        for(int i = 1; i <= 1000; ++i) tree->insertData(i, i);
        tree->dfs();
    }
    void insert(const K &Key, const D &dta){
        tree->insertData(Key, dta);
    }
    void modify(const K &Key, const D &dta){
        tree->modifyData(Key, dta);
    }
    mypair<D*, bool> find(const K &Key){
        D* p = nullptr;
        p = tree->findU(Key);
        if(!p) return mypair<D*, bool>(0, p);
        else return mypair<D*, bool>(1, p);
    }

    void findRange(const K &l, const K &r, sjtu::vetor<D> &vec){
        tree->findR(l, r, vec);
    }
};
#endif
