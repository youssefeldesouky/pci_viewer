SRCS := main.c utils.c
OBJS := $(SRCS:.c=.o)
INCLUDES := -I.
CFLAGS := -Wall -Wextra -g $(INCLUDES)

all: pci_viewer

pci_viewer: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS) pci_viewer