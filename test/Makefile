PROG = $(patsubst %.cpp,%,$(wildcard *Test.cpp))

REQUIRES := librsvg cairo

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile.inc

MAINFLAGS = -std=$(CXX_STD) -Wall -W -Wno-unused-parameter

CFLAGS = -DUNIX -O0 -g $(MAINFLAGS) -Wno-write-strings

INCLUDES += -I ../giza

LIBS += $(LIBRSVG_LIBS) \
	$(CAIRO_LIBS) \
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
	$(CXX) $(CFLAGS) -o $@ $@.cpp $(INCLUDES) $(LIBS)
