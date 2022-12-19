#ifndef INDEX_ITERATOR_HPP
#define INDEX_ITERATOR_HPP
/*
index_iterator.hpp
2014/11/29
psycommando@gmail.com
Description:
        A simple Standard Library compliant iterator over containers implementing the bracket operator, and size() method.
        The size() method of the owner container should provide constant complexity for maximum performance.
*/
#include <stdexcept>
#include <iterator>

namespace utils
{

    /*
        index_iterator
    */
    template<class _ContainerType> class index_iterator
    {
    public:
        typedef _ContainerType                   container_t;
        typedef std::random_access_iterator_tag  iterator_category;
        typedef typename container_t::value_type value_type;
        typedef ptrdiff_t                        difference_type;
        typedef value_type*                      pointer;
        typedef value_type&                      reference;
        typedef container_t*                     container_ptr_t;
        using my_t = index_iterator<container_t>;

        explicit index_iterator( container_ptr_t pcontainer, std::size_t index = 0 )noexcept
            :m_index(index), m_pContainer(pcontainer)
        {}

        index_iterator( const my_t& other )noexcept
            :m_index(other.m_index), m_pContainer(other.m_pContainer)
        {}

        ~index_iterator()noexcept = default;

        my_t& operator=( const my_t& other )
        {
            m_index      = other.m_index;
            m_pContainer = other.m_pContainer;
            return *this;
        }

        bool operator==(const index_iterator & iter)const noexcept { return m_index == iter.m_index; }
        bool operator!=(const index_iterator & iter)const noexcept { return !(*this == iter); }

        index_iterator & operator++()  
        { 
            //This is to avoid moving past the end
            if( m_pContainer->size() > m_index )
                ++m_index; 
            else
                throw std::out_of_range("Iterator out of bound!");
            return *this;
        }

        index_iterator operator++(int)
        {
            my_t temp(*this);
            operator++();
            return temp;
        }

        index_iterator & operator--()  
        { 
            //This is to avoid moving past the end
            if( m_index > 0 )
                --m_index; 
            else
                throw std::out_of_range("Iterator out of bound!");
            return *this;
        }

        index_iterator operator--(int)
        {
            my_t temp(*this);
            operator--();
            return temp;
        }


        index_iterator & operator+=(difference_type n)
        { 
            if ( n >= 0 )
            {
                if( m_pContainer->size() >= (m_index + n) )
                    m_index += n;
                else
                    throw std::out_of_range("Iterator out of bound!");
            }
            else
            {
                if( (m_index - n) >= 0 )
                    m_index -= n;
                else
                    throw std::out_of_range("Iterator out of bound!");
            }
            return *this;
        }

        index_iterator operator-=(difference_type n )
        {
            return (*this += -n);
        }

        index_iterator operator+(difference_type n )const
        {
            index_iterator temp = *this;
            return temp += n;
        }

        index_iterator operator-(difference_type n )const
        {
            index_iterator temp = *this;
            return temp -= n;
        }

        difference_type operator-(const index_iterator & otherit )const
        {
            return this->m_index - otherit.m_index;
        }

        reference operator[](std::size_t n )const
        {
            return *((*this)+n);
        }

        bool operator<(const index_iterator & otherit )const
        {
            return m_index < otherit.m_index;
        }

        bool operator>(const index_iterator & otherit )const
        {
            return otherit < *this;
        }

        bool operator>=(const index_iterator & otherit )const
        {
            return !(*this < otherit);
        }

        bool operator<=(const index_iterator & otherit )const
        {
            return !(*this > otherit);
        }

        reference       operator*()        { return (*m_pContainer)[m_index]; }
        const reference operator*() const  { return (*m_pContainer)[m_index]; }
        pointer         operator->()       { return const_cast<pointer>(& ((*m_pContainer)[m_index])); }
        const pointer   operator->() const { return &((*m_pContainer)[m_index]); }

//    protected:
        std::size_t     m_index;
        container_ptr_t m_pContainer;
    };

    /*
        const_index_iterator
            The same thing as above, but constant
    */
    template<class _CONTAINER_T> class const_index_iterator : public index_iterator<_CONTAINER_T>
    {
    public:
        using parent_t = index_iterator<_CONTAINER_T>;
        using my_t = const_index_iterator<_CONTAINER_T>;
        using container_ptr_t = typename parent_t::container_ptr_t;
        using container_t = typename parent_t::container_t;
        using parent_t::index_iterator;

        const_index_iterator<container_t>& operator=( const const_index_iterator<container_t> & other )
        {
            this->m_index = other.m_index;
            this->m_pContainer = other.m_pContainer;
            return *this;
        }
    };
};

#endif