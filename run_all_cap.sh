#!/bin/bash

BLUE="\033[1;34m"
RED="\033[0;33m"
NC="\033[0m"

echo -e "${RED}.......| Classroom Assignment Problem |.......${NC}"
echo "Rodando todas as intâncias..."

# Para retomar as rodadas a partir de outro ponto, caso sejam interrompidas, mude o '1' para o cenario desejado
for SCENARIO in {1..10}
do
    for INSTANCE in 20181 20182 20191
    do
        echo -e "${BLUE}Rodando -- Instancia: ${INSTANCE} Cenario: ${SCENARIO} Sem setup${NC}"
        ./cap $INSTANCE $SCENARIO 0 0
        
        for SETUP_BEFORE in 0 1
        do
            echo -e "${BLUE}Rodando -- Instancia: ${INSTANCE} Cenario: ${SCENARIO} Setup antes: ${SETUP_BEFORE}${NC}"
            ./cap $INSTANCE $SCENARIO 1 $SETUP_BEFORE
        done
    done
done

echo -e "${BLUE}Todas as rodadas foram concluidas${NC}"