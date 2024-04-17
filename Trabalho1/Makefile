APPS = ./apps
BIN = ./bin
INCLUDE = ./include
OBJ = ./obj
SRC = ./src

make: libed myapps run

all:libed myapps

libed: 

myapps:
	gcc $(APPS)/main.c $(OBJ)/*.o -I $(INCLUDE) -lm -o $(BIN)/programaTrab

%.o: $(SRC)/%.c $(INCLUDE)/%.h
	gcc -c $< -I $(INCLUDE) -o $(OBJ)/$@

run:
	$(BIN)/programaTrab

valgrind: $(BIN)/programaTrab
	valgrind --leak-check=full --show-leak-kinds=all $(BIN)/programaTrab

clean:
	rm -rf $(BIN)/* $(OBJ)/*

commit:
	git add .
	git commit -m "automatic commit"
	git pull
	git push origin main

merge:
	git add .
	git commit -m "merge"
	git pull origin main