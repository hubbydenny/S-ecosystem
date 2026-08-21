PORTNAME=	s-ecosystem
DISTVERSION=	1.0.0
CATEGORIES=	sysutils
MAINTAINER=	hubbydenny@users.noreply.github.com
COMMENT=	Shell utilities: sfetch, scat, sls
WWW=		https://github.com/hubbydenny/S-ecosystem

LICENSE=	GPLv3+
LICENSE_FILE=	${WRKSRC}/LICENSE

USES=		compiler:c++20-lang
USE_CXXSTD=	c++20

USE_GITHUB=	yes
GH_ACCOUNT=	hubbydenny
GH_PROJECT=	S-ecosystem
GH_TAGNAME=	v${DISTVERSION}

PLIST_FILES=	bin/sfetch \
		bin/scat \
		bin/sls

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/sfetch ${STAGEDIR}${PREFIX}/bin/
	${INSTALL_PROGRAM} ${WRKSRC}/scat ${STAGEDIR}${PREFIX}/bin/
	${INSTALL_PROGRAM} ${WRKSRC}/sls ${STAGEDIR}${PREFIX}/bin/

.include <bsd.port.mk>
