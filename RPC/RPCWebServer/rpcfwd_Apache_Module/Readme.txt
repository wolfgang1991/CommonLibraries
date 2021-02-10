Add sth like:

LoadFile /usr/lib/x86_64-linux-gnu/libstdc++.so.6
LoadModule rpcfwd_module /usr/lib/apache2/modules/mod_rpcfwd.so
<Location /rpcfwd>
    SetHandler rpcfwd
</Location>


to /etc/apache2/apache2.conf (Ubuntu or Debian)

Compile, install library + restart apache: sudo make reload
