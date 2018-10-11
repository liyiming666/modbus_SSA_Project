/*
 * map.h
 *
 *  Created on: Sep 7, 2018
 *      Author: wenhan
 */

#ifndef MAP_H_
#define MAP_H_

#include <string>
#include <map>
#include <boost/thread/mutex.hpp>
#include <mutex>

#include "define.h"

using namespace std;

template<class KEY, class VALUE>
class Map : public std::map<KEY, VALUE>{
public:
        void insert(const KEY& key, const VALUE& value) {
                _mutex.lock();
                std::map<KEY, VALUE>::insert(std::pair<KEY, VALUE>(key, value));
                _mutex.unlock();
        }

        void erase(const KEY& key) {
                _mutex.lock();
                std::map<KEY, VALUE>::erase(key);
                _mutex.unlock();
        }

        void erase(const KEY& key, const VALUE& value) {
                _mutex.lock();
                VALUE& v = std::map<KEY, VALUE>::operator[](key);
                if (v == value) {
                        std::map<KEY, VALUE>::erase(key);
                }
                _mutex.unlock();
        }

        VALUE& operator[](const KEY& key) {
                _mutex.lock();
                VALUE& val =  std::map<KEY, VALUE>::operator[](key);
                _mutex.unlock();
                return val;
        }

        u32 size() {
                _mutex.lock();
                u32 val_size = std::map<KEY, VALUE>::size();
                _mutex.unlock();

                return val_size;
        }

public:
        boost::mutex _mutex;
};

#endif /* MAP_H_ */
