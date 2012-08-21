CC = g++
test:
	$(CC)  $(CPPFLAGS) -o  demo main.cpp  -L/usr/local/lib -L$(HOME)/running/3rd/lua/lua-5.1.3_coco-1.1.4/src -lluajit -ldl -lpthread 

debug: CPPFLAGS = -g 
debug: test
