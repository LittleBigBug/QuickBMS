// modified and improved by Luigi Auriemma
// original code on http://nodling.nullneuron.net/ultima/ultima.html

// Ultima 6 dempression utility
// Last updated on 18-February-2005

#include <iostream>
#include <stdio.h>

// --------------------------------------------------
// a dictionary
// --------------------------------------------------

namespace Dict
{
   struct dict_entry
   {
      unsigned char root;
      int codeword;
   };

   const int dict_size = (1<<16); //10000;

   dict_entry dict[dict_size];
   int contains;

   void init()
   {
      contains = 0x102;
   }

   void add(unsigned char root, int codeword)
   {
      dict[contains].root = root;
      dict[contains].codeword = codeword;
      contains++;
   }

   unsigned char get_root(int codeword)
   {
      return (dict[codeword].root);
   }

   int get_codeword(int codeword)
   {
      return (dict[codeword].codeword);
   }

}

// --------------------------------------------------
// a simple implementation of an equally simple stack
// --------------------------------------------------

namespace Stack
{
   const int stack_size = (1<<16); //10000;

   unsigned char stack[stack_size];
   int contains;

   void init()
   {
      contains = 0;
   }

   bool is_empty()
   {
      return (contains==0);
   }

   bool is_full()
   {
      return(contains==stack_size);
   }

   void push(unsigned char element)
   {
      if (!is_full())
      {
         stack[contains] = element;
         contains++;   
      }
   }

   unsigned char pop()
   {
      unsigned char element;
      
      if (!is_empty())
      {
         element = stack[contains-1];
         contains--;
      }
      else
      {
         element = 0;
      }
      return(element);
   }

   unsigned char gettop()
   {
      if (!is_empty())
      {
         return(stack[contains-1]);
      }
      return 0;
   }
}

// ----------------------------------------------
// Read the next code word from the source buffer
// ----------------------------------------------
static
int get_next_codeword (long& bits_read, unsigned char *source, int codeword_size)
{
   unsigned char b0,b1,b2;
   int codeword;
   
   b0 = source[bits_read/8];
   b1 = source[bits_read/8+1];
   b2 = source[bits_read/8+2];

   codeword = ((b2 << 16) + (b1 << 8) + b0);
   codeword = codeword >> (bits_read % 8);

   codeword &= ((1 << codeword_size) - 1);
/*
   switch (codeword_size)
   {
    case 0x9:
        codeword = codeword & 0x1ff;
        break;
    case 0xa:
        codeword = codeword & 0x3ff;
        break;
    case 0xb:
        codeword = codeword & 0x7ff;
        break;
    case 0xc:
        codeword = codeword & 0xfff;
        break;
    default:
        printf("Error: weird codeword size!\n");
        break;
   }
*/
   bits_read += codeword_size;

   return (codeword);
}

static
void output_root(unsigned char root, unsigned char *destination, long& position)
{
   destination[position] = root;
   position++;   
}

static
void get_string(int codeword)
{
   unsigned char root;
   int current_codeword;
   
   current_codeword = codeword;
   Stack::init();
   while (current_codeword > 0xff)
   {
      root = Dict::get_root(current_codeword);
      current_codeword = Dict::get_codeword(current_codeword);
      Stack::push(root);
   }

   // push the root at the leaf
   Stack::push((unsigned char)current_codeword);
}

// -----------------------------------------------------------------------------
// LZW-decompress from buffer to buffer.
// The parameters "source_length" and "destination_length" are currently unused.
// They might be used to prevent reading/writing outside the buffers.
// -----------------------------------------------------------------------------
extern "C"
int ultima6_lzw_decompress(unsigned char *source, long source_length, unsigned char *destination, long destination_length)
{
   const int max_codeword_length = 16; //12;

   bool end_marker_reached = false;
   int codeword_size = 9;
   long bits_read = 0; 
   int next_free_codeword = 0x102;
   int dictionary_size = 1<<codeword_size;

   long bytes_written = 0;

   int cW;
   int pW = 0;
   unsigned char C;

   while (! end_marker_reached)
   {
      cW = get_next_codeword(bits_read, source, codeword_size);
      switch (cW)
      {
      // re-init the dictionary
      case 0x100:
          codeword_size = 9;
          next_free_codeword = 0x102;
          dictionary_size = 1<<codeword_size;
          Dict::init();
          cW = get_next_codeword(bits_read, source, codeword_size);
          output_root((unsigned char)cW, destination, bytes_written);
          break;
      // end of compressed file has been reached
      case 0x101:
          end_marker_reached = true;
          break;
      // (cW <> 0x100) && (cW <> 0x101)
      default:
          if (cW < next_free_codeword)  // codeword is already in the dictionary
          {
             // create the string associated with cW (on the stack)
             get_string(cW);
             C = Stack::gettop();
             // output the string represented by cW
             while (!Stack::is_empty())
             {
                output_root(Stack::pop(), destination, bytes_written);
             }
             // add pW+C to the dictionary
             Dict::add(C,pW);

             next_free_codeword++;
             if (next_free_codeword >= dictionary_size)
             {
                if (codeword_size < max_codeword_length)
                {
                   codeword_size += 1;
                   dictionary_size *= 2;
                }
             }
          }
          else  // codeword is not yet defined
          {
             // create the string associated with pW (on the stack)
             get_string(pW);
             C = Stack::gettop();
             // output the string represented by pW
             while (!Stack::is_empty())
             {
                output_root(Stack::pop(), destination, bytes_written);
             }
             // output the char C
             output_root(C, destination, bytes_written);
             // the new dictionary entry must correspond to cW
             // if it doesn't, something is wrong with the lzw-compressed data.
             if (cW != next_free_codeword) {
                printf("cW != next_free_codeword!\n");
                return -1;
             }
             // add pW+C to the dictionary
             Dict::add(C,pW);
             
             next_free_codeword++;
             if (next_free_codeword >= dictionary_size)
             {
                if (codeword_size < max_codeword_length)
                {                   
                   codeword_size += 1;
                   dictionary_size *= 2;
                } 
             }
          };
          break;
      }
      // shift roles - the current cW becomes the new pW
      pW = cW;
   }

   return bytes_written;
}
