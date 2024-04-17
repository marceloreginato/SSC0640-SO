APPS = ./apps
BIN = ./bin

make: myapps run

all: myapps

myapps:
	gcc $(APPS)/main.c -o $(BIN)/programaTrab

run:
	$(BIN)/programaTrab

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