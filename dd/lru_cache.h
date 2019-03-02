/*

Copyright 2019 Parakram Majumdar

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/




#pragma once

#include "optional.h"
#include <unordered_map>
#include <list>

namespace parakram {

  template<typename TKey, typename TValue>
  class LruCache
  {
    public:

      LruCache(int capacity);

      bool insert(const TKey & key, const TValue & value);
      Optional<TValue> tryGet(const TKey & key);

    private:
      typedef unsigned long long KeyId;
      typedef std::list<KeyId>::const_iterator PriorityQueueIterator;
      typedef std::unordered_map<TKey, KeyId> KeyStore;
      typedef typename KeyStore::const_iterator KeyStoreIterator;
      typedef std::unordered_map<KeyId, KeyStoreIterator> KeyStoreMap;
      int m_capacity;
      KeyId m_largestKey;
      KeyStore m_keyStore;
      KeyStoreMap m_keyStoreMap;
      std::unordered_map<KeyId, TValue> m_valueMap;
      std::list<KeyId> m_priorityQueue;
      std::unordered_map<KeyId, PriorityQueueIterator> m_priorityMap;
  };


  template<typename TKey, typename TValue>
    LruCache<TKey, TValue>::LruCache(int capacity) :
      m_capacity(capacity),
      m_largestKey(0),
      m_keyStore(),
      m_keyStoreMap(),
      m_valueMap(),
      m_priorityQueue(),
      m_priorityMap()
  { }


  template<typename TKey, typename TValue>
    bool LruCache<TKey, TValue>::insert(const TKey & key, const TValue & value)
    {
      // return false if the key already exists
      if (m_keyStore.find(key) != m_keyStore.end())
        return false;

      // get the new key id
      KeyId keyId = ++m_largestKey;
      m_keyStore[key] = keyId;
      m_keyStoreMap[keyId] = m_keyStore.find(key);
      
      // insert the value
      m_valueMap[keyId] = value;

      // push to the top of the priority queue
      m_priorityQueue.push_front(keyId);
      m_priorityMap[keyId] = m_priorityQueue.begin();

      // forget least recently used element
      while (m_keyStore.size() > m_capacity && !m_keyStore.empty())
      {
        KeyId lruId = m_priorityQueue.back();
        m_priorityMap.erase(lruId);
        m_priorityQueue.pop_back();
        m_valueMap.erase(lruId);
        auto ksmit = m_keyStoreMap.find(lruId);
        auto ksit = ksmit->second;
        m_keyStoreMap.erase(ksmit);
        m_keyStore.erase(ksit);
      }
    }

  template<typename TKey, typename TValue>
    Optional<TValue> LruCache<TKey, TValue>::tryGet(const TKey & key)
    {
      auto keyIdIter = m_keyStore.find(key);
      KeyId keyId;
      if (m_keyStore.end() == keyIdIter)
        return Optional<TValue>();
      else
        keyId = keyIdIter->second;

     auto pqiter = m_priorityMap.find(keyId);
     m_priorityQueue.erase(pqiter->second);
     m_priorityQueue.push_front(keyId);
     pqiter->second = m_priorityQueue.begin();

     return m_valueMap[keyId];
    }



} // end namespace parakram
