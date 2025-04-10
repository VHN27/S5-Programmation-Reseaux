CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = jeu_devin_tp3

# Build rule
$(TARGET): jeu_devin_tp3.c
	$(CC) $(CFLAGS) -o $(TARGET) jeu_devin_tp3.c
	$(CC) $(CFLAGS) -o client-automatique client-automatique.c


all:
	$(CC) $(CFLAGS) -o $(TARGET) jeu_devin_tp3.c
	$(CC) $(CFLAGS) -o client_automatique client-automatique.c

# Clean rule
clean:
	rm -f $(TARGET)