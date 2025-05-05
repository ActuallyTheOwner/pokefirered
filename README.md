# Pokémon RubyRed

This is a decomp hack of English Pokémon FireRed.
It is based on the ROM images below:

* [**pokefirered.gba**](https://datomatic.no-intro.org/?page=show_record&s=23&n=1616) `sha1: 41cb23d8dccc8ebd7c649cd8fbb58eeace6e2fdc`
* [**pokefirered_rev1.gba**](https://datomatic.no-intro.org/?page=show_record&s=23&n=1672) `sha1: dd5945db9b930750cb39d00c84da8571feebf417`

To set up the repository, see [INSTALL.md](INSTALL.md).

Note for Gentoo users, it is recomended to use GCC 13 to prevent errors
You will need multilib if not on an ARM CPU.

Please put this in your make.conf
EXTRA_ECONF="--disable-bootstrap"
(Optionally set the system compiler for clang if you still want modern compilation of system packages. pret/pokefirered and pret/agbcc may not use this as of 5/5/25.)

Next, compile for ARM, then for system
use eselect to switch GCC from 15 to 13.

It is possible for other distributions lacking GCC 13 to use an arch-chroot with an OS using GCC 13. Gentoo can install multiple versions of GCC at once.

For contacts and other pret projects, see [pret.github.io](https://pret.github.io/).