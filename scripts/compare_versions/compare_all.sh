#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

for INSTANCE in "20181" "20182" "20191"; do
    rm -rf data/test
    cp -r data/${INSTANCE} data/test

    for SCENARIO in {1..10}; do
        ./cap test ${SCENARIO}
        ./cap_raphael ${SCENARIO}

        RAP_RESULTS=$(cat results/raphael_test.json | jq '. | .value')
        ME_RESULTS=$(cat results/test_${SCENARIO}.json | jq '. | .value')

        if [ ${RAP_RESULTS%.*} -eq ${ME_RESULTS%.*} ];
            then
                echo -e "${GREEN}Instance ${INSTANCE} scenario ${SCENARIO} OK${NC}"
            else
                echo -e "${RED}Instance ${INSTANCE} scenario ${SCENARIO} has different results${NC}"
                exit 1
        fi

    done
done
