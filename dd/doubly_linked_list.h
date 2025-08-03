/*

Copyright 2025 Parakram Majumdar

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

/**
 * std::list does not allow re-inserting a previously invalidated iterator.
 * So, we define out own Doubly Linked List class,
 *   that exposes a individual node shared_ptrs as iterators,
 *   and thus allows erasing and re-inserting the same node object.
 */

#pragma once

#include <memory>

namespace parakram {

    // Forward declaration of DlList
    template<typename T>
    class DlList;

    // Node for the doubly linked list.
    // Acts as an iterator for the list.
    template<typename T>
    struct DlListNode
    {
        using Ptr = std::shared_ptr<DlListNode<T> >;

        T value;
        Ptr next;
        Ptr prev;

        T& operator*() { return value; };
        T const& operator*() const { return value; };

        DlListNode(T v): value(std::move(v)), next(nullptr), prev(nullptr) { }

    };


    // Doubly linked list class.
    template<typename T>
    class DlList
    {
    public:
        using NodePtr = typename DlListNode<T>::Ptr;

        DlList() : head(nullptr), tail(nullptr) {}

        NodePtr push_front(T value);
        NodePtr push_back(T value);

        // erases the front and returns the node iterator
        // returns nullptr if the list is empty
        typename DlListNode<T>::Ptr pop_front();

        // erases the back and returns the node iterator
        // returns nullptr if the list is empty
        typename DlListNode<T>::Ptr pop_back();
        void clear();
        bool empty() const { return head == nullptr; }

        // erases an iterator and returns the next iterator
        NodePtr get_next_and_erase(NodePtr it);

        // re-inserts a to_be_inserted node before the next_position node
        void reinsert_node_before_node(NodePtr to_be_inserted, NodePtr next_position);

        NodePtr begin() const { return head; }
        NodePtr end() const { return nullptr; }

    private:
        NodePtr head;
        NodePtr tail;
    };


    template<typename T>
    typename DlList<T>::NodePtr DlList<T>::push_front(T value)
    {
        NodePtr newNode = std::make_shared<DlListNode<T>>(std::move(value));
        if (!head) {
            head = tail = newNode;
        } else {
            newNode->next = head;
            head->prev = newNode;
            head = newNode;
        }
        return newNode;
    }

    template<typename T>
    typename DlList<T>::NodePtr DlList<T>::push_back(T value)
    {
        NodePtr newNode = std::make_shared<DlListNode<T>>(std::move(value));
        if (!tail) {
            head = tail = newNode;
        } else {
            newNode->prev = tail;
            tail->next = newNode;
            tail = newNode;
        }
        return newNode;
    }

    template<typename T>
    typename DlList<T>::NodePtr DlList<T>::pop_front()
    {
        if (!head) return nullptr; // List is empty
        if (head == tail) {
            head.reset();
            tail.reset();
            return nullptr;
        } else {
            auto ret = head;
            head = head->next;
            if (head) {
                head->prev.reset();
            } else {
                tail.reset();
            }
            ret->next.reset();
            ret->prev.reset();
            return ret;
        }
    }

    template<typename T>
    typename DlList<T>::NodePtr DlList<T>::pop_back()
    {
        if (!tail) return nullptr; // List is empty
        if (head == tail) {
            head.reset();
            tail.reset();
            return nullptr;
        } else {
            auto ret = tail;
            tail = tail->prev;
            if (tail) {
                tail->next.reset();
            } else {
                head.reset();
            }
            ret->next.reset();
            ret->prev.reset();
            return ret;
        }
    }

    template<typename T>
    void DlList<T>::clear()
    {
        head.reset();
        tail.reset();
    }

    template<typename T>
    typename DlList<T>::NodePtr DlList<T>::get_next_and_erase(NodePtr it)
    {
        if (!it) return nullptr; // Nothing to erase
        if (it == head) {
            auto ret = it->next; // Save the next iterator
            pop_front();
            return ret;
        } else if (it == tail) {
            pop_back();
            return nullptr;
        } else {
            auto ret = it->next; // Save the next iterator
            if (it->prev) {
                it->prev->next = it->next;
            }
            if (it->next) {
                it->next->prev = it->prev;
            }
            it->next.reset();
            it->prev.reset();
            return ret;
        }
    }

    template<typename T>
    void DlList<T>::reinsert_node_before_node(NodePtr to_be_inserted, NodePtr next_position)
    {
        if (!to_be_inserted) // Nothing to reinsert
        {
             return;
        }
        else if (!next_position)
        {
            // If next_position is null, push to the back
            if (tail)
            {
                to_be_inserted->prev = tail;
                tail->next = to_be_inserted;
                tail = to_be_inserted;
            }
            else
            {
                head = tail = to_be_inserted;
            }
            return;
        }
        else
        {
            // Insert before next_position
            to_be_inserted->next = next_position;
            to_be_inserted->prev = next_position->prev;
            if (next_position->prev) {
                next_position->prev->next = to_be_inserted;
            } else {
                if (head != next_position) {
                    throw std::runtime_error("Something is wrong: iter->prev is null but iter is not head");
                }
                head = to_be_inserted; // Update head if inserting at the front
            }
            next_position->prev = to_be_inserted;
        }
    }




} // end namespace parakram