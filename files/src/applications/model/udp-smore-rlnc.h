#include <vector>
#include <iostream>
#include <stdint.h>
#include <assert.h>
#include <arpa/inet.h>
#include <stdlib.h>

#ifndef _UDP_SMORE_RLNC_H
#define _UDP_SMORE_RLNC_H

class Gf256
{
  public:
    Gf256 (void): m_x (0) {}
    Gf256 (uint8_t y): m_x (y) {}
    Gf256 (const Gf256 &g): m_x (g.m_x) {}
    virtual ~Gf256 (void) {}

    Gf256 &operator= (Gf256 g) {m_x = g.m_x; return *this;}
    Gf256 &operator= (uint8_t y) {m_x = y; return *this;}

    Gf256 operator+ (Gf256 g) {return Gf256 (g.m_x ^ m_x);}
    Gf256 operator+ (uint8_t y) {return Gf256 (y ^ m_x);}
    friend Gf256 operator+ (uint8_t y, Gf256 g) {return Gf256 (y ^ g.m_x);}
    Gf256 &operator+= (Gf256 g) {m_x ^= g.m_x; return *this;}
    Gf256 &operator+= (uint8_t y) {m_x ^= y; return *this;}

    Gf256 operator- (Gf256 g) {return Gf256 (g.m_x ^ m_x);}
    Gf256 operator- (uint8_t y) {return Gf256 (y ^ m_x);}
    friend Gf256 operator- (uint8_t y, Gf256 g) {return Gf256 (y ^ g.m_x);}
    Gf256 operator- (void) {return *this;}
    Gf256 &operator-= (Gf256 g) {m_x ^= g.m_x; return *this;}
    Gf256 &operator-= (uint8_t y) {m_x ^= y; return *this;}

    Gf256 operator* (Gf256 g) {return Gf256 (mul (m_x, g.m_x));}
    Gf256 operator* (uint8_t y) {return Gf256 (mul (m_x, y));}
    friend Gf256 operator* (uint8_t y, Gf256 g) {return Gf256 (mul (y, g.m_x));}
    Gf256 &operator*= (Gf256 g) {m_x = mul (m_x, g.m_x); return *this;}
    Gf256 &operator*= (uint8_t y) {m_x = mul (m_x, y); return *this;}

    Gf256 operator/ (Gf256 g) {return Gf256 (mul (m_x, inv (g.m_x)));}
    Gf256 operator/ (uint8_t y) {return Gf256 (mul (m_x, inv (y)));}
    friend Gf256 operator/ (uint8_t y, Gf256 g) {return Gf256 (mul (y, inv (g.m_x)));}
    Gf256 &operator/= (Gf256 g) {m_x = mul (m_x, inv (g.m_x)); return *this;}
    Gf256 &operator/= (uint8_t y) {m_x = mul (m_x, inv (y)); return *this;}

    bool operator== (Gf256 g) {return m_x == g.m_x;}
    bool operator== (uint8_t y) {return m_x == y;}
    friend bool operator== (uint8_t y, Gf256 g) {return y == g.m_x;}

    bool operator> (Gf256 g) {return m_x > g.m_x;}
    bool operator> (uint8_t y) {return m_x > y;}
    friend bool operator> (uint8_t y, Gf256 g) {return y > g.m_x;}
    
    bool operator< (Gf256 g) {return m_x < g.m_x;}
    bool operator< (uint8_t y) {return m_x < y;}
    friend bool operator< (uint8_t y, Gf256 g) {return y < g.m_x;}

    bool operator>= (Gf256 g) {return m_x >= g.m_x;}
    bool operator>= (uint8_t y) {return m_x >= y;}
    friend bool operator>= (uint8_t y, Gf256 g) {return y >= g.m_x;}

    bool operator<= (Gf256 g) {return m_x <= g.m_x;}
    bool operator<= (uint8_t y) {return m_x <= y;}
    friend bool operator<= (uint8_t y, Gf256 g) {return y <= g.m_x;}

