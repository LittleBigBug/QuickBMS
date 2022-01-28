// the following code has been provided by Pokan http://www.geocities.jp/pokan_chan/

//////////////////////////////////////////////////////////////
//  return decoded data size.
long DecodeData(unsigned char *dst_buf, long dst_buf_size, unsigned char *src_buf, long src_buf_size);
//  return encoded data size.
long EncodeData(unsigned char *dst_buf, long dst_buf_size, unsigned char *src_buf, long src_buf_size);
//////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
//  Decode

#define falcom_ReverseByteBits(b) ((((b)&1)<<3)|(((b)&2)<<1)|(((b)&4)>>1)|(((b)&8)>>3))

long falcom_read_bit_flags(unsigned char *buf, long *buf_off, unsigned long *bit_flags, int *bit_flags_len)
{
	(*bit_flags) |= *((unsigned short*)(buf + *buf_off)) << *bit_flags_len;
	(*bit_flags_len) += 16;
	(*buf_off) += 2;

	return 2;
}

long falcom_DecodeData(unsigned char *dst_buf, long dst_buf_size, unsigned char *src_buf, long src_buf_size)
{
	int		i, n;
	int     block_num;
	int     block_off;
	int     block_size;
	int	    bit_flags_len;
	int		copy_size;
	int		copy_off;
	long    dst_off;
	long    src_off;
	unsigned char   byte_val;
	unsigned long   bit_flags;

	n = 0;
	block_num = 0xFF;
	copy_off = 0;
	src_off = 0;
	dst_off = 0;
	while (block_num)
	{
		if (dst_off > dst_buf_size || src_off > src_buf_size) break;

		block_size = *((unsigned short*)(src_buf + src_off));
		src_off += 2; block_size -= 4;
		if (src_buf[src_off++] != 0x00) break;
		//
		bit_flags = src_buf[src_off++];
		bit_flags_len = 8;
		copy_size = 1;
		for (block_off = 0; block_off < block_size; )
		{
			if (dst_off > dst_buf_size || src_off > src_buf_size) break;
			if (bit_flags_len == 0)
			{
				block_off += falcom_read_bit_flags(src_buf, &src_off, &bit_flags, &bit_flags_len);
			}

			//	1xyyyy
			if (bit_flags & 1)
			{
				copy_size = 0;
				if (bit_flags_len == 1)
				{
					block_off += falcom_read_bit_flags(src_buf, &src_off, &bit_flags, &bit_flags_len);
				}

				if ((bit_flags >> 1) & 1)
				{
					//	11xxxxx
					if (bit_flags_len < 7)
					{
						block_off += falcom_read_bit_flags(src_buf, &src_off, &bit_flags, &bit_flags_len);
					}

					copy_off = 0;
					byte_val = src_buf[src_off++]; block_off++;
					bit_flags >>= 2; bit_flags_len -= 2;
					if (bit_flags & 1) copy_off = 0x1000;

					bit_flags >>= 1; bit_flags_len--;
					copy_off += 0x100 * falcom_ReverseByteBits(bit_flags & 0x0F);

					// 1100000xyyyy
					if ((bit_flags & 0x0F) == 0 && copy_off == 0)
					{
						if (block_off >= block_size) break;
						bit_flags >>= 4; bit_flags_len -= 4;
						if (bit_flags_len < 5)
						{
							block_off += falcom_read_bit_flags(src_buf, &src_off, &bit_flags, &bit_flags_len);
						}
						byte_val = src_buf[src_off++]; block_off++;
						copy_size = 0x0E;

						i = bit_flags & 1;
						bit_flags >>= 1; bit_flags_len--;
						copy_size += falcom_ReverseByteBits(bit_flags & 15) << (i?8:0);
						if (i)
						{
							copy_size += byte_val;
							byte_val = src_buf[src_off++]; block_off++;
						}

						bit_flags >>= 4; bit_flags_len -= 4;
						for (i = 0; i < copy_size; i++)
						{
							if (dst_off > dst_buf_size) break;
							dst_buf[dst_off++] = byte_val;
						}
						continue;
					}
					bit_flags >>= 4; bit_flags_len -= 4;

					//	<2
					if (bit_flags_len < 2)
					{
						block_off += falcom_read_bit_flags(src_buf, &src_off, &bit_flags, &bit_flags_len);
					}

					copy_size = 2;
					for (i = 0; i < 4; i++)
					{
						if (bit_flags & 1) break;
						if (src_off > src_buf_size) break;

						bit_flags >>= 1; bit_flags_len--;
						if (bit_flags_len == 0)
						{
							block_off += falcom_read_bit_flags(src_buf, &src_off, &bit_flags, &bit_flags_len);
						}
						copy_size++;
					}
					if (i == 4)
					{
						if ((bit_flags & 1) == 0)
						{
							copy_size = src_buf[src_off++] + 0x0E; block_off++;
						}
						else
						{
							if (bit_flags_len < 4)
							{
								block_off += falcom_read_bit_flags(src_buf, &src_off, &bit_flags, &bit_flags_len);
							}
							copy_size = falcom_ReverseByteBits(bit_flags & 0xF) - 2;
							bit_flags >>= 3; bit_flags_len -= 3;
						}
					}

					copy_off += byte_val;
				}
				else
				{
					//	1xxxxx
					byte_val = src_buf[src_off++]; block_off++;
					while (1)
					{
						bit_flags >>= 1; bit_flags_len--;
						if (src_off > src_buf_size) break;
						if (bit_flags_len == 0)
						{
							block_off += falcom_read_bit_flags(src_buf, &src_off, &bit_flags, &bit_flags_len);
						}
						copy_size++;
						if (copy_size > 5)
						{
							if ((bit_flags & 1) == 0)
							{
								copy_size = src_buf[src_off++] + 0x0E; block_off++;
							}
							else
							{
								if (bit_flags_len < 4)
								{
									block_off += falcom_read_bit_flags(src_buf, &src_off, &bit_flags, &bit_flags_len);
								}
								copy_size = falcom_ReverseByteBits(bit_flags & 0xF) - 2;
								bit_flags >>= 3; bit_flags_len -= 3;
							}
							break;
						}

						//	end check
						if ((bit_flags & 1) == 1) break;
					}

					copy_off = byte_val;
				}

				copy_off = dst_off - copy_off;
				for (i = 0; i < copy_size; i++)
				{
					if (dst_off > dst_buf_size) break;
					dst_buf[dst_off++] = dst_buf[copy_off++];
				}
			}
			else
			{
				dst_buf[dst_off++] = src_buf[src_off++];
				block_off++;
			}
			bit_flags >>= 1; bit_flags_len--;
		}
		i = src_buf[src_off++];

		if (n > 1 && i > block_num) i = 0;
		block_num = i;
		if (n == 0 && block_num == 0) block_num = 0xFF;
		n++;
	}

	return dst_off;
}

