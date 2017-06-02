//
// Created by Phil Nash on 02/06/2017.
//
#include <cassert>
#include <ostream>
#include <sstream>
#include <vector>

#ifndef TEXTFLOW_HPP_INCLUDED
#define TEXTFLOW_HPP_INCLUDED

namespace TextFlow {

    inline auto isWhitespace( char c ) -> bool {
        static std::string chars = " \t\n\r";
        return chars.find( c ) != std::string::npos;
    }
    inline auto isBreakableBefore( char c ) -> bool {
        static std::string chars = "[({<|";
        return chars.find( c ) != std::string::npos;
    }
    inline auto isBreakableAfter( char c ) -> bool {
        static std::string chars = "])}>.,:;*+-=&/\\";
        return chars.find( c ) != std::string::npos;
    }

    class Columns;

    class Column {
        std::vector<std::string> m_strings;
        size_t m_width = 80;
        size_t m_indent = 0;
        size_t m_initialIndent = std::string::npos;

    public:
        class Iterator {
            friend Column;

            Column const& m_column;
            size_t m_stringIndex = 0;
            size_t m_pos = 0;

            size_t m_len;
            bool m_suffix = false;

            Iterator( Column const& column, size_t stringIndex )
            :   m_column( column ),
                m_stringIndex( stringIndex )
            {}

            auto line() const -> std::string const& { return m_column.m_strings[m_stringIndex]; }

            auto isBoundary( size_t at ) const -> bool {
                assert( at > 0 );
                assert( at <= line().size() );

                return at == line().size() ||
                       ( isWhitespace( line()[at] ) && !isWhitespace( line()[at-1] ) ) ||
                       isBreakableBefore( line()[at] ) ||
                       isBreakableAfter( line()[at-1] );
            }

            void calcLength() {
                assert( m_stringIndex < m_column.m_strings.size() );

                m_suffix = false;
                auto width = m_column.m_width-indent();
                if( line().size() < m_pos + width ) {
                    m_len = line().size() - m_pos;
                }
                else {
                    size_t len = width;
                    while (len > 0 && !isBoundary(m_pos + len))
                        --len;
                    while (len > 0 && isWhitespace( line()[m_pos + len - 1] ))
                        --len;

                    if (len > 0) {
                        m_len = len;
                    } else {
                        m_suffix = true;
                        m_len = width - 1;
                    }
                }
            }

            auto indent() const -> size_t {
                auto initial = m_pos == 0 && m_stringIndex == 0 ? m_column.m_initialIndent : std::string::npos;
                return initial == std::string::npos ? m_column.m_indent : initial;
            }

            auto addIndentAndSuffix(std::string const &plain) const -> std::string {
                return std::string( indent(), ' ' ) + (m_suffix ? plain + "-" : plain);
            }

        public:
            Iterator( Column const& column ) : m_column( column ) {
                assert( m_column.m_width > m_column.m_indent );
                assert( m_column.m_initialIndent == std::string::npos || m_column.m_width > m_column.m_initialIndent );
                calcLength();
                if( m_len == 0 )
                    m_stringIndex++; // Empty string
            }

            auto operator *() const -> std::string {
                assert( m_stringIndex < m_column.m_strings.size() );
                assert( m_pos < line().size() );
                if( m_pos + m_column.m_width < line().size() )
                    return addIndentAndSuffix(line().substr(m_pos, m_len));
                else
                    return addIndentAndSuffix(line().substr(m_pos));
            }

            auto operator ++() -> Iterator& {
                m_pos += m_len;
                while( m_pos < line().size() && isWhitespace( line()[m_pos] ) )
                    ++m_pos;

                if( m_pos == line().size() ) {
                    m_pos = 0;
                    ++m_stringIndex;
                }
                if( m_stringIndex < m_column.m_strings.size() )
                    calcLength();
                return *this;
            }
            auto operator ++(int) -> Iterator {
                Iterator prev( *this );
                operator++();
                return prev;
            }

            auto operator ==( Iterator const& other ) const -> bool {
                return
                    m_pos == other.m_pos &&
                    m_stringIndex == other.m_stringIndex &&
                    &m_column == &other.m_column;
            }
            auto operator !=( Iterator const& other ) const -> bool {
                return !operator==( other );
            }
        };

