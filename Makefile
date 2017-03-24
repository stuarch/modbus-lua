CC = gcc
INCLUDE= `pkg-config --cflags --libs lua51 libmodbus`
TARGET= modbus

main: $(TARGET).c
	$(CC) -Wall -shared -fPIC $(INCLUDE) -o $(TARGET).so $(TARGET).c

clean:
	rm $(TARGET).o

install:
	@if ! [ -d /usr/local/lib/lua ]; then\
		mkdir /usr/local/lib/lua/;\
	fi
	@if ! [ -d /usr/local/lib/lua/5.1 ]; then\
		mkdir /usr/local/lib/lua/5.1/;\
	fi
	cp ./$(TARGET).so /usr/local/lib/lua/5.1/$(TARGET).so

remove:
	rm /usr/local/lib/lua/5.1/$(TARGET.so)

