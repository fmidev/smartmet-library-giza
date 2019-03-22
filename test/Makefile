PROG = $(patsubst %.cpp,%,$(wildcard *Test.cpp))

MAINFLAGS = -std=c++11 -Wall -W -Wno-unused-parameter

EXTRAFLAGS = -Werror -pedantic -Wpointer-arith -Wcast-qual \
	-Wcast-align -Wconversion -Winline \
	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wno-pmf-conversions \
	-Wsign-promo -Wchar-subscripts -Wold-style-cast \
	-Wshadow -Woverloaded-virtual

DIFFICULTFLAGS = -Weffc++ -Wunreachable-code -Wredundant-decls

CC = g++
CFLAGS = -DUNIX -O0 -g $(MAINFLAGS) -Wno-write-strings

INCLUDES = \
	-I ../giza \
	-I /usr/include/smartmet \
	$$(pkg-config --cflags librsvg-2.0) \
	$$(pkg-config --cflags cairo)

LIBS = 	$$(pkg-config --libs librsvg-2.0) \
	$$(pkg-config --libs cairo) \
	../libsmartmet-giza.so \
	-lboost_filesystem \
	-lboost_system

all: $(PROG)
clean:
	rm -f $(PROG) *~

test: $(PROG)
	@mkdir -p failures
	@rm -f *.err
	@for prog in $(PROG); do \
	  ( ./$$prog || touch $$prog.err ) ; \
	done
	@test `find . -name \*.err | wc -l` = "0" || ( echo ; echo "The following tests have errors:" ; \
		for i in *.err ; do echo `basename $$i .err`; done ; rm -f *.err ; false ) 

$(PROG) : % : %.cpp ../libsmartmet-giza.so
	$(CC) $(CFLAGS) -o $@ $@.cpp $(INCLUDES) $(LIBS)
