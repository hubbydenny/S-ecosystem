# $NetBSD$

DISTNAME=	S-ecosystem-1.0.0
PKGNAME=	s-ecosystem-1.0.0
CATEGORIES=	sysutils
MASTER_SITES=	${MASTER_SITE_GITHUB:=hubbydenny/}
GITHUB_PROJECT=	S-ecosystem
GITHUB_TAG=	v${PKGVERSION_NOREV}
EXTRACT_SUFX=	.tar.gz

MAINTAINER=	hubbydenny@users.noreply.github.com
HOMEPAGE=	https://github.com/hubbydenny/S-ecosystem/
COMMENT=	Shell utilities: sfetch, scat, sls
LICENSE=	gnu-gpl-v3

USE_LANGUAGES=	c c++
GCC_REQD+=	11
USE_CXX_FEATURES+=	c++20

INSTALLATION_DIRS=	bin

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/sfetch ${DESTDIR}${PREFIX}/bin/sfetch
	${INSTALL_PROGRAM} ${WRKSRC}/scat ${DESTDIR}${PREFIX}/bin/scat
	${INSTALL_PROGRAM} ${WRKSRC}/sls ${DESTDIR}${PREFIX}/bin/sls

.include "../../mk/bsd.pkg.mk"
