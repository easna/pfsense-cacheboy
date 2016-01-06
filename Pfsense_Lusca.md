# Introduction #
Compiled with:<br>
'--enable-removal-policies=lru heap' <br>
'--enable-storeio=aufs,coss,null' - <i>null if you don't want to use your hard drive</i><br>
'--enable-delay-pools' <br>
'--enable-arp-acl' - <i>mac address acl</i> <br>
'--enable-pf-transparent' <br>
'--enable-snmp' - <i>for MRTG</i><br>

Lusca-cache patched <a href='https://code.google.com/p/pfsense-cacheboy/source/detail?r=11'>r11</a>. Patches applied:<br>
<ol><li>ignore-must-revalidate<br>
</li><li>vary aware using storeurl<br>
</li><li>save vary object/index other than COSS (vary index will be lost on rebuild if in coss)<br>
<h3>Instruction</h3>
<b>Pfsense 1.2.3 or 2.0</b>
</li><li>From your "PfSense's GUI" run this at <b>Diagnostics</b> > <b>Command</b>. <br><i><sup>other than lusca packages might be old. Use package.sh to use original repository.</sup></i>
<pre><code>fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/script/package.sh &amp;&amp; chmod +x package.sh &amp;&amp; ./package.sh<br>
</code></pre>
</li><li>Now you may install lusca-cache at <b>System</b> > <b>Packages</b>.<br>
</li><li>To restore or use original repository. <br><i><sup>you can no longer see lusca updates</sup></i>
<pre><code>./package.sh off<br>
</code></pre>
<b>Pfsense 2.0</b>
</li><li>Go to the <a href='http://pfsense/pkg_mgr_settings.php'>http://pfsense/pkg_mgr_settings.php</a>
</li><li>then use chudy.0fees.net as your Package Repository<br>
<b>Notes</b><br>
Configurations at:<br>
/usr/local/etc/squid/dir.conf - <i>customized cache_dir if you might need it most case if you have more than one coss, aufs</i><br>
/usr/local/etc/squid/include.conf - <i>other squid configurations</i><br>
/usr/local/etc/squid/refresh.conf - <i>refresh patterns</i><br></li></ol>

<b>Troubleshoot:</b><br>
chudy.0fees.net <i>is free domain if error just retry</i> <br><br>
Pfsense GUI's doesn't save your configurations<br>
Causes:<br>
<ul><li>Previous squid packages and scripts doesn't clean up properly(pfsense limitation).<br>
</li><li>config.xml is already dirty as well as your previous installation had already been compromised.<br>
Fixed:<br>
Run this at <b>Diagnostics: Execute command</b> > <b>PHP execute</b><br>
<pre><code>$num_fields = array('squidcache','squid','squidauth','squidnac','squidtraffic','squidupstream','squidusers');<br>
foreach ($num_fields as $field)<br>
unset($config['installedpackages']["$field"]);<br>
write_config();<br>
</code></pre>
for selective approach where <b>squidcache</b> of URL <i>http://pfsense/pkg_edit.php?xml=squid_cache.xml&id=0</i> as an example that usually doesn't save.<br>
<pre><code>unset($config['installedpackages'][squidcache]);<br>
write_config();<br>
</code></pre>
for donation you may send it to<br> paypal: chudy_fernandez at yahoo dot com