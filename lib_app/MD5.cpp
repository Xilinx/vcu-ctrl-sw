/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

#include "lib_app/MD5.h"
#include <string>

// mix functions for processBlock()
inline uint32_t F(uint32_t X, uint32_t Y, uint32_t Z)
{
  return Z ^ (X & (Y ^ Z)); // RFC1321: F = (X & Y) | ((~X) & Z);
}

inline uint32_t G(uint32_t X, uint32_t Y, uint32_t Z)
{
  return Y ^ (Z & (X ^ Y)); // RFC1321: G = (X & Z) | (Y & (~Z));
}

inline uint32_t H(uint32_t X, uint32_t Y, uint32_t Z)
{
  return X ^ Y ^ Z;
}

inline uint32_t I(uint32_t X, uint32_t Y, uint32_t Z)
{
  return Y ^ (X | ~Z);
}

inline uint32_t Rot(uint32_t X, uint32_t s)
{
  return (X << s) | (X >> (32 - s));
}

#define MD5(d, X, Y, Z, Fn, i, C, s) d = (Rot(d + Fn(X, Y, Z) + pBlock[i] + C, s) + X)

/*************************************************************************************/
CMD5::CMD5()
{
  // RFC 1321
  m_pHash32[0] = 0x67452301;
  m_pHash32[1] = 0xEFCDAB89;
  m_pHash32[2] = 0x98BADCFE;
  m_pHash32[3] = 0x10325476;

  m_uNumBytes = 0;
  m_uBound = 0;
}

/*************************************************************************************/
void CMD5::Update(uint8_t* pBuffer, uint32_t uSize)
{
  m_uNumBytes += uSize;

  if(m_uBound)
  {
    while(uSize && m_uBound < sizeof(m_pBound))
    {
      m_pBound[m_uBound++] = *pBuffer++;
      uSize--;
    }
  }

  if(m_uBound == sizeof(m_pBound))
  {
    UpdateBlock(reinterpret_cast<uint32_t*>(m_pBound));
    m_uBound = 0;
  }

  while(uSize >= sizeof(m_pBound))
  {
    UpdateBlock(reinterpret_cast<uint32_t*>(pBuffer));
    pBuffer += sizeof(m_pBound);
    uSize -= sizeof(m_pBound);
  }

  while(uSize--)
    m_pBound[m_uBound++] = *pBuffer++;
}

/*************************************************************************************/
std::string CMD5::GetMD5()
{
  std::string sMD5;
  static const char* sToHex = "0123456789abcdef";

  m_pBound[m_uBound++] = 0x80; // Safe because m_pBound is never full at this step

  if(sizeof(m_pBound) - m_uBound < 8)
  {
    while(m_uBound < sizeof(m_pBound))
      m_pBound[m_uBound++] = 0;

    UpdateBlock(reinterpret_cast<uint32_t*>(m_pBound));
    m_uBound = 0;
  }

  while(m_uBound < sizeof(m_pBound) - 8)
    m_pBound[m_uBound++] = 0;

  // Append length in Bits
  AL_64U* pAppend = reinterpret_cast<AL_64U*>(m_pBound + m_uBound);
  *pAppend = (m_uNumBytes << 3);

  UpdateBlock(reinterpret_cast<uint32_t*>(m_pBound));

  sMD5.clear();

  for(size_t h = 0; h < sizeof(m_pHash8); ++h)
  {
    sMD5 += sToHex[m_pHash8[h] >> 4];
    sMD5 += sToHex[m_pHash8[h] & 15];
  }

  return sMD5;
}