    bool operator!= (Gf256 g) {return m_x != g.m_x;}
    bool operator!= (uint8_t y) {return m_x != y;}
    friend bool operator!= (uint8_t y, Gf256 g) {return y != g.m_x;}

    friend std::ostream &operator<< (std::ostream &o, Gf256 g) {return o << (int)g.m_x;}

    uint8_t Value (void) {return m_x;}

  private:
    static const uint64_t modulus = 0x11b; // x^8 + x^4 + x^3 + x + 1
    uint8_t m_x;

    static int
    high_bit (uint64_t n)
    {
      int res = 0;
      uint64_t tmp;
      unsigned i;

      for (i = 32; i; i >>= 1)
        if ((tmp = (n >> i)))
        {
          n = tmp;
          res += i;
        }

      return res;
    }

    static uint64_t
    pmul (uint8_t x, uint8_t y)
    {
      int b;
      uint64_t scratch = 0;

      for (b = 0; b < 8; ++b)
        if (x & ((uint64_t)0x1 << b))
          scratch ^= (uint64_t)y << b;

      return scratch;
    }

    struct qr {uint64_t q; uint8_t r;};
    static struct qr
    pdivrem (uint64_t n, uint64_t d)
    {
      uint64_t r = n, q = 0;
      int hr, hd, t;

      hr = high_bit (r);
      hd = high_bit (d);
      while (r != 0 && hr >= hd)
        {
          t = hr - hd;	// Divide leading terms
          q ^= 1 << t;	// q = q + t
          r ^= d << t;	// r = r - (t * d)

          hr = high_bit (r);
        }

      assert (!(r & ~0xff));
      return (struct qr){q, (uint8_t)(r&0xff)};
    }

    static uint8_t
    mul(uint8_t x, uint8_t y)
    {
      return pdivrem (pmul (x, y), modulus).r;
    }

    static uint8_t
    inv (uint8_t x)
    {
      uint8_t x2   = mul (x,   x);
      uint8_t x4   = mul (x2,  x2);
      uint8_t x8   = mul (x4,  x4);
      uint8_t x16  = mul (x8,  x8);
      uint8_t x32  = mul (x16, x16);
      uint8_t x64  = mul (x32, x32);
      uint8_t x128 = mul (x64, x64);
      uint8_t x192 = mul (x64, x128);
      uint8_t x224 = mul (x32, x192);
      uint8_t x240 = mul (x16, x224);
      uint8_t x248 = mul (x8,  x240);
      uint8_t x252 = mul (x4,  x248);
      uint8_t x254 = mul (x2,  x252);

      return x254;
    }
};

class Gf256Vector
{
  public:
    Gf256Vector (uint32_t len): m_len (len) {m_e = new Gf256[m_len];}
    Gf256Vector (const Gf256Vector &v): m_len (v.m_len)
    {
      m_e = new Gf256[m_len];
      for (uint32_t i = 0; i < m_len; ++i)
        m_e[i] = v.m_e[i];
    }
    Gf256Vector (const uint8_t *elems, uint32_t len): m_len (len)
    {
      m_e = new Gf256[m_len];
      for (uint32_t i = 0; i < m_len; ++i)
        m_e[i] = elems[i];
    }
    Gf256Vector (const Gf256 *elems, uint32_t len): m_len (len)
    {
      m_e = new Gf256[m_len];
      for (uint32_t i = 0; i < m_len; ++i)
        m_e[i] = elems[i];
    }
    ~Gf256Vector (void) {m_len = 0; delete [] m_e; m_e = 0;}

    uint32_t Len (void) const {return m_len;}

    void
    Array (uint8_t *buf, uint32_t len)
    {
      assert (len == Len ());
      for (uint32_t i = 0; i < len; ++i)
        buf[i] = m_e[i].Value ();
    }

    bool
    IsZero (void)
    {
      for (uint32_t i = 0; i < m_len; ++i)
        if (m_e[i] != 0)
          return false;
      return true;
    }

