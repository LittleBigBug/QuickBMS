CATALOGS="fr"
LOCALEDIR=/share/locale

all:
	for cat in $(CATALOGS) ; do msgfmt -o $$cat.mo -v $$cat.po ; done

install:
	for cat in $(CATALOGS) ; \
	do \
		msgfmt -o $$cat.mo -v $$cat.po ; \
		mkdir -p $(DESTDIR)/$(LOCALEDIR)/$$cat/LC_MESSAGES; \
		cp -f $$cat.mo \
			$(DESTDIR)/$(LOCALEDIR)/$$cat/LC_MESSAGES/grzip.mo ; \
	done
	

clean:
	rm -f core *~ *.mo
	
cleandir: clean
