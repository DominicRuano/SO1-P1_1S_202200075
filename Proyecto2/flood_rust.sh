#!/bin/bash

URL="http://104.198.201.197.nip.io/input"

TOTAL=2  # Número total de peticiones que quieres enviar

SUCCESS=0
FAIL=0
TOTAL_TIME=0

for i in $(seq 1 $TOTAL)
do
  START_TIME=$(date +%s%3N) # Milisegundos actuales
  RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X POST "$URL" -H "Content-Type: application/json" -d "{\"description\":\"Test$i\",\"country\":\"GT\",\"weather\":\"Soleado\"}"
)
  END_TIME=$(date +%s%3N)

  DURATION=$((END_TIME - START_TIME)) # Tiempo en ms

  TOTAL_TIME=$((TOTAL_TIME + DURATION))

  if [ "$RESPONSE" == "200" ]; then
    ((SUCCESS++))
  else
    ((FAIL++))
  fi

  echo "Peticiones: $i - Respuesta: $RESPONSE - Duración: ${DURATION}ms"
done

AVG_TIME=$((TOTAL_TIME / TOTAL))

echo ""
echo "✔️ Peticiones exitosas: $SUCCESS"
echo "❌ Peticiones fallidas: $FAIL"
echo "⏱️ Tiempo promedio de respuesta: ${AVG_TIME}ms"