    Gf256 &operator[] (uint32_t i) {assert (i < m_len); return m_e[i];}

    Gf256Vector &operator= (Gf256Vector v)
    {
      assert (m_len == v.m_len);
      for (uint32_t i = 0; i < m_len; ++i)
        m_e[i] = v.m_e[i];
      return *this;
    }

    Gf256Vector operator+ (Gf256Vector v)
    {
      assert (m_len == v.m_len);
      Gf256 buf[m_len];
      for (uint32_t i = 0; i < m_len; ++i)
        buf[i] = m_e[i] + v.m_e[i];
      return Gf256Vector (buf, m_len);
    }

    Gf256Vector &operator+= (Gf256Vector v)
    {
      assert (m_len == v.m_len);
      for (uint32_t i = 0; i < m_len; ++i)
        m_e[i] += v.m_e[i];
      return *this;
    }

    Gf256Vector operator- (Gf256Vector v)
    {
      assert (m_len == v.m_len);
      Gf256 buf[m_len];
      for (uint32_t i = 0; i < m_len; ++i)
        buf[i] = m_e[i] - v.m_e[i];
      return Gf256Vector (buf, m_len);
    }

    Gf256Vector &operator-= (Gf256Vector v)
    {
      assert (m_len == v.m_len);
      for (uint32_t i = 0; i < m_len; ++i)
        m_e[i] -= v.m_e[i];
      return *this;
    }

    Gf256Vector operator* (Gf256 k)
    {
      Gf256 buf[m_len];
      for (uint32_t i = 0; i < m_len; ++i)
        buf[i] = m_e[i] * k;
      return Gf256Vector (buf, m_len);
    }
    friend Gf256Vector operator* (Gf256 k, Gf256Vector v) {return v * k;}

    Gf256Vector &operator*= (Gf256 k)
    {
      for (uint32_t i = 0; i < m_len; ++i)
        m_e[i] *= k;
      return *this;
    }

    Gf256Vector operator/ (Gf256 k)
    {
      Gf256 buf[m_len];
      for (uint32_t i = 0; i < m_len; ++i)
        buf[i] = m_e[i] * 1/k;
      return Gf256Vector (buf, m_len);
    }

    Gf256Vector &operator/= (Gf256 k)
    {
      for (uint32_t i = 0; i < m_len; ++i)
        m_e[i] *= 1/k;
      return *this;
    }

    Gf256 operator* (Gf256Vector v)
    {
      Gf256 res;
      for (uint32_t i = 0; i < m_len; ++i)
        res += m_e[i] * v.m_e[i];
      return res;
    }

    bool operator== (Gf256Vector v)
    {
      for (uint32_t i = 0; i < m_len; ++i)
        if (m_e[i] != v.m_e[i])
          return false;
      return true;
    }

    bool operator!= (Gf256Vector v)
    {
      for (uint32_t i = 0; i < m_len; ++i)
        if (m_e[i] != v.m_e[i])
          return true;
      return false;
    }

    friend std::ostream &operator<< (std::ostream &o, Gf256Vector v)
    {
      static char hex[] = "0123456789abcdef";
      for (uint32_t i = 0; i < v.m_len; ++i)
        o << "\t" << hex[(v.m_e[i].Value()&0xf0) >> 4] << hex[v.m_e[i].Value()&0x0f];
      return o;
    }

  private:
    uint32_t m_len;
    Gf256 *m_e;
};

class Gf256Matrix
{
  public:
    Gf256Matrix (uint32_t cols): m_cols (cols) {}
    Gf256Matrix (uint32_t rows, uint32_t cols): m_cols (cols)
    {
      for (uint32_t i = 0; i < rows; ++i)
        m_e.push_back (Gf256Vector (cols));
    }
    Gf256Matrix (const Gf256Vector &v) {m_cols = v.Len (); m_e.push_back (v);}
    Gf256Matrix (const Gf256Matrix &m): m_cols (m.m_cols), m_e (m.m_e) {}
    ~Gf256Matrix (void) {m_cols = 0; m_e.clear ();}

