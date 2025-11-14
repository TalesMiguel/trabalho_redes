#!/bin/bash

RESULTS_DIR="results"
mkdir -p $RESULTS_DIR

PROJECT_DIR="/home/tales/grad/redes"
NS3_DIR="$PROJECT_DIR/ns-3.45"
SCRATCH_FILE="$PROJECT_DIR/TrabRedes.cc"

CLIENTS=(1 2 4 8 16 32)
PROTOCOLS=(0 1 2)
MOBILITY_MODES=(0 1)

PROTOCOL_NAMES=("udp" "tcp" "mixed")
MOBILITY_NAMES=("static" "mobile")

if [ ! -f "$SCRATCH_FILE" ]; then
    echo "ERRO: Arquivo TrabRedes.cc nao encontrado em $SCRATCH_FILE"
    exit 1
fi

cp "$SCRATCH_FILE" "$NS3_DIR/scratch/"
if [ $? -ne 0 ]; then
    echo "ERRO: Nao foi possivel copiar TrabRedes.cc para scratch/"
    exit 1
fi

echo "Compilando TrabRedes.cc..."
cd $NS3_DIR
./ns3 build
if [ $? -ne 0 ]; then
    echo "ERRO: Falha na compilacao"
    exit 1
fi

echo ""
echo "Iniciando simulacoes..."
echo "Total de cenarios: $((${#CLIENTS[@]} * ${#PROTOCOLS[@]} * ${#MOBILITY_MODES[@]}))"
echo ""

count=0
total=$((${#CLIENTS[@]} * ${#PROTOCOLS[@]} * ${#MOBILITY_MODES[@]}))

for mobility in "${MOBILITY_MODES[@]}"; do
    for protocol in "${PROTOCOLS[@]}"; do
        for clients in "${CLIENTS[@]}"; do
            count=$((count + 1))
            
            mob_name=${MOBILITY_NAMES[$mobility]}
            proto_name=${PROTOCOL_NAMES[$protocol]}
            output_file="$PROJECT_DIR/projeto/${RESULTS_DIR}/flow_${proto_name}_${mob_name}_${clients}.xml"
            
            echo "[$count/$total] Executando: protocol=${proto_name}, mobility=${mob_name}, clients=${clients}"
            
            ./ns3 run "TrabRedes --nWifi=$clients --protocol=$protocol --mobility=$mobility --outputFile=$output_file"
            
            if [ $? -eq 0 ]; then
                echo "  -> Concluido: $output_file"
            else
                echo "  -> ERRO na simulacao"
            fi
            echo ""
        done
    done
done

echo "Todas as simulacoes foram concluidas!"
echo "Resultados salvos em: $PROJECT_DIR/projeto/$RESULTS_DIR/"
