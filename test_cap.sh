#!/bin/bash
echo "Rodando testes"

for SCENARIO in {1..10}
do
    for SETUP in 0 1
    do
        for SETUP_BEFORE in 0 1
        do
            for HEURISTIC in 0 1
            do
                echo "Testando -- Cenario: $SCENARIO Setup: $SETUP Setup antes: $SETUP_BEFORE Heuristica: $HEURISTIC"
                ./cap $SCENARIO $SETUP $SETUP_BEFORE $HEURISTIC
            done
        done
    done
done

echo "Testes concluidos"