    uint32_t Rows (void) {return m_e.size ();}
    uint32_t Cols (void) {return m_cols;}

    Gf256Matrix
    Transpose (void)
    {
      uint32_t rows = Rows ();
      Gf256Matrix res (m_cols, rows);
      for (uint32_t y = 0; y < rows; ++y)
        for (uint32_t x = 0; x < m_cols; ++x)
          res.m_e[x][y] = m_e[y][x];
      return res;
    }

    Gf256Matrix &operator= (Gf256Matrix m)
    {
      m_e.clear ();
      m_cols = m.m_cols;
      for (uint32_t rows = m.Rows (), i = 0; i < rows; ++i)
        m_e.push_back(m.m_e[i]);
      return *this;
    }

    Gf256Vector &operator[] (uint32_t i) {assert (i < Rows ()); return m_e[i];}

    Gf256Matrix operator+ (Gf256Matrix m)
    {
      assert (m_e.size () == m.Rows ());
      Gf256Matrix res (*this);
      for (uint32_t rows = Rows (), i = 0; i < rows; ++i)
        res.m_e[i] += m.m_e[i];
      return res;
    }

    Gf256Matrix &operator+= (Gf256Matrix m)
    {
      assert (m_e.size () == m.m_e.size ());
      for (uint32_t rows = Rows (), i = 0; i < rows; ++i)
        m_e[i] += m.m_e[i];
      return *this;
    }

    Gf256Matrix operator- (Gf256Matrix m)
    {
      assert (m_e.size () == m.Rows ());
      Gf256Matrix res (*this);
      for (uint32_t rows = Rows (), i = 0; i < rows; ++i)
        res.m_e[i] -= m.m_e[i];
      return res;
    }

    Gf256Matrix &operator-= (Gf256Matrix m)
    {
      assert (m_e.size () == m.m_e.size ());
      for (uint32_t rows = Rows (), i = 0; i < rows; ++i)
        m_e[i] -= m.m_e[i];
      return *this;
    }

    Gf256Matrix operator+ (Gf256Vector v)
    {
      assert (v.Len() == m_cols);
      Gf256Matrix res (*this);
      res.m_e.push_back (v);
      return res;
    }

    Gf256Matrix &operator+= (Gf256Vector v)
    {
      assert (v.Len() == m_cols);
      m_e.push_back (v);
      return *this;
    }

    Gf256Matrix operator* (Gf256Matrix m)
    {
      assert (m_cols == m.Rows ());
      uint32_t rows = Rows (), cols = m.m_cols;
      Gf256Matrix res (rows, cols);
      Gf256Matrix mT (m.Transpose ());
      for (uint32_t y = 0; y < rows; ++y)
        for (uint32_t x = 0; x < cols; ++x)
          res[y][x] = m_e[y] * mT.m_e[x];
      return res;
    }

    Gf256Matrix &operator*= (Gf256Matrix m)
    {
      assert (m_cols == m.Rows ());
      uint32_t rows = Rows (), cols = m.m_cols;
      Gf256Matrix tmp (rows, cols);
      Gf256Matrix mT (m.Transpose ());
      for (uint32_t y = 0; y < rows; ++y)
        for (uint32_t x = 0; x < cols; ++x)
          tmp[y][x] = m_e[y] * mT.m_e[x];
      *this = tmp;
      return *this;
    }

    Gf256Vector operator* (Gf256Vector v)
    {
      assert (m_cols == v.Len ());
      uint32_t rows = Rows ();
      Gf256Vector res (rows);
      for (uint32_t i = 0; i < rows; ++i)
        res[i] = m_e[i] * v;
      return res;
    }

    friend Gf256Vector operator* (Gf256Vector v, Gf256Matrix m)
    {
      assert (m.m_e.size () == v.Len ());
      Gf256Vector res (m.m_cols);
      Gf256Matrix mT (m.Transpose ());
      for (uint32_t i = 0; i < m.m_cols; ++i)
        res[i] = v * mT.m_e[i];
      return res;
    }

