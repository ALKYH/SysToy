WSL_BASH := wsl bash -lc

.PHONY: all clean run

all:
	$(WSL_BASH) "cd /mnt/e/Projects/SysToy && make -f Makefile.wsl all"

clean:
	$(WSL_BASH) "cd /mnt/e/Projects/SysToy && make -f Makefile.wsl clean"

run:
	$(WSL_BASH) "cd /mnt/e/Projects/SysToy && make -f Makefile.wsl run"
