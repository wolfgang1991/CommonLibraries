libcurl is used for RPC client via HTTP. To disable this run make with NO_CURL=1 .

Examples are available in tests folder.

For using the http version with apache you need to use mod_proxy.

Enable it with:

sudo a2enmod proxy_http

Edit apache configuration (on Ubuntu:  /etc/apache2/apache2.conf ) and add something like:

ProxyPass "/test" "http://localhost:34634/test"
ProxyPassReverse "/test" "http://localhost:34634/test"

and restart apache (on Ubuntu: sudo apache2ctl restart ).

(In this example the http server using JSON-RPC is listening on the same machine at port 34634 and expects https post requests at /test .)