/*************************************************************************************/
void CMD5::UpdateBlock(uint32_t* pBlock)
{
  uint32_t a = m_pHash32[0];
  uint32_t b = m_pHash32[1];
  uint32_t c = m_pHash32[2];
  uint32_t d = m_pHash32[3];

  MD5(a, b, c, d, F, 0, 0xD76AA478, 7);
  MD5(d, a, b, c, F, 1, 0xE8C7B756, 12);
  MD5(c, d, a, b, F, 2, 0x242070DB, 17);
  MD5(b, c, d, a, F, 3, 0xC1BDCEEE, 22);
  MD5(a, b, c, d, F, 4, 0xF57C0FAF, 7);
  MD5(d, a, b, c, F, 5, 0x4787C62A, 12);
  MD5(c, d, a, b, F, 6, 0xA8304613, 17);
  MD5(b, c, d, a, F, 7, 0xFD469501, 22);
  MD5(a, b, c, d, F, 8, 0x698098D8, 7);
  MD5(d, a, b, c, F, 9, 0x8B44F7AF, 12);
  MD5(c, d, a, b, F, 10, 0xFFFF5BB1, 17);
  MD5(b, c, d, a, F, 11, 0x895CD7BE, 22);
  MD5(a, b, c, d, F, 12, 0x6B901122, 7);
  MD5(d, a, b, c, F, 13, 0xFD987193, 12);
  MD5(c, d, a, b, F, 14, 0xA679438E, 17);
  MD5(b, c, d, a, F, 15, 0x49B40821, 22);

  MD5(a, b, c, d, G, 1, 0xF61E2562, 5);
  MD5(d, a, b, c, G, 6, 0xC040B340, 9);
  MD5(c, d, a, b, G, 11, 0x265E5A51, 14);
  MD5(b, c, d, a, G, 0, 0xE9B6C7AA, 20);
  MD5(a, b, c, d, G, 5, 0xD62F105D, 5);
  MD5(d, a, b, c, G, 10, 0x02441453, 9);
  MD5(c, d, a, b, G, 15, 0xD8A1E681, 14);
  MD5(b, c, d, a, G, 4, 0xE7D3FBC8, 20);
  MD5(a, b, c, d, G, 9, 0x21E1CDE6, 5);
  MD5(d, a, b, c, G, 14, 0xC33707D6, 9);
  MD5(c, d, a, b, G, 3, 0xF4D50D87, 14);
  MD5(b, c, d, a, G, 8, 0x455A14ED, 20);
  MD5(a, b, c, d, G, 13, 0xA9E3E905, 5);
  MD5(d, a, b, c, G, 2, 0xFCEFA3F8, 9);
  MD5(c, d, a, b, G, 7, 0x676F02D9, 14);
  MD5(b, c, d, a, G, 12, 0x8D2A4C8A, 20);

  MD5(a, b, c, d, H, 5, 0xFFFA3942, 4);
  MD5(d, a, b, c, H, 8, 0x8771F681, 11);
  MD5(c, d, a, b, H, 11, 0x6D9D6122, 16);
  MD5(b, c, d, a, H, 14, 0xFDE5380C, 23);
  MD5(a, b, c, d, H, 1, 0xA4BEEA44, 4);
  MD5(d, a, b, c, H, 4, 0x4BDECFA9, 11);
  MD5(c, d, a, b, H, 7, 0xF6BB4B60, 16);
  MD5(b, c, d, a, H, 10, 0xBEBFBC70, 23);
  MD5(a, b, c, d, H, 13, 0x289B7EC6, 4);
  MD5(d, a, b, c, H, 0, 0xEAA127FA, 11);
  MD5(c, d, a, b, H, 3, 0xD4EF3085, 16);
  MD5(b, c, d, a, H, 6, 0x04881D05, 23);
  MD5(a, b, c, d, H, 9, 0xD9D4D039, 4);
  MD5(d, a, b, c, H, 12, 0xE6DB99E5, 11);
  MD5(c, d, a, b, H, 15, 0x1FA27CF8, 16);
  MD5(b, c, d, a, H, 2, 0xC4AC5665, 23);

  MD5(a, b, c, d, I, 0, 0xF4292244, 6);
  MD5(d, a, b, c, I, 7, 0x432AFF97, 10);
  MD5(c, d, a, b, I, 14, 0xAB9423A7, 15);
  MD5(b, c, d, a, I, 5, 0xFC93A039, 21);
  MD5(a, b, c, d, I, 12, 0x655B59C3, 6);
  MD5(d, a, b, c, I, 3, 0x8F0CCC92, 10);
  MD5(c, d, a, b, I, 10, 0xFFEFF47D, 15);
  MD5(b, c, d, a, I, 1, 0x85845DD1, 21);
  MD5(a, b, c, d, I, 8, 0x6FA87E4F, 6);
  MD5(d, a, b, c, I, 15, 0xFE2CE6E0, 10);
  MD5(c, d, a, b, I, 6, 0xA3014314, 15);
  MD5(b, c, d, a, I, 13, 0x4E0811A1, 21);
  MD5(a, b, c, d, I, 4, 0xF7537E82, 6);
  MD5(d, a, b, c, I, 11, 0xBD3AF235, 10);
  MD5(c, d, a, b, I, 2, 0x2AD7D2BB, 15);
  MD5(b, c, d, a, I, 9, 0xEB86D391, 21);

  m_pHash32[0] += a;
  m_pHash32[1] += b;
  m_pHash32[2] += c;
  m_pHash32[3] += d;
}

