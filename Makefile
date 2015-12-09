SRC		= files
DST		= ns-3.24
FILES	:= $(patsubst $(SRC)/%,%,$(shell find files -name '*' -type f))

.PHONY: all copyfiles run clean

all: copyfiles
	@$(MAKE) -C ns-3.24 configure
	@$(MAKE) -C ns-3.24 build

copyfiles:
	@for file in $(FILES); do cp -f $(SRC)/$$file $(DST)/$$file; done

run:
	@(cd ns-3.24; ./waf --run scratch/smore)

clean:
	@$(MAKE) -C ns-3.24 clean
	$(RM) $(patsubst %,$(DST)/%,$(FILES))
