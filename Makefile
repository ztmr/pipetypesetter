
CC=gcc
RMFIL=rm -f
MKSLINK=ln -s
STRIP=strip

CFLAGS_DBG=-O0 -ggdb3
CFLAGS_STD=-O2

# Rendering backend Settings
CFLAGS+=-D_HAS_CAIRO_=1 -D_HAS_GNOMEPRINT_=0

#LIBFO_CFLAGS!=pkg-config --cflags libfo-0.6
LIBFO_CFLAGS=`pkg-config --cflags libfo-0.6`
CFLAGS+=${LIBFO_CFLAGS}

#LIBFO_LDFLAGS!=pkg-config --libs libfo-0.6
LIBFO_LDFLAGS=`pkg-config --libs libfo-0.6`
LDFLAGS+=${LIBFO_LDFLAGS}

SRC=pipetypesetter.c

all: xslfo2pdf xslfo2ps xslfo2svg

xslfo2pdf: ${SRC} xslfo2pdf.dbg
	${CC} ${CFLAGS} ${CFLAGS_STD} ${LDFLAGS} ${SRC} -o $@
	${STRIP} $@

xslfo2pdf.dbg: ${SRC}
	${CC} ${CFLAGS} ${CFLAGS_DBG} ${LDFLAGS} ${SRC} -o $@

xslfo2ps:
	${MKSLINK} xslfo2pdf $@

xslfo2svg:
	${MKSLINK} xslfo2pdf $@

clean:
	${RMFIL} xslfo2pdf xslfo2pdf.dbg xslfo2ps xslfo2svg

