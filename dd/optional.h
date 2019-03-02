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

#include <stdexcept>

namespace parakram {

  template<typename TValue>
  class Optional {
    public:
      Optional();
      Optional(const TValue & value);
      Optional(const Optional<TValue> & that);
      Optional<TValue> & operator = (const Optional<TValue> & that);
      bool isPresent() const;
      const TValue & get();
      const TValue & get() const;
      void reset();
      void reset(const TValue & value);
      ~Optional();
    private:
      char m_data[sizeof(TValue) + 1];

      TValue * dptr() {
        return reinterpret_cast<TValue *>(m_data + 1);
      }
      const TValue * dptr() const {
        return reinterpret_cast<const TValue *>(m_data + 1);
      }
  };

  template<typename TValue>
    Optional<TValue>::Optional()
    {
      m_data[0] = 0;
    }

  template<typename TValue>
    Optional<TValue>::Optional(const TValue & value)
    {
      m_data[0] = 1;
      new (dptr())TValue(value);
    }

  template <typename TValue>
    Optional<TValue>::Optional(const Optional<TValue> & that)
    {
      m_data[0] = that.m_data[0];
      if (m_data[0])
        new (dptr())TValue(*that.dptr());
    }

  template <typename TValue>
    Optional<TValue> & Optional<TValue>::operator = (const Optional<TValue> & that)
    {
      reset();
      m_data[0] = that.m_data[0];
      if (m_data[0])
        new (dptr())TValue(*that.dptr());
    }

  template<typename TValue>
    bool Optional<TValue>::isPresent() const
    {
      return static_cast<bool>(m_data[0]);
    }

  template<typename TValue>
    const TValue & Optional<TValue>::get()
    {
      if (!isPresent())
        throw std::runtime_error("Attempted get on empty Optional");
      else
        return *dptr();
    }

  template<typename TValue>
    const TValue & Optional<TValue>::get() const
    {
      if (!isPresent())
        throw std::runtime_error("Attempted const get on empty Optional");
      else
        return *dptr();
    }

  template<typename TValue>
    void Optional<TValue>::reset()
    {
      if (isPresent())
      {
        m_data[0] = 0;
        dptr()->~TValue();
      }
    }

  template<typename TValue>
    void Optional<TValue>::reset(const TValue & value)
  {
    if (isPresent())
    {
      reset();
    }
    m_data[0] = 1;
    new (dptr())TValue(value);
  }

  template<typename TValue>
  Optional<TValue>::~Optional()
  {
    reset();
  }


} // end namespace parakram


