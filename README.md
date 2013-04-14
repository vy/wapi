    `_`_`_`_____`_____`__`
    |`|`|`|``_``|``_``|``|
    |`|`|`|`````|```__|``|
    |_____|__|__|__|``|__|

WAPI (Wireless API) provides an easy-to-use function set to configure wireless network interfaces on a GNU/Linux system. One can think WAPI as a lightweight C API for `iwconfig`, `wlanconfig`, `ifconfig`, and `route` commands. (But it is not a thin wrapper for these command line tools.) It is currently being used in [WISERLAB](http://www.wiserlab.org/) test-bed to configure 802.11 based wireless interfaces. The development is partially supported by European Commission Grant Number PIRG06-GA-2009-256326.

For source codes, see [http://github.com/vy/wapi](http://github.com/vy/wapi). While you can manually generate the documentation via Doxygen, the most recent version of the documentation is (hopefully) always available online at [http://vy.github.com/wapi](http://vy.github.com/wapi).

You can also easily integrate WAPI to OpenWrt platform as well. For this purpose, just see `Makefile.OpenWrt` in the root directory.
