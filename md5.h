/* $Id$
 */

#ifndef MD5_H
#define MD5_H

/* most modern machine should have 32 bits ints 
   * (some configure option should be added)
   */
typedef unsigned int uint32;

struct Bin_MD5Context {
  uint32 buf[4];
  uint32 bits[2];
  unsigned char in[64];
};

void Bin_MD5Init(struct Bin_MD5Context *context);
void Bin_MD5Update(struct Bin_MD5Context *context, 
		   unsigned char const *buf,
		   unsigned len);
void Bin_MD5Final(unsigned char digest[16], 
		  struct Bin_MD5Context *context);
void Bin_MD5Transform (uint32 buf[4], uint32 const in[16]);

#endif /* !MD5_H */