    Gf256Matrix operator* (Gf256 k)
    {
      Gf256Matrix res (*this);
      for (uint32_t rows = Rows (), y = 0; y < rows; ++y)
        for (uint32_t x = 0; x < m_cols; ++x)
          res.m_e[y][x] *= k;
      return res;
    }

    friend Gf256Matrix operator* (Gf256 k, Gf256Matrix m)
    {
      Gf256Matrix res (m);
      for (uint32_t rows = m.Rows (), y = 0; y < rows; ++y)
        for (uint32_t x = 0; x < m.m_cols; ++x)
          res.m_e[y][x] *= k;
      return res;
    }

    Gf256Matrix &operator*= (Gf256 k)
    {
      for (uint32_t rows = Rows (), y = 0; y < rows; ++y)
        for (uint32_t x = 0; x < m_cols; ++x)
          m_e[y][x] *= k;
      return *this;
    }

    Gf256Matrix operator/ (Gf256 k)
    {
      Gf256Matrix res (*this);
      for (uint32_t rows = Rows (), y = 0; y < rows; ++y)
        for (uint32_t x = 0; x < m_cols; ++x)
          res.m_e[y][x] /= k;
      return res;
    }

    Gf256Matrix &operator/= (Gf256 k)
    {
      for (uint32_t rows = Rows (), y = 0; y < rows; ++y)
        for (uint32_t x = 0; x < m_cols; ++x)
          m_e[y][x] /= k;
      return *this;
    }

    bool operator== (Gf256Matrix m)
    {
      for (uint32_t rows = Rows (), y = 0; y < rows; ++y)
        for (uint32_t x = 0; x < m_cols; ++x)
          if (m_e[y][x] != m.m_e[y][x])
            return false;
      return true;
    }

    bool operator!= (Gf256Matrix m)
    {
      for (uint32_t rows = Rows (), y = 0; y < rows; ++y)
        for (uint32_t x = 0; x < m_cols; ++x)
          if (m_e[y][x] != m.m_e[y][x])
            return true;
      return false;
    }

    Gf256Matrix operator| (Gf256Matrix m)
    {
      assert (Rows () == m.Rows ());
      uint32_t rows = Rows (), cols = m_cols + m.m_cols;
      Gf256Matrix res (rows, cols);
      for (uint32_t y = 0; y < rows; ++y)
        {
          for (uint32_t x = 0; x < m_cols; ++x)
            res[y][x] = m_e[y][x];
          for (uint32_t x = 0; x < m.m_cols; ++x)
            res[y][m_cols+x] = m.m_e[y][x];
        }
      return res;
    }

    friend std::ostream &operator<< (std::ostream &o, Gf256Matrix m)
    {
      for (uint32_t rows = m.Rows (), i = 0; i < rows; ++i)
        o << m.m_e[i] << std::endl;
      return o;
    }

    Gf256Matrix
    Submatrix (uint32_t top, uint32_t left, uint32_t bottom, uint32_t right)
    {
      assert (bottom > 1 && top < bottom-1 && bottom < Rows ());
      assert (right > 1 && left < right-1 && right < Cols ());
      uint32_t rows = bottom-top+1, cols = right-left+1;
      Gf256Matrix res (rows, cols);
      for (uint32_t y = 0; y < rows; ++y)
        for (uint32_t x = 0; x < cols; ++x)
          res.m_e[y][x] = m_e[top+y][left+x];
      return res;
    }

