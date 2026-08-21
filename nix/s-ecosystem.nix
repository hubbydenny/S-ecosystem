{ lib, stdenv, fetchFromGitHub }:

stdenv.mkDerivation rec {
  pname = "s-ecosystem";
  version = "1.0.0";

  src = fetchFromGitHub {
    owner = "hubbydenny";
    repo = "S-ecosystem";
    rev = "v${version}";
    sha256 = "sha256-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
  };

  preBuild = ''
    substituteInPlace Makefile --replace 'g++' '${stdenv.cc}/bin/g++'
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    cp sfetch scat sls $out/bin/
    runHook postInstall
  '';

  meta = with lib; {
    description = "Shell utilities: sfetch, scat, sls";
    homepage = "https://github.com/hubbydenny/S-ecosystem";
    license = licenses.gpl3Plus;
    platforms = platforms.linux;
    maintainers = [ ];
  };
}
