#ifndef __DES_HPP__
#define __DES_HPP__

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include "constants.h"

class Key {
  uint64_t m_key;

private:
  void from_string( const std::string & a_key );
  std::string to_string() const;

public:
  Key();
  Key( const uint64_t m_key );
  Key( const std::string & a_key );

  uint64_t operator () () const;

  friend std::ostream & operator << ( std::ostream & a_out, const Key & a_key );
  friend std::istream & operator >> ( std::istream & a_in, Key & a_key );

};

class DES {
  uint64_t m_keys[ROUNDS];

  private:
    uint64_t f( uint64_t a_part, const uint32_t a_round ) const;
    void output_last_byte( const uint64_t a_block, std::ostream & a_out ) const;

  public:
    DES( const Key & a_key );
    uint64_t encrypt_block( uint64_t a_block ) const;
    uint64_t decrypt_block( uint64_t a_block ) const;
    void encrypt( std::istream & a_in, std::ostream & a_out ) const;
    void decrypt( std::istream & a_in, std::ostream & a_out ) const;
};

#endif