    Gf256Matrix
    Reduce (void)
    {
      Gf256Matrix res = *this;
      uint32_t rows = Rows (), cols = Cols (), dim = (rows < cols) ? rows : cols;
      uint32_t minrow;

      for(uint32_t pivot = 0; pivot < dim; ++pivot) {
        minrow = pivot;
        for(uint32_t y = pivot+1; y < rows; ++y)
          if(res.m_e[y][pivot] > res.m_e[minrow][pivot])
            minrow = y;
        Gf256Vector tmp = res.m_e[pivot];
        res.m_e[pivot] = res.m_e[minrow];
        res.m_e[minrow] = tmp;

        // M[pivot] = M[pivot] * (1/M[pivot][pivot])
        res.m_e[pivot] /= res.m_e[pivot][pivot];

        // M[y] = M[y] - M[pivot] * (M[y, pivot] / M[pivot, pivot])
        for(uint32_t y = pivot+1; y < rows; ++y)
          res.m_e[y] -= res.m_e[pivot] * res.m_e[y][pivot];

        for(uint32_t y = pivot; y > 0; --y)
          // M[y-1] = M[y-1] + (M[pivot] * M[y-1][lead+1])
          res.m_e[y-1] += res.m_e[pivot] * res.m_e[y-1][pivot];
      }

      return res;
    }

    static Gf256Matrix
    Identity (uint32_t n)
    {
      Gf256Matrix res (n, n);
      for (uint32_t i = 0; i < n; ++i)
        res[i][i] = 1;
      return res;
    }

    static Gf256Matrix
    Zero (uint32_t rows, uint32_t cols)
    {
      return Gf256Matrix (rows, cols);
    }

  private:
    uint32_t m_rows;
    uint32_t m_cols;
    std::vector<Gf256Vector> m_e;
};

class RlncCodeword
{
  public:
    RlncCodeword (uint8_t *e, uint16_t esz, uint8_t *c, uint16_t csz): m_esz (esz), m_csz (csz)
    {
      m_e = new uint8_t[m_esz];
      for (uint16_t i = 0; i < m_esz; ++i)
        m_e[i] = e[i];
      m_c = new uint8_t[m_csz];
      for (uint16_t i = 0; i < m_csz; ++i)
        m_c[i] = c[i];
    }
    RlncCodeword (const RlncCodeword &w): m_esz (w.m_esz), m_csz (w.m_csz)
    {
      m_e = new uint8_t[m_esz];
      for (uint16_t i = 0; i < m_esz; ++i)
        m_e[i] = w.m_e[i];
      m_c = new uint8_t[m_csz];
      for (uint16_t i = 0; i < m_csz; ++i)
        m_c[i] = w.m_c[i];
    }
    ~RlncCodeword (void) {delete [] m_c; m_c = 0; m_csz = 0; delete [] m_e; m_e = 0; m_esz = 0;}

    uint16_t GetNativeSize (void) {return m_csz;}
    uint16_t GetNativeCount (void) {return m_esz;}
    const uint8_t *GetPacket (void) {return m_c;}
    const uint8_t *GetCoeffs (void) {return m_e;}
    uint32_t GetSerializedSize (void) {return m_csz + m_esz + 4;}

    void
    Serialize (uint8_t *buf, uint32_t len)
    {
      assert (len >= GetSerializedSize ());
      uint16_t esz = htons (m_esz), csz = htons (m_csz);
      uint32_t i = 0;
      buf[i++] = ((uint8_t*)&esz)[0]; buf[i++] = ((uint8_t*)&esz)[1];
      for (uint32_t j = 0; j < m_esz; ++j)
        buf[i+j] = m_e[j];
      i += m_esz;
      buf[i++] = ((uint8_t*)&csz)[0]; buf[i++] = ((uint8_t*)&csz)[1];
      for (uint32_t j = 0; j < m_csz; ++j)
        buf[i+j] = m_c[j];
      i += m_csz;
      assert (i == GetSerializedSize ());
    }

    static RlncCodeword
    Deserialize(uint8_t *buf, uint32_t len)
    {
      uint16_t esz = ntohs (*(uint16_t*)buf);
      uint16_t csz = ntohs (*(uint16_t*)(buf+esz+2));
      assert ((uint32_t)esz+(uint32_t)csz+4 == len);

      return RlncCodeword (buf+2, esz, buf+esz+4, csz);
    }

