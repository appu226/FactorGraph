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

#include <vector>
#include <memory>
#include <stdexcept>

namespace parakram {

  // ***** MaxHeap *****
  // ****** class ******
  // A max heap data structure.
  // Every element has a priority.
  // The element with the highest priority 
  //   can be fetched using the top function,
  //   and can be removed using the pop function.
  // While inserting elements, it is possible to
  //   get a pointer to the element,
  //   which can be used to change the priority.
  // Removing an element does not affect pointers
  //   to other elements, but the pionter to 
  //   that element gets invalidated.
  // Template arguments:
  //   TElement: the type of the element that is stored.
  //   TPriority: the type of the priority value associated with each element.
  //   TCompare: a comparision type for TPriority, defaulting to std::less<TPriority>
  template<typename TElement, typename TPriority, typename TCompare = std::less<TPriority> >
    class MaxHeap
    {
      private:
        // ************ DataCell ************
        // ***** private internal class *****
        // Used for storing the elements and their priorities.
        // Also contains the position in the max heap,
        //   which is used while adjusting the priority of the element.
        struct DataCell {
          int position;
          TPriority priority;
          TElement element;
          DataCell(int pos, const TPriority & p, const TElement & e):
            position(pos),
            priority(p),
            element(e)
          { }
        };
      public:
        typedef std::shared_ptr<const DataCell> DataCellCptr;
        typedef std::shared_ptr<DataCell> DataCellPtr;



        // *********** MaxHeap *********
        // ***** empty constructor *****
        MaxHeap() { }



        // ******* MaxHeap *******
        // ***** constructor *****
        // "Builds" a max heap from an iterable collection of
        //   pairs of elements and priorities.
        // Template arguments:
        //   TIterable: the type of the iterable collection
        //              of pairs of elements and priorities.
        // Input arguments:
        //   iterable: An iterable collection.
        //             The iterable must support std::cbegin(iterable), 
        //               and std::cend(iterable).
        //             The iterator it returned by cbegin
        //               must be incrementable via ++it,
        //               and must eventually be comparable
        //               to the result of cend.
        //             The iterator it must also support
        //               it->first, which should give the elements
        //               and it->second, giving the corresponding priorities.
        template<typename TIterable>
        MaxHeap(const TIterable & iterable)
        {
          auto itEnd = std::cend(iterable);
          for ( auto it = std::cbegin(iterable); it != itEnd; ++it )
          {
            auto dataCell = std::make_shared<DataCell>( m_data.size(), it->second, it->first );
            m_data.push_back(dataCell);
          }
          for ( int i = m_data.size() - 1; i >= 0; --i )
            siftDown(i);
        }



        // ****** insert ******
        // ***** function *****
        // Inserts a new element into the max heap, along with it's priority.
        // Input arguments:
        //   element: the element to be inserted
        //   priority: the corresponding priority of the element
        // Output value:
        //   A pointer to the storage of the element, which can be used
        //     to remove the elment, or to modify its priority
        // See also:
        //  remove
        //  updatePriority
        DataCellCptr insert(const TElement & element, const TPriority & priority);



        // ****** remove ******
        // ***** function *****
        // Removes an element from the heap
        // Input argument:
        //   dataCell: a pointer to the element, obtained while inserting the element
        // See also:
        //   insert
        void remove(const DataCellCptr & dataCell);



        // ***** updatePriority *****
        // ******** function ********
        // Updates the priority of an element.
        // Input arguments:
        //   dataCell: a pointer to the element, obtained while
        //             inserting the element using the insert function
        //   newPriority: the new priority of the element
        // See also:
        //   insert
        void updatePriority(const DataCellCptr & dataCell, const TPriority & newPriority);


        // ******* top ********
        // ***** function *****
        // Gets the element with the highest priority.
        // Output value:
        //   A const reference to the element with the highest priority
        const TElement & top() const;


        // ******* pop ********
        // ***** function *****
        // Removes the elmeent with the highest priority.
        void pop();



        // ******* size *******
        // ***** function *****
        // Returns the number of elements currently stored in the data structure.
        int size() const { return m_data.size(); }



      private:

        std::vector<DataCellPtr> m_data;

        void swap(int i, int j);
        void siftDown(int i);
        void siftUp(int i);
        void checkDataCell(const DataCell & dataCell) const;
        void checkPosition(int pos) const;
        static int left( int i ) { return (i * 2) + 1; }
        static int right( int i ) { return (i * 2) + 2; }
        static int parent( int i ) { return (i - 1) / 2; }
        bool hasLeft( int i ) const { return left( i ) < m_data.size(); }
        bool hasRight( int i ) const { return right( i ) < m_data.size(); }
    };


  // ***** MaxHeap::insert *****
  template<typename TElement, typename TPriority, typename TCompare>
    typename MaxHeap<TElement, TPriority, TCompare>::DataCellCptr
    MaxHeap<TElement, TPriority, TCompare>::insert(
        const TElement & element,
        const TPriority & priority )
    {
      auto dc = std::make_shared<DataCell>( m_data.size(), priority, element );
      m_data.push_back( dc );
      siftUp(m_data.size() - 1);
      return dc;
    }


