
.PHONY: all clean install
all : pack unpack_demo

pack: pack.c crc16.c pack.h
	$(CC) pack.c crc16.c -o $@

unpack_demo: unpack_demo.c pack.h
	$(CC) unpack_demo.c -o $@

install:
	@mkdir -p $(OBC_OUTPUT_DIR)
	cp pack $(OBC_OUTPUT_DIR)/
	cp unpack_demo $(OBC_OUTPUT_DIR)/

clean:
	-rm -f pack unpack_demo
	-rm -f $(OBC_OUTPUT_DIR)/pack
	-rm -f $(OBC_OUTPUT_DIR)/unpack_demo
