    friend std::ostream &operator<< (std::ostream &o, RlncCodeword w)
    {
      static char hex[] = "0123456789abcdef";
      o << w.m_esz << ": ";
      for (uint16_t i = 0; i < w.m_esz; ++i)
        o << hex[(w.m_e[i]&0xf0) >> 4] << hex[w.m_e[i]&0x0f];
      o << ", " << w.m_csz << ": ";
      for (uint16_t i = 0; i < w.m_csz; ++i)
        o << hex[(w.m_c[i]&0xf0) >> 4] << hex[w.m_c[i]&0x0f];
      return o;
    }

  private:
    uint8_t *m_e;
    uint16_t m_esz;
    uint8_t *m_c;
    uint16_t m_csz;
};

class RlncWord
{
  public:
    RlncWord (uint8_t *buf, uint16_t len): m_len (len)
    {
      m_buf = new uint8_t[m_len];
      for (uint16_t i = 0; i < m_len; ++i)
        m_buf[i] = buf[i];
    }
    RlncWord (const RlncWord &w): m_len (w.m_len)
    {
      m_buf = new uint8_t[m_len];
      for (uint16_t i = 0; i < m_len; ++i)
        m_buf[i] = w.m_buf[i];
    }
    ~RlncWord (void) {delete [] m_buf;}

    uint16_t GetLen (void) {return m_len;}
    uint8_t const *GetPacket (void) {return m_buf;}

    friend std::ostream &operator<< (std::ostream &o, RlncWord w)
    {
      for (uint16_t i = 0; i < w.m_len; ++i)
        o << (char)w.m_buf[i];
      return o;
    }

  private:
    uint16_t m_len;
    uint8_t *m_buf;
};

class RlncEncoder
{
  public:
    RlncEncoder (std::vector<RlncWord> words, uint16_t wordsz)
      : m_words (words), m_wordsz (wordsz) {}
    RlncEncoder (const RlncEncoder &e): m_words (e.m_words), m_wordsz (e.m_wordsz) {}
    ~RlncEncoder (void) {}

    uint16_t GetNativeSize (void) {return m_wordsz;}
    uint16_t GetNativeCount (void) {return m_words.size ();}

    RlncCodeword Encode ()
    {
      uint16_t ncoeffs = m_words.size ();
      uint8_t coeffs[ncoeffs];
      for (uint16_t i = 0; i < ncoeffs; ++i)
        coeffs[i] = rand () % 256;

      Gf256Vector v (m_wordsz);
      for (uint32_t i = 0; i < ncoeffs; ++i)
        v += Gf256Vector (m_words[i].GetPacket (), m_wordsz) * Gf256 (coeffs[i]);

      uint8_t buf[v.Len ()];
      v.Array (buf, sizeof (buf));
      return RlncCodeword (coeffs, ncoeffs, buf, m_wordsz);
    }

  private:
    std::vector<RlncWord> m_words;
    uint16_t m_wordsz;
};

class RlncDecoder
{
  public:
    RlncDecoder (uint16_t wordsz, uint16_t wordct)
      : m_wordsz (wordsz), m_wordct (wordct), m_decoder (0) {}
    RlncDecoder (const RlncDecoder &d)
      : m_wordsz (d.m_wordsz), m_wordct (d.m_wordct), m_cwords (d.m_cwords)
      , m_decoder (0) {}
    ~RlncDecoder (void) {delete m_decoder; m_decoder = 0;}

    uint16_t GetNativeSize (void) {return m_wordsz;}
    uint16_t GetNativeCount (void) {return m_wordct;}

