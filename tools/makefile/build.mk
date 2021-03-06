
.PHONY: clean mkdir lib app mkonelib doc

lib: mkdir $(OBJ)
	$(AR) -rc $(OUT_DIR)/$(OUT_NAME).a  $(foreach obj, $(OBJ), $(OUT_DIR)/$(obj))

shared: mkdir $(OBJ)
	$(CC) -shared -Wl,-soname,$(OUT_NAME).so.1 $(CFLAGS) $(EXTRA_CFLAGS) $(INC_PATH) -o $(OUT_DIR)/$(OUT_NAME).so.1.0  $(foreach obj, $(OBJ), $(OUT_DIR)/$(obj))

%.o:%.c
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) $(INC_PATH) -o $(OUT_DIR)/$@ $< 

mkdir:
	install -d $(OUT_DIR)

clean:
	-rm -r $(OUT_DIR)/* $(APP_NAME)
	-rmdir --ignore-fail-on-non-empty -p $(OUT_DIR)

app:
	$(CC) $(EXTRA_CFLAGS) $(CFLAGS) $(MAIN_C) -o $(APP_NAME) $(LIB_PATH) $(INC_PATH)  $(LIB_LIST)

doc:
	cd ${ROOT_DIR};$(DOXYGEN);cd -

