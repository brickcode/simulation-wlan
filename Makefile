.phony: clean

all: wlan

wlan: wlan.o
	g++ -o wlan wlan.o

wlan.o: wlan.cpp
	g++ -c wlan.cpp -o wlan.o

clean:
	-rm -rf *.o
	-rm -rf wlan