    bool
    AddCodeword (RlncCodeword &cword)
    {
      assert (cword.GetNativeSize () == m_wordsz);
      assert (cword.GetNativeCount () == m_wordct);
      if (m_decoder)
        return true;

      uint16_t ncwords = m_cwords.size ();
      Gf256Matrix d (m_wordct);

      if (ncwords != 0)
        {
          for (uint16_t i = 0; i < ncwords; ++i)
            d += Gf256Vector (m_cwords[i].GetCoeffs (), m_cwords[i].GetNativeCount ());
          d += Gf256Vector (cword.GetCoeffs (), cword.GetNativeCount ());
          d = d | Gf256Matrix::Identity(d.Rows ());
          d = d.Reduce ();
          if (d[d.Rows () - 1].IsZero ())
            return false;
        }

      m_cwords.push_back (cword);
      ++ncwords;

      if (ncwords >= m_wordct)
        {
          //std::cout << "Decoded:\n" << d << std::endl;
          m_decoder = new Gf256Matrix (d.Submatrix (0, m_wordct, m_wordct-1, d.Cols()-1));
          //std::cout << "Decoder:\n" << *m_decoder << std::endl;
          return true;
        }
      return false;
    }

    RlncWord
    Decode (uint16_t idx)
    {
      assert (m_decoder != 0);

      uint8_t buf[m_wordsz];
      Gf256Matrix m (m_wordsz);
      for(uint16_t i = 0; i < m_wordct; ++i)
        m += Gf256Vector (m_cwords[i].GetPacket (), m_cwords[i].GetNativeSize ());

      ((*m_decoder)[idx] * m).Array(buf, sizeof (buf));
      return RlncWord (buf, sizeof (buf));
    }

  private:
    uint16_t m_wordsz;
    uint16_t m_wordct;
    std::vector<RlncCodeword> m_cwords;
    Gf256Matrix *m_decoder;
};

class RlncRecoder
{
  public:
    RlncRecoder (uint16_t wordsz, uint16_t wordct)
      : m_wordsz (wordsz), m_wordct (wordct), m_ready (false) {}
    RlncRecoder (const RlncRecoder &r)
      : m_wordsz (r.m_wordsz), m_wordct (r.m_wordct), m_cwords (r.m_cwords)
      , m_ready (false) {}
    ~RlncRecoder (void) {m_cwords.clear (); m_ready = false;}

    bool IsReady (void) {return m_ready;}

    RlncCodeword
    Recode (void)
    {
      assert (m_ready == true);

      uint16_t ncoeffs = m_cwords.size ();
      uint8_t coeffs[ncoeffs];
      for (uint16_t i = 0; i < ncoeffs; ++i)
        coeffs[i] = rand () % 256;

      Gf256Vector c (m_wordsz);
      Gf256Vector e (m_wordct);
      for (uint32_t i = 0; i < ncoeffs; ++i)
        {
          c += Gf256Vector (m_cwords[i].GetPacket (), m_wordsz) * Gf256 (coeffs[i]);
          e += Gf256Vector (m_cwords[i].GetCoeffs (), m_wordct) * Gf256 (coeffs[i]);
        }

      uint8_t cbuf[c.Len ()];
      uint8_t ebuf[e.Len ()];
      c.Array (cbuf, sizeof (cbuf));
      e.Array (ebuf, sizeof (ebuf));
      return RlncCodeword (ebuf, m_wordct, cbuf, m_wordsz);
    }

    void
    AddCodeword (RlncCodeword &cword)
    {
      assert (cword.GetNativeSize () == m_wordsz);
      assert (cword.GetNativeCount () == m_wordct);

      uint16_t ncwords = m_cwords.size ();
      Gf256Matrix d (m_wordct);

      if (ncwords != 0)
        {
          for (uint16_t i = 0; i < ncwords; ++i)
            d += Gf256Vector (m_cwords[i].GetCoeffs (), m_cwords[i].GetNativeCount ());
          d += Gf256Vector (cword.GetCoeffs (), cword.GetNativeCount ());
          d = d | Gf256Matrix::Identity(d.Rows ());
          d = d.Reduce ();
          if (d[d.Rows () - 1].IsZero ())
            return;
        }

      m_cwords.push_back (cword);
      ++ncwords;

      if (ncwords > 1)
        m_ready = true;
    }

  private:
    uint16_t m_wordsz;
    uint16_t m_wordct;
    std::vector<RlncCodeword> m_cwords;
    bool m_ready;
};

#endif /* _UDP_SMORE_RLNC_H */
