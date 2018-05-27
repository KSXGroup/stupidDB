#ifndef _STUPID_DB_HPP_
#define _STUPID_DB_HPP_
#include "BPTree.hpp"
template <typename K, typename D, typename Compare =  std::less<K> >
class DB{
private:
    BPTree<K, D, Compare> *tree = nullptr;
public:
    DB(){}
    ~DB(){
        if(tree) delete tree;
        tree = nullptr;
    }
    void init(const char *fileName){
        tree = new BPTree<K, D, Compare>(fileName);
        //for(int i = 1; i <= 1000; ++i) tree->insertData(i, i);
        //tree->dfs();
    }
    bool insert(const K &Key, const D &dta){
        return tree->insertData(Key, dta);
    }
    bool modify(const K &Key, const D &dta){
        return tree->modifyData(Key, dta);
    }

    bool erase(const K &Key){
        return tree->removeData(Key);
    }

    mypair<D*, bool> find(const K &Key){
        D* p = nullptr;
        p = tree->findU(Key);
        if(!p) return mypair<D*, bool>(p, 0);
        else return mypair<D*, bool>(p, 1);
    }

    void findRange(const K &l, const K &r, sjtu::vector<K> &vec){
        tree->findR(l, r, vec);
    }

    void findRangeData(const K &l, const K &r, sjtu::vector<D> &vec){
        tree->findRD(l, r, vec);
    }

    size_t size() const{
        return tree->size();
    }

    //warning!
    void trunc(){
        tree->trunc();
    }
};
#endif
