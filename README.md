# libToxTun

LibToxTun is a library that provides an easy way to set up a virtual network connection via tox.
The goal is to make the use of the connection for the user completly configuration free and by doing so to provide a simple way to share files, play games, use printers etc.
It's done by setting up a tap interface and sending all data through tox via its api.

You can find a qTox client with added support for libToxTun [here](https://github.com/ddorian1/qTox/tree/toxtun).

At the moment, one-to-one connections on Linux and Windows are supported. (Other Unix systems may work as well, but aren't tested yet.)
The IPv4 addresses are hardcoded to 10.0.0.1 and 10.0.0.2 for the time being.

## Next planned features
- Group connection support
- Auto-select unused IPv4 addresses
- Perfomance improvements
- Support for more platforms

## Usage notes
### Linux
You need a kernel with tun support (ether build in or as modul "tun").
Since the library needs to set up the tun interface, it needs either to by run as root (NOT recommended) or with the CAP_NET_ADMIN capabilitie set. To do so, run
```sudo setcap cap_net_admin=ep FOO```
replacing FOO with your aplication.

### Windows
For windows, you need the tuntap driver of the openvpn project, found [here](https://openvpn.net/index.php/open-source/downloads.html) (look for Tap-windows), and at least one unused virtual device.
