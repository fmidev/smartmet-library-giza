PROG = $(patsubst %.cpp,%,$(wildcard *Test.cpp))

REQUIRES := librsvg cairo

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile.inc

INCLUDES += -I ../giza

LIBS += ../libsmartmet-giza.so \
	-lsmartmet-macgyver \
	-lfmt \
	-lwebp \
	$(REQUIRED_LIBS)

CFLAGS += $(shell pkg-config --cflags Magick++)
LIBS += $(shell pkg-config --libs Magick++)

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
