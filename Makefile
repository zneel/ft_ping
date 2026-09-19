CC = clang
CFLAGS = -Wall -Wextra -Werror

srcs = src/main.c
objs = $(srcs:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all: ft_ping

ft_ping: $(objs)
	$(CC) $(CFLAGS) $(objs) -o ft_ping

clean:
	rm -f $(objs)

re: fclean all

fclean: clean
	rm -f ft_ping

vm-setup:
	vm/setup.sh

vm:
	vm/run.sh

vm-stop:
	vm/stop.sh

vm-ssh:
	ssh -p 2222 -i vm/id_ed25519 debian@localhost

vm-clean: vm-stop
	rm -f vm/disk.qcow2 vm/efi-vars.fd vm/seed.iso vm/console.log
	rm -rf vm/.cidata

.PHONY: all clean re fclean vm-setup vm vm-stop vm-ssh vm-clean