        Column( std::string const& text ) { m_strings.push_back( text ); }

        auto width( size_t newWidth ) -> Column& {
            assert( newWidth > 0 );
            m_width = newWidth;
            return *this;
        }
        auto indent( size_t newIndent ) -> Column& {
            m_indent = newIndent;
            return *this;
        }
        auto initialIndent( size_t newIndent ) -> Column& {
            m_initialIndent = newIndent;
            return *this;
        }

        auto width() const -> size_t { return m_width; }
        auto begin() const -> Iterator { return Iterator( *this ); }
        auto end() const -> Iterator { return Iterator( *this, m_strings.size() ); }

        inline friend std::ostream& operator << ( std::ostream& os, Column const& col ) {
            bool first = true;
            for( auto line : col ) {
                if( first )
                    first = false;
                else
                    os << "\n";
                os <<  line;
            }
            return os;
        }

        auto operator + ( Column const& other ) -> Columns;

        auto toString() const -> std::string {
            std::ostringstream oss;
            oss << *this;
            return oss.str();
        }
    };

    class Spacer : public Column {

    public:
        Spacer( size_t spaceWidth ) : Column( "" ) {
            width( spaceWidth );
        }
    };

    class Columns {
        std::vector<Column> m_columns;

    public:

        class Iterator {
            friend Columns;
            struct EndTag {};

            std::vector<Column> const& m_columns;
            std::vector<Column::Iterator> m_iterators;
            size_t m_activeIterators;

            Iterator( Columns const& columns, EndTag )
            :   m_columns( columns.m_columns ),
                m_activeIterators( 0 )
            {
                m_iterators.reserve( m_columns.size() );

                for( auto const& col : m_columns )
                    m_iterators.push_back( col.end() );
            }

        public:
            Iterator( Columns const& columns )
            :   m_columns( columns.m_columns ),
                m_activeIterators( m_columns.size() )
            {
                m_iterators.reserve( m_columns.size() );

                for( auto const& col : m_columns )
                    m_iterators.push_back( col.begin() );
            }

            auto operator ==( Iterator const& other ) const -> bool {
                return m_iterators == other.m_iterators;
            }
            auto operator !=( Iterator const& other ) const -> bool {
                return m_iterators != other.m_iterators;
            }
            auto operator *() const -> std::string {
                std::string row, padding;

                for( size_t i = 0; i < m_columns.size(); ++i ) {
                    auto width = m_columns[i].width();
                    if( m_iterators[i] != m_columns[i].end() ) {
                        std::string col = *m_iterators[i];
                        row += padding + col;
                        if( col.size() < width )
                            padding = std::string( width - col.size(), ' ' );
                        else
                            padding = "";
                    }
                    else {
                        padding += std::string( width, ' ' );
                    }
                }
                return row;
            }
            auto operator ++() -> Iterator& {
                for( size_t i = 0; i < m_columns.size(); ++i ) {
                    if (m_iterators[i] != m_columns[i].end())
                        ++m_iterators[i];
                }
                return *this;
            }
            auto operator ++(int) -> Iterator {
                Iterator prev( *this );
                operator++();
                return prev;
            }

        };
        auto begin() const -> Iterator { return Iterator( *this ); }
        auto end() const -> Iterator { return Iterator( *this, Iterator::EndTag() ); }

        auto operator += ( Column const& col ) -> Columns& {
            m_columns.push_back( col );
            return *this;
        }
        auto operator + ( Column const& col ) -> Columns {
            Columns combined = *this;
            combined += col;
            return combined;
        }

        inline friend std::ostream& operator << ( std::ostream& os, Columns const& cols ) {

            bool first = true;
            for( auto line : cols ) {
                if( first )
                    first = false;
                else
                    os << "\n";
                os << line;
            }
            return os;
        }

        auto toString() const -> std::string {
            std::ostringstream oss;
            oss << *this;
            return oss.str();
        }
    };

    auto Column::operator + ( Column const& other ) -> Columns {
        Columns cols;
        cols += *this;
        cols += other;
        return cols;
    }
}

#endif // TEXTFLOW_HPP_INCLUDED
