MAKE=make
CCC=g++
prefix=/usr/local
exec_prefix=${prefix}

LIBS=-lXcomposite -lXext -lX11  -lXft  -lfreetype  -lfontconfig -lfreetype -lSM -lICE
LDFLAGS=

CXXFLAGS=-g -O2 -I/usr/include/freetype2 -I/usr/include/libpng16  -I/usr/include/freetype2 -I/usr/include/libpng16  -I/usr/include/freetype2 -I/usr/include/libpng16  -Wall

OBJECTS	= Border.o Buttons.o Channel.o Client.o Config.o Events.o Main.o Manager.o Menu.o Rotated.o Session.o

.c.o:
	$(CCC) -c $(CXXFLAGS) $<

all:	wmx

wmx:	$(OBJECTS)
	test -f wmx && mv -f wmx wmx.old || true
	$(CCC) -o wmx $(OBJECTS) $(LDFLAGS) $(LIBS)

depend:
	-awk '{print;} /^# DO NOT DELETE/{exit}' Makefile>Makefile.dep
	test -s Makefile.dep
	mv Makefile.dep Makefile;g++ -MM $(CXXFLAGS) *.c>>Makefile

clean:
	rm -f *.o core

distclean: clean
	rm -f wmx wmx.old *~ *.bak *.org *.orig config.* Makefile

install:
	-mv -f ${exec_prefix}/bin/wmx ${exec_prefix}/bin/wmx.old
	cp wmx ${exec_prefix}/bin/wmx
	-chmod 0755 ${exec_prefix}/bin/wmx
	-chown root:wheel ${exec_prefix}/bin/wmx
	-rm -f ${exec_prefix}/bin/wmx.old

install-rpm:
	mkdir -p $(DESTDIR)/usr/bin/
	cp wmx $(DESTDIR)/usr/bin/wmx


# DO NOT DELETE -- make depend depends on it
Border.o: Border.c Border.h General.h Config.h Rotated.h Client.h \
 Manager.h listmacro.h
Buttons.o: Buttons.c Manager.h General.h Config.h listmacro.h Client.h \
 Border.h Rotated.h Menu.h
Channel.o: Channel.c Manager.h General.h Config.h listmacro.h Client.h \
 Border.h Rotated.h
Client.o: Client.c Manager.h General.h Config.h listmacro.h Client.h \
 Border.h Rotated.h
Config.o: Config.c Config.h
Events.o: Events.c Manager.h General.h Config.h listmacro.h Client.h \
 Border.h Rotated.h
Main.o: Main.c Manager.h General.h Config.h listmacro.h Client.h \
 Border.h Rotated.h
Manager.o: Manager.c Manager.h General.h Config.h listmacro.h Menu.h \
 Client.h Border.h Rotated.h Cursors.h
Menu.o: Menu.c Menu.h General.h Config.h Manager.h listmacro.h \
 Client.h Border.h Rotated.h
Rotated.o: Rotated.c Config.h Rotated.h
Session.o: Session.c Manager.h General.h Config.h listmacro.h
