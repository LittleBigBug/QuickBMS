
name = yamb
type = sync
std = -std=gnu99

srcdir = .
root := $(shell \
  cd "$(srcdir)"; root="$(srcdir)"; \
  while [ "`pwd`" != "/" ]; do \
    if [ -r "`pwd`/test/ecrypt-test.mk" ]; then  \
      echo $$root; exit; \
    fi; \
    cd ..; root="$$root"/..; \
  done; \
  echo ".")

include $(root)/test/ecrypt-test.mk

