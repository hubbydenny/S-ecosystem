# Maintainer: hubbydenny <hubbydenny at users dot noreply dot github dot com>
pkgname=s-ecosystem
pkgver=1.0.0
pkgrel=1
pkgdesc="Shell utilities: sfetch (system fetch), scat (file cat), sls (simple ls)"
arch=('x86_64')
url="https://github.com/hubbydenny/S-ecosystem"
license=('GPL-3.0-or-later')
makedepends=('gcc' 'make')
source=("$url/archive/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
  cd "$srcdir/S-ecosystem-$pkgver"
  make
}

package() {
  cd "$srcdir/S-ecosystem-$pkgver"
  install -Dm755 sfetch "$pkgdir/usr/bin/sfetch"
  install -Dm755 scat "$pkgdir/usr/bin/scat"
  install -Dm755 sls "$pkgdir/usr/bin/sls"
}
