MAKE=make
CCC=g++
prefix=/usr/local
exec_prefix=${prefix}

LIBS = -lX11 -lXcomposite -lXext -lXft -lfontconfig
LDFLAGS =
CXXFLAGS = -g -O2 -Wall -I/usr/include/freetype2 

OBJECTS = Border.o Buttons.o Client.o Events.o Main.o Manager.o Menu.o

.c.o:
	$(CCC) -c $(CXXFLAGS) $<

all: wmx

wmx: $(OBJECTS)
	$(CCC) -o wmx $(OBJECTS) $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o core

Border.o: Border.c Border.h General.h Config.h Client.h Manager.h listmacro.h
Buttons.o: Buttons.c Manager.h General.h Config.h listmacro.h Client.h Border.h Menu.h
Client.o: Client.c Manager.h General.h Config.h listmacro.h Client.h Border.h
Events.o: Events.c Manager.h General.h Config.h listmacro.h Client.h Border.h
Main.o: Main.c Manager.h General.h Config.h listmacro.h Client.h Border.h
Manager.o: Manager.c Manager.h General.h Config.h listmacro.h Menu.h Client.h Border.h
Menu.o: Menu.c Menu.h General.h Config.h Manager.h listmacro.h Client.h Border.h
