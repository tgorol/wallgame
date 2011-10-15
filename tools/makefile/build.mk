
.PHONY: clean mkdir lib app

lib: mkdir $(OBJ)
	$(AR) -rc $(OUT_NAME) $(foreach obj, $(OBJ), $(OUT_DIR)/$(obj))

%.o:%.c
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) $(INC_PATH) -o $(OUT_DIR)/$@ $< 

mkdir:
	install -d $(OUT_DIR)

clean:
	-rm -r $(OUT_DIR)/* $(APP_NAME)
	-rmdir --ignore-fail-on-non-empty -p $(OUT_DIR)

mkonelib:
#	cd $(OUT_DIR); for lib in `ls lib*.a`; do        \
		ar x $$lib `ar t $$lib`;                     \
	done
	ar -Trc $(OUT_DIR)/libwg.a $(OUT_DIR)/*.o

app: mkonelib
	$(CC) $(EXTRA_CFLAGS) $(CFLAGS) -o $(APP_NAME) $(LIB_PATH) $(INC_PATH) -lwg $(LIB_LIST)

