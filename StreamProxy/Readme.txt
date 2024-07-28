Routing of TCP or UDP communication via a proxy.
This is useful to connect applications to server with TCP based protocols (bidirectional) and or UDP based streams (sender in "server" network, multiple receivers per sender) which sits in different subnets via the public internet.
JSON-RPC over TLS is used for establishing connections via the proxy.
UDP and TCP are send over SSL. UDP streams only once per UDPStreamManager, which duplicates UDP multicast streams in the local network.
