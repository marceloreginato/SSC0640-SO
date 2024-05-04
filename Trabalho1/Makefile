APPS = ./apps
BIN = ./bin

make: myapps run

all: myapps

myapps:
	gcc $(APPS)/main.c -o $(BIN)/programaTrab -pthread -lrt

run:
	$(BIN)/programaTrab 10 2 4 2 5 1 2
#14 100 5 5 10 50 3

valgrind: $(BIN)/programaTrab
	valgrind --leak-check=full --show-leak-kinds=all $(BIN)/programaTrab

clean:
	rm -rf $(BIN)/*

commit:
	git add .
	git commit -m "automatic commit"
	git pull
	git push origin main

merge:
	git add .
	git commit -m "merge"
	git pull origin main