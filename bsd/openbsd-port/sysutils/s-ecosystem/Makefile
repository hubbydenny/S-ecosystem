COMMENT=	Shell utilities: sfetch, scat, sls

GH_ACCOUNT=	hubbydenny
GH_PROJECT=	S-ecosystem
GH_TAGNAME=	v1.0.0

CATEGORIES=	sysutils

HOMEPAGE=	https://github.com/hubbydenny/S-ecosystem

MAINTAINER=	hubbydenny <hubbydenny@users.noreply.github.com>

# GPLv3+
PERMIT_PACKAGE=	Yes

WANTLIB=	${COMPILER_LIBCXX} c m

COMPILER=	base-gcc ports-gcc
COMPILER_LANGS=	c++

do-build:
	cd ${WRKBUILD} && ${SETENV} ${MAKE_ENV} \
		${CXX} ${CXXFLAGS} -std=c++20 -Isrc/helpers -Isrc/sfetch \
		-Iexternal/tomlplusplus/include \
		src/sfetch/main.cpp -o sfetch && \
	${CXX} ${CXXFLAGS} -std=c++20 src/cat/scat.cpp -o scat && \
	${CXX} ${CXXFLAGS} -std=c++20 src/simple/ls.cpp -o sls

do-install:
	${INSTALL_PROGRAM} ${WRKBUILD}/sfetch ${PREFIX}/bin/
	${INSTALL_PROGRAM} ${WRKBUILD}/scat ${PREFIX}/bin/
	${INSTALL_PROGRAM} ${WRKBUILD}/sls ${PREFIX}/bin/

.include <bsd.port.mk>
