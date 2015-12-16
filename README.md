rmtsvc
==========
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Environment](https://img.shields.io/badge/Windows-XP, Vista, 7, 8, 10-yellow.svg)
![License](https://img.shields.io/github/license/hsluoyz/rmtsvc.svg)

rmtsvc is the abbreviation for **ReMoTe SerViCe**. It is A web-based remote desktop &amp; control service for Windows. [yycnet](http://bbs.pediy.com/member.php?u=106711) open-sourced the 2.5.2 code of rmtsvc at http://bbs.pediy.com/showthread.php?t=184683.

rmtsvc supports **Windows XP and later (including Win7, Win8 and Win10)**. You can install rmtsvc service in a Windows machine, and then use your web browser to control it, including **remote desktop, command execution, process monitoring**, etc. 

## Build

* Open `rmtsvc.dsw` and build using **Visual C++ 6.0**, you should get the binary: `rmtsvc.exe`.

## Run

1. Put `example\rmtsvc.ini` in the same folder with `rmtsvc.exe`.
2. Run `rmtsvc.exe -i` and `rmtsvc.exe -s` to install and start the rmtsvc service.
3. Launch your web browser to `http://127.0.0.1:777`, username is `abc`, password is `123`, log on the portal and do your stuff.

## Download

If you don't want to build the binary by yourself, the prebuilt binaries can always be found here:
https://github.com/hsluoyz/rmtsvc/releases

## License

rmtsvc is published under [**The MIT License (MIT)**](http://opensource.org/licenses/MIT).

## Contact

* hsluoyz@gmail.com
