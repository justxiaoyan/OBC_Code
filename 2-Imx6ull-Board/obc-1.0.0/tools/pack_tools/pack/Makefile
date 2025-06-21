
.PHONY: all clean
all : pack unpack_demo

pack: pack.c
	@ $(CC) pack.c -o $@

unpack_demo: unpack_demo.c
	@ $(CC) unpack_demo.c -o $@

install:
	cp pack $(OBC_OUTPUT_DIR) 
	cp unpack_demo $(OBC_OUTPUT_DIR) 

clean:
	-rm pack
	-rm $(OBC_OUTPUT_DIR)/pack
	-rm unpack_demo
	-rm $(OBC_OUTPUT_DIR)/unpack_demo
























