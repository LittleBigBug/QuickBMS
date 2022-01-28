LIBOUT = liblzhl.a
LIBOBJS = HuffStat.o HuffStatTmp.o LZBuffer.o LZHL.o LZHLCompressor.o LZHLDecoderStat.o LZHLDecompressor.o LZHLEncoder.o LZHLEncoderStat.o
EXEOUT = test
EXEOBJS = test.o

$(LIBOUT): $(LIBOBJS)
	$(AR) rcs $(LIBOUT) $(LIBOBJS)
    
$(EXEOUT): $(EXEOBJS) $(LIBOUT)
	$(CXX) -o $(EXEOUT) $(EXEOBJS) $(LIBOUT)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(LIBOBJS) $(EXEOBJS) $(LIBOUT) $(EXEOUT)
