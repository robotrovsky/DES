#include <iostream>
#include <cassert>
#include <sstream>
#include <cstring>
#include <cstdint>

#include "des.h"
#include "des_constants.h"

// http://page.math.tu-berlin.de/~kant/teaching/hess/krypto-ws2006/des.htm

namespace
{

uint32_t hex_value( char c )
{
  assert( isxdigit(c) );

  // A-F
  c = tolower(c);

  return isdigit(c) ? (c - '0') : (c - 'a' + 10);
}

uint64_t from_hex( const std::string & a_key )
{
  assert( a_key.size() == KEY_LEN );

  uint64_t res = 0;

  for( size_t i = 0; i < KEY_LEN; i++ )
  {
    const char c = a_key[i];
    assert( isxdigit(c) );
    res = (16 * res + hex_value(c));
  }

  return res;
}

std::string to_hex( const uint64_t a_key )
{
  std::stringstream ss;

  ss << std::hex << a_key;

  std::string part = ss.str();

  size_t need_nulls = KEY_LEN - part.size();
  
  return std::string(need_nulls, '0') + part;
}
}

Key::Key()
{
}

Key::Key( const uint64_t a_key )
  : m_key( a_key )
{
}

Key::Key( const std::string & a_key )
{
  from_string( a_key );
}

uint64_t Key::operator () () const
{
  return m_key;
}


void Key::from_string( const std::string & a_key )
{
  assert( a_key.size() == KEY_LEN );

  m_key = from_hex( a_key );
}

std::string Key::to_string() const
{
  std::stringstream res;

  res << to_hex( m_key );

  return res.str();
}

std::ostream & operator << ( std::ostream & a_out, const Key & a_key )
{
  a_out << a_key.to_string();
  return a_out;
}

std::istream & operator >> ( std::istream & a_in, Key & a_key )
{
  std::string str;
  a_in >> str;

  a_key.from_string(str);

  return a_in;
}


namespace {     

inline uint64_t generate_bit( const uint32_t a_bit )
{
  return MASKS[a_bit];
}

inline bool get_bit( const uint64_t a_block, const uint32_t a_bit )
{
  return (a_block & generate_bit(a_bit)) != 0;
}

inline void set_bit( uint64_t & a_block, const uint32_t a_bit, const bool a_value )
{
  if(a_value)
    a_block |= generate_bit(a_bit);
  else
    a_block &= ~generate_bit(a_bit);
}

inline uint64_t l_subkey( const uint64_t a_block )
{
  return (a_block & SUBKEY_PART_MASK); 
}

inline uint64_t r_subkey( const uint64_t a_block )
{
  return (a_block << SUBKEY_PART_BITS);
}

inline uint64_t merge_subkey( const uint64_t a_l, const uint64_t a_r )
{
  return a_l | (a_r >> SUBKEY_PART_BITS);
}

uint64_t shift_subkey( uint64_t a_block, const uint32_t a_howmuch )
{
  for( uint32_t it = 0; it < a_howmuch; it++ ) {
    set_bit(a_block, SUBKEY_PART_BITS, get_bit(a_block, 0));
    a_block <<= 1;
  }

  return a_block;
}

void print_block( const uint64_t a_block )
{
  for(uint32_t bit = 0; bit < BLOCK_BITS; bit++) {
    if(bit != 0 && (bit % 4) == 0)
      std::cout << " ";

    std::cout << (get_bit(a_block, bit) ? "1" : "0");
  }

  std::cout << std::endl;
}

uint64_t transform_PC1( const uint64_t a_block )
{
  uint64_t res = 0;
  for(uint32_t i = 0; i < PC1_LEN; i++)
    set_bit(res, i, get_bit(a_block, PC1[i]));
  return res;
}

uint64_t transform_PC2( const uint64_t a_block )
{
  uint64_t res = 0;
  for(uint32_t i = 0; i < PC2_LEN; i++)
    set_bit(res, i, get_bit(a_block, PC2[i]));
  return res;
}

uint64_t transform_IP( uint64_t a_block )
{
  uint64_t res = 0;

  #define IP_IT(i) res |= IP[i][a_block & IP_MASK]; \
                   a_block >>= IP_PART_BITS;

  IP_IT(0)
  IP_IT(1)
  IP_IT(2)
  IP_IT(3)
  IP_IT(4)
  IP_IT(5)
  IP_IT(6)
  IP_IT(7)

  #undef IP_IT

  return res;
}

uint64_t transform_IPR( uint64_t a_block )
{
  uint64_t res = 0;

  #define IPR_IT(i) res |= IPR[i][a_block & IPR_MASK]; \
                    a_block >>= IPR_PART_BITS;

  IPR_IT(0)
  IPR_IT(1)
  IPR_IT(2)
  IPR_IT(3)
  IPR_IT(4)
  IPR_IT(5)
  IPR_IT(6)
  IPR_IT(7)

  #undef IPR_IT

  return res;
}


inline uint64_t transform_E( uint64_t a_block )
{
  uint64_t res = 0;

  a_block >>= (BLOCK_BITS - E_BITS);

  #define E_IT(i) res |= E[i][a_block & E_MASK]; \
                  a_block >>= E_PART_BITS;

  E_IT(0)
  E_IT(1)
  E_IT(2)
  E_IT(3)

  #undef E_IT

  return res;
}

inline uint64_t transform_P( uint64_t a_block )
{
  uint64_t res = 0;

  a_block >>= (BLOCK_BITS - P_BITS);

  #define P_IT(i) res |= P[i][a_block & P_MASK]; \
                  a_block >>= P_PART_BITS;

  P_IT(0)
  P_IT(1)
  P_IT(2)
  P_IT(3)

  #undef P_IT

  return res;
}

inline uint64_t transform_SBOX( uint64_t a_block )
{
  uint64_t res = 0;

  a_block >>= (BLOCK_BITS - SBOX_BITS);

  #define SBOX_IT(i) res |= SBOX[i][a_block & SBOX_MASK]; \
                     a_block >>= SBOX_PART_BITS;

  SBOX_IT(7)
  SBOX_IT(6)
  SBOX_IT(5)
  SBOX_IT(4)
  SBOX_IT(3)
  SBOX_IT(2)
  SBOX_IT(1)
  SBOX_IT(0)

  #undef SBOX_IT
  
  return res;
}

inline uint64_t l_block( const uint64_t a_block )
{
  return (a_block & PART_MASK);
}

inline uint64_t r_block( const uint64_t a_block )
{
  return (a_block << PART_BITS);
}

inline uint64_t merge_block( const uint64_t a_l, const uint64_t a_r )
{
  return a_l | (a_r >> PART_BITS);
}

bool is_byte_empty( const uint64_t a_block, const uint32_t a_ind )
{
  const uint64_t mask = (BYTE_MASK << (a_ind * BYTE_BITS));
  return (a_block & mask) == 0;
}

}

