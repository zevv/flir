MCONF_LIB_SRC = mconf/zconf.tab.c \
            mconf/lxdialog/checklist.c \
            mconf/lxdialog/menubox.c \
            mconf/lxdialog/textbox.c \
            mconf/lxdialog/yesno.c \
            mconf/lxdialog/inputbox.c \
            mconf/lxdialog/util.c 

MCONF_SRC = mconf/mconf.c \
	    $(MCONF_LIB_SRC)

CONF_SRC = mconf/conf.c \
	    $(MCONF_LIB_SRC)

mconf/mconf: $(MCONF_SRC)
	$(E) $(HOSTCC) $? -o $@ -lncurses -DCURSES_LOC="<ncurses.h>"

mconf/conf: $(CONF_SRC)
	$(E) $(HOSTCC) $? -o $@ -lncurses -DCURSES_LOC="<ncurses.h>"

mconf-clean:
	$(E) rm -f mconf/mconf mconf/conf
