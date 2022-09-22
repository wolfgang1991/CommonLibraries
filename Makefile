# make Irrlicht separately in Irrlicht/source/Irrlicht

all:
	cd ./Common && "$(MAKE)" DEBUG=$(DEBUG)
	cd ./IrrlichtExtensions && "$(MAKE)" DEBUG=$(DEBUG)
	cd ./RPC/JSONRPC2 && "$(MAKE)" DEBUG=$(DEBUG)
	cd ./Sound && "$(MAKE)" DEBUG=$(DEBUG)

# Cleans all temporary files and compilation results.
clean:
	cd ./Common && "$(MAKE)" clean
	cd ./IrrlichtExtensions && "$(MAKE)" clean
	cd ./RPC/JSONRPC2 && "$(MAKE)" clean
	cd ./Sound && "$(MAKE)" clean

.PHONY: all clean
