//
//  safe_mem_reader.hpp
//  blocksci_parser
//
//  Created by Harry Kalodner on 9/26/17.
//

#ifndef safe_mem_reader_hpp
#define safe_mem_reader_hpp

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem/path.hpp>

#include <stdio.h>

class SafeMemReader {
public:
    typedef boost::iostreams::mapped_file_source::size_type size_type;
    
    explicit SafeMemReader(const boost::filesystem::path &path) : fileMap{path, boost::iostreams::mapped_file::readonly} {
        begin = fileMap.begin();
        end = fileMap.end();
        pos = begin;
    }
    
    bool has(size_type n) {
        return pos + n <= end;
    }
    
    template<typename Type>
    Type readNext() {
        auto size = sizeof(Type);
        if (!has(size)) {
            throw std::out_of_range("Tried to read past end of file");
        }
        Type val;
        memcpy(&val, pos, size);
        pos += size;
        return val;
    }
    
    // reads a variable length integer.
    // See the documentation from here:  https://en.bitcoin.it/wiki/Protocol_specification#Variable_length_integer
    uint32_t readVariableLengthInteger() {
        auto v = readNext<uint8_t>();
        
        try {
            if ( v < 0xFD ) { // If it's less than 0xFD use this value as the unsigned integer
                return static_cast<uint32_t>(v);
            } else if (v == 0xFD) {
                return static_cast<uint32_t>(readNext<uint16_t>());
            } else if (v == 0xFE) {
                return readNext<uint32_t>();
            } else {
                return static_cast<uint32_t>(readNext<uint64_t>()); // TODO: maybe we should not support this here, we lose data
            }
        } catch(...) {
            rewind(sizeof(uint8_t));
            throw;
        }
        
    }
    
    void advance(size_type n) {
        if (!has(n)) {
            throw std::out_of_range("Tried to advance past end of file");
        }
        pos += n;
    }
    
    void rewind(size_type n) {
        if (pos < begin + n) {
            throw std::out_of_range("Tried to rewind past start of file");
        }
        pos -= n;
    }
    
    void reset() {
        pos = begin;
    }
    
    void reset(size_type n) {
        if (begin + n > end) {
            throw std::out_of_range("Tried to reset out of file");
        }
        pos = begin + n;
    }
    
    size_type offset() {
        return pos - begin;
    }
    
    boost::iostreams::mapped_file_source::iterator unsafePos() {
        return pos;
    }
    
protected:
    boost::iostreams::mapped_file_source fileMap;
    boost::iostreams::mapped_file_source::iterator pos;
    boost::iostreams::mapped_file_source::iterator begin;
    boost::iostreams::mapped_file_source::iterator end;
};

#endif /* safe_mem_reader_hpp */
