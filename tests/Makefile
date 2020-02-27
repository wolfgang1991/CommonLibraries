all:
	cd ./FontTest && $(MAKE) DEBUG=$(DEBUG)
	cd ./GUIElementTests && $(MAKE) DEBUG=$(DEBUG)
	cd ./JSONRPCTestClient && $(MAKE) DEBUG=$(DEBUG)
	cd ./JSONRPCTestServer && $(MAKE) DEBUG=$(DEBUG)
	cd ./JSONTest && $(MAKE) DEBUG=$(DEBUG)
	cd ./PathTransform && $(MAKE) DEBUG=$(DEBUG)
	cd ./PolygonTest && $(MAKE) DEBUG=$(DEBUG)
	cd ./RectangleGradientDescent && $(MAKE) DEBUG=$(DEBUG)
	cd ./SocketTests && $(MAKE) DEBUG=$(DEBUG)

# Cleans all temporary files and compilation results.
clean:
	cd ./FontTest && $(MAKE) clean
	cd ./GUIElementTests && $(MAKE) clean
	cd ./JSONRPCTestClient && $(MAKE) clean
	cd ./JSONRPCTestServer && $(MAKE) clean
	cd ./JSONTest && $(MAKE) clean
	cd ./PathTransform && $(MAKE) clean
	cd ./PolygonTest && $(MAKE) clean
	cd ./RectangleGradientDescent && $(MAKE) clean
	cd ./SocketTests && $(MAKE) clean

.PHONY: all clean
