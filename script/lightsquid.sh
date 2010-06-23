#!/bin/sh
pkg_add -r p5-Net-DNS
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/lightsquid/ip2name.dns
mv ip2name.dns /usr/local/libexec/lightsquid/
echo "done!"