  // ***** MaxHeap::remove *****
  template<typename TElement, typename TPriority, typename TCompare>
    void 
    MaxHeap<TElement, TPriority, TCompare>::remove( 
        const typename MaxHeap<TElement, TPriority, TCompare>::DataCellCptr & dataCell )
    {
      if( dataCell->position == m_data.size() - 1 )
        m_data.pop_back();
      else
      {
        checkDataCell( *dataCell );
        int lastIdx = m_data.size() - 1;
        int dcIdx = dataCell->position;
        swap(dcIdx, lastIdx);
        m_data.pop_back();
        siftDown( dcIdx );
      }
    }


  // ***** MaxHeap::updatePriority *****
  template<typename TElement, typename TPriority, typename TCompare>
    void
    MaxHeap<TElement, TPriority, TCompare>::updatePriority(
        const DataCellCptr & dataCell, 
        const TPriority & newPriority)
    {
      checkDataCell( *dataCell );
      TCompare lt;
      TPriority oldPriority = dataCell->priority;
      m_data[dataCell->position]->priority = newPriority;
      if ( lt(oldPriority, newPriority) )
      {
        // oldPriority < newPriority
        // higher priority things go up
        // so the data cell needs to be sifted up
        siftUp( dataCell->position );
      } else if ( lt(newPriority, oldPriority) )
      {
        siftDown( dataCell->position );
      }
    }


  // ***** MaxHeap::top *****
  template<typename TElement, typename TPriority, typename TCompare>
    const TElement & 
    MaxHeap<TElement, TPriority, TCompare>::top() const
    {
      if ( m_data.empty() )
      {
        throw std::runtime_error("Cannot get top from empty MaxHeap");
      }
      return m_data[0]->element;
    }


  // ***** MaxHeap::pop *****
  template<typename TElement, typename TPriority, typename TCompare>
    void
    MaxHeap<TElement, TPriority, TCompare>::pop()
    {
      if ( m_data.empty() )
        throw std::runtime_error("Cannot remove top from empty MaxHeap");
      remove(m_data[0]);
    }


  // ***** MaxHeap::swap *****
  template<typename TElement, typename TPriority, typename TCompare>
    void
    MaxHeap<TElement, TPriority, TCompare>::swap(int i, int j)
    {
      checkPosition(i);
      checkPosition(j);
      DataCellPtr idc = m_data[i], jdc = m_data[j];
      idc->position = j;
      jdc->position = i;
      m_data[i] = jdc;
      m_data[j] = idc;
    }


  // ***** MaxHEap::siftDown *****
  template<typename TElement, typename TPriority, typename TCompare>
    void
    MaxHeap<TElement, TPriority, TCompare>::siftDown(int i)
    {
      checkPosition(i);
      const TPriority * maxPriority = & m_data[ i ]->priority;
      int newIndex = i;
      TCompare lt;
      if( hasLeft( i ) )
      {
        const auto & leftPriority = m_data[ left(i) ]->priority;
        if( lt( *maxPriority, leftPriority ) )
        {
          newIndex = left(i);
          maxPriority = &leftPriority;
        }
      }
      if( hasRight( i ) )
      {
        const auto & rightPriority = m_data[ right(i) ]->priority;
        if( lt( *maxPriority, rightPriority ) )
        {
          newIndex = right(i);
          // not needed, because we never use maxPriority again
          // maxPriority = &rightPriority;
        }
      }
      if( newIndex != i )
      {
        swap( i, newIndex );
        siftDown( newIndex );
      }
    }


  // ***** MaxHeap::siftUp *****
  template<typename TElement, typename TPriority, typename TCompare>
    void
    MaxHeap<TElement, TPriority, TCompare>::siftUp(int i)
    {
      checkPosition(i);
      TCompare lt;
      if( i == 0 )
        return;
      TPriority * parentPriority = & m_data[ parent(i) ]->priority;
      if ( lt( *parentPriority, m_data[i]->priority ) )
      {
        swap( i, parent(i) );
        siftUp( parent(i) );
      }
    }


  // ***** MaxHeap::checkPosition *****
  template<typename TElement, typename TPriority, typename TCompare>
    void
    MaxHeap<TElement, TPriority, TCompare>::checkPosition(int pos) const
    {
      if( pos < 0 ) 
        throw std::runtime_error("Invalid position: position cannot be less than zero");
      if( pos >= m_data.size() ) 
        throw std::runtime_error("Invalid position: position must be less than size");
      if( m_data[pos]->position != pos )
        throw std::runtime_error("Invalid position: data cell has inconsistent position");
    }


  // ***** MaxHeap::checkDataCell *****
  template<typename TElement, typename TPriority, typename TCompare>
    void
    MaxHeap<TElement, TPriority, TCompare>::checkDataCell(const DataCell & dataCell) const
  {
    int pos = dataCell.position;
    checkPosition(pos);
    if( &dataCell != m_data[pos].get() )
      throw std::runtime_error("Invalid DataCell: DataCell does not belong to this MaxHea");
  }

} // end namespace parakram
