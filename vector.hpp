#ifndef SJTU_VECTOR_HPP
#define SJTU_VECTOR_HPP

#include "exceptions.hpp"

#include <climits>
#include <cstddef>
#include <vector>

namespace sjtu {
/**
 * a data container like std::vector
 * store data in a successive memory and support random access.
 */
template<typename T>
class vector {
private:
        T *data = nullptr;
        size_t cap = 0;
        size_t sz = 0;

        void doubleCap(){
            cap *= 2;
            T* tmp = (T*)malloc(sizeof(T) * cap);
            for(size_t i = 0; i < sz; ++i) new(tmp + i)T(data[i]);
            for(size_t i = 0; i < sz; ++i) data[i].~T();
            free(data);
            data = tmp;
            tmp = nullptr;
            return;
        }
public:
	/**
	 * TODO
	 * a type for actions of the elements of a vector, and you should write
	 *   a class named const_iterator with same interfaces.
	 */
	/**
	 * you can see RandomAccessIterator at CppReference for help.
	 */
	class const_iterator;
	class iterator {
            friend class vector<T>;
            friend class const_iterator;
	private:
		/**
		 * TODO add data members
		 *   just add whatever you want.
		 */
                vector<T> *v = nullptr;
                size_t pos = 0;
	public:
                iterator() = default;
                iterator(vector *vec, size_t p):v(vec), pos(p){}
		/**
		 * return a new iterator which pointer n-next elements
		 *   even if there are not enough elements, just return the answer.
		 * as well as operator-
		 */
		iterator operator+(const int &n) const {
                        return iterator(v, pos + n);
		}
		iterator operator-(const int &n) const {
                        return iterator(v, pos - n);
                }
		// return th distance between two iterator,
		// if these two iterators points to different vectors, throw invaild_iterator.
		int operator-(const iterator &rhs) const {
                        if(v != rhs.v) throw invalid_iterator();
                        else return pos - rhs.pos;
		}
		iterator operator+=(const int &n) {
                        return iterator(v, pos + n);
		}
		iterator operator-=(const int &n) {
                        return iterator(v, pos - n);
		}
		/**
		 * TODO iter++
		 */
                iterator operator++(int) {
                    iterator tmp = iterator(v, pos);
                    pos++;
                    return tmp;
                }
		/**
		 * TODO ++iter
		 */
                iterator& operator++() {
                    pos++;
                    return *this;
                }
		/**
		 * TODO iter--
		 */
                iterator operator--(int) {
                    iterator tmp = iterator(v, pos);
                    pos--;
                }
		/**
		 * TODO --iter
		 */
                iterator& operator--() {
                    pos--;
                    return *this;
                }
		/**
		 * TODO *it
		 */
                T& operator*() const{
                    return v -> data[pos];
                }
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
                bool operator==(const iterator &rhs) const {
                    if(v == rhs.v && pos = rhs.pos) return 1;
                    else return 0;
                }
                bool operator==(const const_iterator &rhs) const {
                    if(v == rhs.v && pos == rhs.pos) return 1;
                    else return 0;
                }
		/**
		 * some other operator for iterator.
		 */
                bool operator!=(const iterator &rhs) const {
                    if(v != rhs.v || pos != rhs.pos) return 1;
                    else return 0;
                }
                bool operator!=(const const_iterator &rhs) const {
                    if(v != rhs.v || pos != rhs.pos) return 1;
                    else return 0;
                }
	};
	/**
	 * TODO
	 * has same function as iterator, just for a const object.
	 */
	class const_iterator {
            friend class vector<T>;
            friend class iterator;
        private:
                /**
                 * TODO add data members
                 *   just add whatever you want.
                 */
                const vector<T> *v = nullptr;
                size_t pos = 0;
        public:
                const_iterator() = default;
                const_iterator(const vector *vec, size_t p):v(vec), pos(p){}
                const_iterator operator+(const int &n) const {
                        return const_iterator(v, pos + n);
                }
                const_iterator operator-(const int &n) const {
                        return const_iterator(v, pos - n);
                }
                int operator-(const const_iterator &rhs) const {
                        if(v != rhs.v) throw invalid_iterator();
                        else return pos - rhs.pos;
                }
                const_iterator operator+=(const int &n) {
                        return const_iterator(v, pos + n);
                }
                const_iterator operator-=(const int &n) {
                        return const_iterator(v, pos - n);
                }
                const_iterator operator++(int) {
                    const_iterator tmp = const_iterator(v, pos);
                    pos++;
                    return tmp;
                }
                const_iterator& operator++() {
                    pos++;
                    return *this;
                }
                const_iterator operator--(int) {
                    const_iterator tmp = const_iterator(v, pos);
                    pos--;
                }
                const_iterator& operator--() {
                    pos--;
                    return *this;
                }
                T& operator*() const{
                    return v -> data[pos];
                }
                bool operator==(const iterator &rhs) const {
                    if(v == rhs.v && pos = rhs.pos) return 1;
                    else return 0;
                }
                bool operator==(const const_iterator &rhs) const {
                    if(v == rhs.v && pos == rhs.pos) return 1;
                    else return 0;
                }
                bool operator!=(const iterator &rhs) const {
                    if(v != rhs.v || pos != rhs.pos) return 1;
                    else return 0;
                }
                bool operator!=(const const_iterator &rhs) const {
                    if(v != rhs.v || pos != rhs.pos) return 1;
                    else return 0;
                }
	};
	/**
	 * TODO Constructs
	 * Atleast three: default constructor, copy constructor and a constructor for std::vector
	 */
	vector() {}
        vector(const vector &other) {
            if(other.data == this -> data) return;
            else{
                if(data){
                    for(size_t i = 0; i < sz; ++i) data[i].~T();
                    free(data);
                }
                data = nullptr;
                if(other.data) data = (T*)malloc(sizeof(T) * other.cap);
                sz = other.sz; cap = other.cap;
                for(size_t i = 0 ; i < other.sz; ++i) new(data + i)T(other.data[i]);
                return;
            }
        }
        vector(const std::vector<T> &other){
            cap = other.capacity();
            sz = other.size();
            if(cap){
                data = (T*)malloc(sizeof(T) * cap);
                for(size_t i = 0; i < sz; ++i) new(data + i)T(other[i]);
            }
            return;
        }
	/**
	 * TODO Destructor
	 */
        ~vector() {
            if(data){
                for(size_t i = 0; i < sz; ++i) data[i].~T();
                free(data);
            }
            data = nullptr;
            sz = 0;
            cap = 0;
        }
	/**
	 * TODO Assignment operator
	 */
        vector &operator=(const vector &other) {
            if(this -> data == other.data) return *this;
            else{
                if(data){
                    for(size_t i = 0; i < sz; ++i) data[i].~T();
                    free(data);
                }
                data = nullptr;
                sz = other.sz;
                cap = other.cap;
                if(cap){
                    data = (T*)malloc(sizeof(T) * cap);
                    for(size_t i = 0; i < sz; ++i) new(data + i)T(other.data[i]);
                }
                return *this;
            }
        }
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 */
        T & at(const size_t &pos) {
            if(pos < 0 || pos >= sz) throw index_out_of_bound();
            else return data[pos];
        }
        const T & at(const size_t &pos) const {
            if(pos < 0 || pos >= sz) throw index_out_of_bound();
            else return data[pos];
        }
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 * !!! Pay attentions
	 *   In STL this operator does not check the boundary but I want you to do.
	 */
        T & operator[](const size_t &pos) {
            if(pos < 0 || pos >= sz) throw index_out_of_bound();
            else return data[pos];
        }
        const T & operator[](const size_t &pos) const {
            if(pos < 0 || pos >= sz) throw index_out_of_bound();
            else return data[pos];
        }
	/**
	 * access the first element.
	 * throw container_is_empty if size == 0
	 */
        const T & front() const {
            if(!sz) throw container_is_empty();
            else return data[0];
        }
	/**
	 * access the last element.
	 * throw container_is_empty if size == 0
	 */
        const T & back() const {
            if(!sz) throw container_is_empty();
            else return data[sz - 1];
        }
	/**
	 * returns an iterator to the beginning.
	 */
        iterator begin() {
            return iterator(this, 0);
        }
        const_iterator cbegin() const {
            return const_iterator(this, 0);
        }
	/**
	 * returns an iterator to the end.
	 */
        iterator end() {
            return iterator(this, sz);
        }
        const_iterator cend() const {
            return const_iterator(this, sz);
        }
	/**
	 * checks whether the container is empty
	 */
        bool empty() const {
            return sz;
        }
	/**
	 * returns the number of elements
	 */
        size_t size() const {
            return sz;
        }
	/**
	 * returns the number of elements that can be held in currently allocated storage.
	 */
        size_t capacity() const {
            return cap;
        }
	/**
	 * clears the contents
	 */
        void clear() {
            sz = 0;
        }
	/**
	 * inserts value before pos
	 * returns an iterator pointing to the inserted value.
	 */
        iterator insert(iterator pos, const T &value) {
            if(pos.pos > sz) throw invalid_iterator();
            if(sz == cap) doubleCap();
            new(data + sz)T(value);
            for(size_t i = sz - 1; i >= pos.pos; --i){
                if(i >= sz) break;
                data[i + 1] = data[i];
            }
            data[pos.pos] = T(value);
            ++sz;
            return iterator(this, pos.pos);
        }
	/**
	 * inserts value at index ind.
	 * after inserting, this->at(ind) == value is true
	 * returns an iterator pointing to the inserted value.
	 * throw index_out_of_bound if ind > size (in this situation ind can be size because after inserting the size will increase 1.)
	 */
        iterator insert(const size_t &ind, const T &value) {
            if(ind > sz) throw index_out_of_bound();
            if(sz == cap) doubleCap();
            new(data + sz)T(value);
            for(size_t i = sz - 1; i >= ind; --i){
                if(i >= sz) break;
                data[i + 1] = data[i];
            }
            data[ind] = value;
            ++sz;
            return iterator(this, ind);
        }
	/**
	 * removes the element at pos.
	 * return an iterator pointing to the following element.
	 * If the iterator pos refers the last element, the end() iterator is returned.
	 */
        iterator erase(iterator pos) {
            if(pos.pos >= sz) throw invalid_iterator();
            else{
                for(size_t i = pos.pos; i <= sz - 2; ++i) data[i] = data[i + 1];
                data[sz].~T();
                --sz;
                return iterator(this, pos.pos);
            }
        }
	/**
	 * removes the element with index ind.
	 * return an iterator pointing to the following element.
	 * throw index_out_of_bound if ind >= size
	 */
        iterator erase(const size_t &ind) {
            if(ind >= sz) throw index_out_of_bound();
            else{
                for(size_t i = ind; i <= sz - 2; ++i) data[i] = data[i + 1];
                data[sz].~T();
                --sz;
                return iterator(this, ind);
            }
        }
	/**
	 * adds an element to the end.
	 */
        void push_back(const T &value) {
            if(!data){
                data = (T*)malloc(sizeof(T));
                new(data)T(value);
                sz++;
                cap++;
                return;
            }
            else{
                if(sz == cap) doubleCap();
                new(data + (sz++))T(value);
                return;
            }
        }
	/**
	 * remove the last element from the end.
	 * throw container_is_empty if size() == 0
	 */
        void pop_back() {
            if(!sz) throw container_is_empty();
            else{
                data[sz - 1].~T();
                sz--;
            }
        }
};


}

#endif
