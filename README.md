# S-ecosystem
Shell utilities suite for Linux.

### sfetch
System fetch with 20 distro logos, side-by-side layout, auto-detection.

```
sfetch
sfetch --logo arch
sfetch --colors "red cyan green yellow"
sfetch --no-color
```

Supported logos: `kiss`, `alpine`, `alpine2`, `arch`, `artix`, `artix2`, `bedrock`, `chimera`, `chimera2`, `debian`, `exherbo`, `gentoo`, `gnu`, `guix`, `haiku`, `parabola`, `raspbian`, `uwuntu`, `void`, `unknown`

### scat
File concatenation utility.

```
scat file1.txt file2.txt
```

### sls
Simple ls utility.

```
sls
sls /path/to/dir
```

## Installation

### Manual
```bash
git clone https://github.com/hubbydenny/S-ecosystem
cd S-ecosystem
make
sudo make install
```
### PACKETS ARE NOT ACCEPTED
### Void Linux
```bash
sudo xbps-install -S s-ecosystem
```

### Arch Linux (AUR)
```bash
yay -S s-ecosystem
```

### Fedora
```bash
sudo dnf copr enable hubbydenny/s-ecosystem
sudo dnf install s-ecosystem
```

### NixOS
```bash
nix-env -iA nixpkgs.s-ecosystem
```

### Gentoo
```bash
sudo eselect repository add guru git@github.com:hubbydenny/guru.git
sudo emaint sync -r guru
sudo emerge s-ecosystem
```

### OBS (Debian, Ubuntu, openSUSE)
```bash
# Debian/Ubuntu
echo 'deb https://download.opensuse.org/repositories/home:/warpius/Debian_12/ /' | sudo tee /etc/apt/sources.list.d/home:warpius.list
curl -fsSL https://download.opensuse.org/repositories/home:/warpius/Debian_12/Release.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/home:warpius.gpg > /dev/null
sudo apt update && sudo apt install s-ecosystem

# openSUSE
sudo zypper addrepo https://download.opensuse.org/repositories/home:/warpius/openSUSE_Tumbleweed/home:warpius.repo
sudo zypper install s-ecosystem
```

## Config

TOML config at `~/.config/sfetch/config.toml`:

```toml
[colors]
title = "bright_green"
subtitle = "bright_cyan"
values = "bright_white"
labels = "bright_blue"
colors = "bright_green"

[display]
show_colors = true
show_logo = true
```

## Build

Requirements: g++ with C++20 support

```bash
make          # build all
make clean    # clean
```

Installed to `/usr/local/bin/` by default.


## License

GPL-3.0-or-later