DES::DES( const Key & a_key )
{ 
  // K+
  const uint64_t Kp = transform_PC1(a_key());

  // C0 && D0
  uint64_t C = l_subkey(Kp), D = r_subkey(Kp);
  
  // Cn && Dn && Kn 
  for(uint32_t round = 0; round < ROUNDS; round++) {
    C = shift_subkey(C, SUBKEYS_SHIFTS[round]);
    D = shift_subkey(D, SUBKEYS_SHIFTS[round]);
    m_keys[round] = transform_PC2(merge_subkey(C, D));
  }
}

inline uint64_t DES::f( uint64_t a_part, const uint32_t a_round ) const
{
  return transform_P(transform_SBOX(transform_E(a_part) ^ m_keys[a_round]));
}

uint64_t DES::encrypt_block( uint64_t a_block ) const
{
  a_block = transform_IP(a_block);

  uint64_t l = l_block(a_block), r = r_block(a_block), temp = 0;

  for( uint32_t round = 0; round < ROUNDS; round++ ) {
    temp = r;
    r = l ^ f(r, round);
    l = temp;
  }

  return transform_IPR(merge_block(r, l));
}

uint64_t DES::decrypt_block( uint64_t a_block ) const
{
  a_block = transform_IP(a_block);

  uint64_t l = l_block(a_block), r = r_block(a_block), temp = 0;

  for( uint32_t round = ROUNDS; round > 0; round-- ) {
    temp = r;
    r = l ^ f(r, round - 1);
    l = temp;
  }

  return transform_IPR(merge_block(r, l));
}

void DES::encrypt( std::istream & a_in, std::ostream & a_out ) const
{
  uint64_t buffer = 0;
  bool end = false;

  while(!end) {
    buffer = 0;
    a_in.read(reinterpret_cast < char * > (&buffer), BLOCK_BYTES);
    const size_t readed = a_in.gcount();

    if(readed == BLOCK_BYTES) {
      buffer = encrypt_block(buffer);
      a_out.write(reinterpret_cast < const char * > (&buffer), BLOCK_BYTES);
    } else {
      const size_t pos = readed * BYTE_BITS;
      buffer |= (static_cast < uint64_t > (1) << pos);
      buffer = encrypt_block(buffer);
      a_out.write(reinterpret_cast < const char * > (&buffer), BLOCK_BYTES);
      end = true;
    }
  }
}

void DES::output_last_byte( const uint64_t a_block, std::ostream & a_out ) const
{
  assert( a_block != 0 );

  uint32_t end = BLOCK_BYTES - 1;

  while(is_byte_empty(a_block, end))
    --end;

  if(end > 0)
    a_out.write(reinterpret_cast < const char * > (&a_block), end);
}

void DES::decrypt( std::istream & a_in, std::ostream & a_out ) const
{
  uint64_t last = 0, buffer = 0;
  bool first = true, end = false;

  while(!end) {
    buffer = 0;
    a_in.read( reinterpret_cast < char * > (&buffer), BLOCK_BYTES);
    const size_t readed = a_in.gcount();
 
    if(readed == 0) {
      if(!first)
        output_last_byte(last, a_out);

      end = true;
    } else if(readed == BLOCK_BYTES) {
      if(!first)
        a_out.write(reinterpret_cast < const char * > (&last), BLOCK_BYTES);
      
      last = decrypt_block(buffer);
    } else {
      assert(true);
    }

    first = false;
  }
}
