all: build

reports := $(root)/reports-$(shell \
  if [ -n "$$HOSTNAME" ]; then \
    echo "$$HOSTNAME"; \
  else \
    hostname; \
  fi)

ifdef conf
  include $(reports)/configs/$(conf).mk
endif

include $(root)/test/compiler.mk

src = $(filter %.c %.S %.s, $(wildcard $(srcdir)/$(name).*))
api = ecrypt-$(type).h
binary = ecrypt-test$(id)$(exe)

ifdef var
  defvar = -DECRYPT_VARIANT=$(var)
endif

CFLAGS = $(opt)
LDFLAGS = $(CFLAGS)
TARGET_ARCH = $(arch)
CPPFLAGS = -DECRYPT_API=$(api) $(defvar) -I$(root)/include -I$(srcdir)

vpath %.h $(srcdir):$(root)/test:$(root)/api:$(root)/include
vpath %.c $(srcdir):$(root)/test:$(root)/api:$(root)/include
vpath %.s $(srcdir)
vpath %.S $(srcdir)

build: $(binary)

name:
	@echo $(name)

hash: $(binary)
	@((objdump -d $< 2> /dev/null) || cat $<) \
	  | (md5sum || md5 || openssl md5 || cksum) 2> /dev/null

run: $(binary)
	@echo $(run) ./$<

vectors: unverified.test-vectors
unverified.test-vectors: $(binary)
	$(run) ./$< -vqt 3600 > $@

$(binary): ecrypt-test$(testobj) ecrypt-$(type)$(testobj) $(name)$(testobj)
$(api): ecrypt-config.h ecrypt-machine.h ecrypt-portable.h 
ecrypt-test$(testobj): $(api) timers.h
ecrypt-$(type)$(testobj): $(api)
$(name)$(testobj): $(api)

variants: $(api)
	max=`awk \
	  '/^#define[ \t]+ECRYPT_MAXVARIANT/ { print $$3; exit; }' $<`; \
	var=0; while [ "$$var" -lt "$$max" ]; do var=`expr $$var + 1`; \
	  echo $$var; \
	done > $@

timestamp: $(src) $(api) ecrypt-test.c ecrypt-$(type).c timers.h
	touch timestamp

clean:
	$(RM) $(binary) $(name)$(testobj)

mrproper:
	$(RM) ecrypt-test* *.o *.obj *~

.PHONY: all build name hash run clean mrproper