//////////////////////////////////////////////////////////////
//  Encode

//  Max 0xFFFF
#define FP_BLOCK_SIZE 0xFFF0

int falcom_write_bit_flags(unsigned char *dst, long *dst_off, unsigned long *flags, long *flags_len, long *flags_off)
{
	if ((*flags_len) < 16) return 0;
	*(unsigned short*)(&dst[(*flags_off)]) = (unsigned short)(*flags);
	(*flags) >>= 16;
	(*flags_len) -= 16;
	(*flags_off) = (*dst_off);
	(*dst_off) += 2;

	return 2;
}

long falcom_EncodeDataBlock(unsigned char *dst, long dst_size, unsigned char *src, long src_size, int block_num)
{
	if (src_size > FP_BLOCK_SIZE) src_size = FP_BLOCK_SIZE;

	int     f, fill_mode;
	long    fill_len;
	long    j, n;
	long    len;
	long    block_size;
	long    block_size_off;
	long    block_src_size;
	long    fill_size_min = 0x0E;
	unsigned long   flags;
	long    flags_len;
	long    flags_off;
	long    dst_off, src_off;
	long    back_off;
	long    back_off_max = -0x2000;

	dst_off = 0;
	src_off = 0;
	block_size = 4;
	block_size_off = dst_off; dst_off += 2;
	block_src_size = 0;
	flags = 0;
	flags_len = 8;
	flags_off = dst_off; dst_off += 2;
	while (dst_off < dst_size && src_off < src_size)
	{
		len = 0;
		fill_len = 0;
		back_off = 0;
		fill_mode = 0;
		//  
		if (src_off < src_size-fill_size_min)
		{
			fill_mode = 1;
			for (j = 0; j < fill_size_min; j++)
			{
				if (src[src_off] != src[src_off + j])
				{
					fill_mode = 0;
					break;
				}
			}
			if (fill_mode)
			{
				for (n = fill_size_min; ; n++)
				{
					if (src_off + n >= src_size) break;
					if (src[src_off] != src[src_off + n]) break;
					if (n >= 0xFFF+0x00) break;
				}
				fill_len = n;
			}
		}
		//
		if (src_off < src_size-2)
		{
			for (j = -1; j > back_off_max; j--)
			{
				if ((src_off + j) < 0) break;
				if (*(unsigned short*)(src + src_off) != *(unsigned short*)(src + src_off + j)) continue;
				//  
				for (n = 2; ; n++)
				{
					if (src_off + n >= src_size) break;
					if (src[src_off + n] != src[src_off + j + n]) break;
					if (n > 0xFF+0x0E) break;
				}
				if (n > 0xFF+0x0E) n = 0xFF+0x0E;
				if (n > len)
				{
					len = n;
					back_off = j;
				}
			}
		}
		//  0x40ÅH 3Ex 3Fx
		if (back_off && fill_len < 0x40 && len > 0x02) fill_mode = 0;
		//
		if (fill_mode)
		{
			len = fill_len;
			flags |= (1 << flags_len); flags_len++;
			flags |= (1 << flags_len); flags_len++;
			flags_len++;
			flags_len += 4;
			if (flags_len == 16)
			{
				dst[dst_off++] = 0x01; block_size++;
				block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
			}
			else
			{
				block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
				dst[dst_off++] = 0x01; block_size++;
			}
			if (len <= 0x1d) //0x0E+0x0F)
			{
				flags_len++;
				block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
				n = len - 0x0E;
				for (j = 3; j >= 0; j--)
				{
					flags |= ((n&1) << (flags_len+j));
					n >>= 1;
				}
				flags_len += 4;
				if (flags_len == 16)
				{
					dst[dst_off++] = src[src_off]; block_size++;
					block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
				}
				else
				{
					block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
					dst[dst_off++] = src[src_off]; block_size++;
				}
			}
			else
			{
				flags |= (1 << flags_len); flags_len++;
				block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
				n = len - 0x0E;
				n = n / 0x100;
				for (j = 3; j >= 0; j--)
				{
					flags |= ((n&1) << (flags_len+j));
					n >>= 1;
				}
				flags_len += 4;
				n = (len - 0x0E) & 0xFF;
				if (flags_len == 16)
				{
					dst[dst_off++] = (unsigned char)n; block_size++;
					dst[dst_off++] = src[src_off]; block_size++;
					block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
				}
				else
				{
					block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
					dst[dst_off++] = (unsigned char)n; block_size++;
					dst[dst_off++] = src[src_off]; block_size++;
				}
			}
			src_off += len;
			block_src_size += len;
			continue;
		}
		//  no copy
		if (back_off == 0)
		{
			flags_len++;
			dst[dst_off++] = src[src_off++];
			block_src_size++;
			block_size++;
			block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
			continue;
		}
		//
		src_off += len;
		block_src_size += len;
		flags |= (1 << flags_len);
		flags_len++;
		block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
		back_off *= -1;
		f = 0;
		if (back_off < 0x100)
		{
			dst[dst_off++] = (unsigned char)back_off;
			block_size++;
		}
		else
		{
			n = back_off;
			flags |= (1 << flags_len); flags_len++;
			if (n >= 0x1000) { flags |= (1 << flags_len); n -= 0x1000; }
			flags_len++;
			n = n / 0x100;
			for (j = 3; j >= 0; j--)
			{
				flags |= ((n&1) << (flags_len+j));
				n >>= 1;
			}
			flags_len += 4;
			if (flags_len == 16)
			{
				dst[dst_off++] = (unsigned char)(back_off & 0xFF); block_size++;
				block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
			}
			else
			{
				block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
				dst[dst_off++] = (unsigned char)(back_off & 0xFF); block_size++;
			}
			f = 1;
		}
		if (len < 0x06)
		{
			flags_len += len - f;
			flags |= (1 << (flags_len-1));
			block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
			continue;
		}
		if (len < 0x0E)
		{
			flags_len += 5 - f;
			n = len + 2;
			for (j = 3; j >= 0; j--)
			{
				flags |= ((n&1) << (flags_len+j));
				n >>= 1;
			}
			flags_len += 4;
			block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
			continue;
		}
		flags_len++;
		flags_len += 5 - f;
		if (flags_len == 16)
		{
			dst[dst_off++] = (unsigned char)(len - 0x0E); block_size++;
			block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
		}
		else
		{
			block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
			dst[dst_off++] = (unsigned char)(len - 0x0E); block_size++;
		}
	}
	if (src_off == src_size)
	{
		flags |= (1 << flags_len); flags_len++;
		flags |= (1 << flags_len); flags_len++;
		//  
		flags_len += 5;
		//
		block_size += falcom_write_bit_flags(dst, &dst_off, &flags, &flags_len, &flags_off);
		dst[dst_off++] = 0x00; block_size++;
	}
	*(unsigned short*)(&dst[flags_off]) = (unsigned short)flags;
	*(unsigned short*)(&dst[block_size_off]) = (unsigned short)block_size;
	dst[dst_off++] = (unsigned char)block_num;
	//

	return dst_off;
}

long falcom_EncodeData(unsigned char *dst, long dst_size, unsigned char *src, long src_size)
{
	long size;
	int  i, n, nn;

	n = src_size / FP_BLOCK_SIZE;
	nn = src_size % FP_BLOCK_SIZE;
	if (nn) n++;

	size = 0;
	for (i = 0; i < n; i++)
	{
		src_size = FP_BLOCK_SIZE;
		if (nn && (i+1) == n) src_size = nn;
		size += falcom_EncodeDataBlock(dst + size, dst_size - size, src, src_size, n-i-1);
		src += src_size;
		if (size > dst_size) break;
	}

	return size;
}

