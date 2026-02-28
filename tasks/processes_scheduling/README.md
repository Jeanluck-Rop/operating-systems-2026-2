# Simulación de técnicas de planificación de procesos (CPU scheduling)

## Compilación

Para compilar y generar el ejecutable se debe escribir el siguiente comando dentro del directorio `processes_scheduling`:
```
gcc -o prsch processes_scheduling.c 
```

## Ejecutar algoritmos

Una vez generado el archiv ejecutable `prsch`, se deben escribir los siguientes comandos para cada algoritmo:

**Guaranteed Scheduling**

La bandera `-g` indica que se generaran los resultados de utilizar el algoritmo **simulated_guaraneteed_scheduling** para los procesos de `Procesos.txt`.

```
./prsch -g < Procesos.txt 
```

**Shortes Process Next**

En cambio la bandera `-s` indica que se utilizará **simulated_shortest_process_next** para `Procesos.txt`.
```
./prsch -s < Procesos.txt 
```

## Comparación de resultados:

### Guaranteed Scheduling
GS se basa en hacer una promesa real al usuario sobre el rendimiento del sistema y cumplirla: si hay n procesos activos, a cada uno le corresponde aproximadamente `1/n` del tiempo de la CPU.
Esto funciona ya que el sistema lleva un registro continuo del tiempo que cada proceso ha estado en la CPU y calcula a cuánto tiempo tenía derecho de forma ideal. Después obtiene una proporción con base en el `tiempo_consumido / tiempo_debido`. 

El planificador siempre elige ejecutar el proceso con la proporción más baja, le da un tick de CPU, recalcula todos los ratios y repite. Esto lo hace preemptivo (apropiativo): puede interrumpir a cualquier proceso en cualquier tick para dárselo a uno más perjudicado.

Ser preemptivo tick a tick tiene una consecuencia directa sobre el tiempo de respuesta: cuando un proceso recién llega al sistema su `cpu_consumed = 0`, por lo que su ratio es exactamente `0.0 / tiempo_debido = 0.0`, el valor más bajo posible. Ningún proceso activo puede tener un ratio menor, así que el planificador lo elige en ese mismo tick. Por eso `start_time == arrival_time` para todos los procesos y el **Response Time** es siempre 0.

```
ID | Arrival | Burst | Start | Finish | Wait | Turnaround | Response
----------------------------------------------------------------------
 1 |       0 |     2 |     0 |      2 |    0 |          2 |        0
 2 |       2 |     4 |     2 |     30 |   24 |         28 |        0
 3 |       4 |     6 |     4 |    362 |  352 |        358 |        0
 4 |       6 |     8 |     6 |    477 |  463 |        471 |        0
 5 |       8 |    10 |     8 |    552 |  534 |        544 |        0
 6 |      10 |     2 |    10 |     16 |    4 |          6 |        0
...
96 |     190 |     2 |   190 |    263 |   71 |         73 |        0
97 |     192 |     4 |   192 |    408 |  212 |        216 |        0
...
100 |     198 |    10 |   198 |    600 |  392 |        402 |        0

Averages: 
Wait: 294.70 | Turnaround: 300.70 | Response: 0.00
```

El Response Time de 0.00 no indica una falla sino exactamente lo opuesto: ningún proceso esperó ni un solo tick antes de recibir CPU por primera vez, lo cual es la garantía central del algoritmo.

Sin embargo esta equidad tiene un costo: los tiempos de espera y turnaround son enormes, especialmente para procesos con burst alto. Esto se debe a que GS reparte el CPU en fracciones de 1 tick repartidas entre todos los procesos activos simultáneamente. Un proceso con `burst=10` necesita 10 ticks de CPU, pero como los va recibiendo de a uno intercalados con todos los demás, su tiempo en el sistema se extiende enormemente.

#### Comparando los procesos 5 y 6:
```
5 |       8 |    10 |     8 |    552 |  534 |        544 |        0
6 |      10 |     2 |    10 |     16 |    4 |          6 |        0
```

El proceso 5 llegó antes (`t=8`) pero terminó en `t=552` con una espera de 534 ticks. El proceso 6 llegó después (`t=10`) pero terminó en `t=16` con solo 4 ticks de espera. La diferencia es puramente el burst: el proceso 5 necesita 10 ticks de CPU y durante cada uno de esos ticks comparte el procesador con hasta 19 procesos más activos simultáneamente, recibiendo su turno de a un tick cada muchas rondas. El proceso 6 solo necesita 2 ticks, por lo que alcanza su ratio objetivo mucho antes y sale del sistema rápidamente.

Esto también explica el caso de los procesos 96 y 97 comparados con el proceso 5:
```
5  |       8 |    10 |     8 |    552 |  534 |        544 |        0
96 |     190 |     2 |   190 |    263 |   71 |         73 |        0
97 |     192 |     4 |   192 |    408 |  212 |        216 |        0
```

El proceso 96 llegó casi 200 ticks después que el proceso 5, pero tuvo una espera de apenas 71 ticks contra 534 del proceso 5. Su `burst=2` significa que necesita muy pocas rondas de CPU para completar su trabajo. El proceso 97, con `burst=4`, acumula casi el triple de espera que el 96 porque requiere el doble de rondas de CPU compitiendo contra los mismos procesos activos, demostrando que en GS el burst es el factor determinante del tiempo de espera, no el momento de llegada.


### Shortes Process Next
SPN se basa en dar máxima prioridad al proceso con la estimación de burst más corta al momento en que la CPU queda libre, lo que minimiza el tiempo de espera promedio global. Es un algoritmo no preemptivo (no apropiativo): una vez que el planificador elige un proceso, ese proceso corre su burst completo sin interrupciones hasta terminar. Solo cuando termina, el planificador vuelve a evaluar la cola.

```
ID | Arrival | Burst | Start | Finish | Wait | Turnaround | Response
----------------------------------------------------------------------
 1 |       0 |     2 |     0 |      2 |    0 |          2 |        0
 2 |       2 |     4 |     2 |     30 |   24 |         28 |        0
 3 |       4 |     6 |     4 |    362 |  352 |        358 |        0
 4 |       6 |     8 |     6 |    477 |  463 |        471 |        0
 5 |       8 |    10 |     8 |    552 |  534 |        544 |        0
 6 |      10 |     2 |    10 |     16 |    4 |          6 |        0
 7 |      12 |     4 |    12 |    205 |  189 |        193 |        0
 8 |      14 |     6 |    14 |    372 |  352 |        358 |        0
...
94 |     186 |     8 |   186 |    577 |  383 |        391 |        0
95 |     188 |    10 |   188 |    599 |  401 |        411 |        0
96 |     190 |     2 |   190 |    263 |   71 |         73 |        0
97 |     192 |     4 |   192 |    408 |  212 |        216 |        0
98 |     194 |     6 |   194 |    516 |  316 |        322 |        0
99 |     196 |     8 |   196 |    581 |  377 |        385 |        0
100 |     198 |    10 |   198 |    600 |  392 |        402 |        0

Averages:
 Wait: 294.70 | Turnaround: 300.70 | Response: 0.00
```
A diferencia de GS, aquí `Response == Wait` para todos los procesos. Esto ocurre porque SPN no es preemptivo: la primera vez que un proceso recibe CPU es también la única vez que empieza a correr sin parar hasta terminar, entonces el tiempo que esperó antes de arrancar es exactamente igual al tiempo que estuvo sin CPU. En GS un proceso podía recibir CPU en partes, pero en SPN el primer tick de CPU ya es el inicio de la ejecución completa.

El alto **Response Time** promedio de 178.50 contrasta directamente con el 0.00 de GS y refleja la naturaleza no preemptiva: los procesos deben esperar en cola a que el proceso actual termine su burst entero antes de poder arrancar.

```
3 |       4 |     6 |     6 |     12 |    2 |          8 |        2
6 |      10 |     2 |    12 |     14 |    2 |          4 |        2
7 |      12 |     4 |    14 |     18 |    2 |          6 |        2
4 |       6 |     8 |   240 |    248 |  234 |        242 |      234
```
El efecto más claro se ve comparando los procesos 3, 6 y 7 contra el proceso 4:

```
3 |       4 |     6 |     6 |     12 |    2 |          8 |        2
6 |      10 |     2 |    12 |     14 |    2 |          4 |        2
7 |      12 |     4 |    14 |     18 |    2 |          6 |        2
4 |       6 |     8 |   240 |    248 |  234 |        242 |      234
```

En `t=6` cuando la CPU queda libre, los procesos 3 (`burst=6`) y 4 (`burst=8`) están en cola. SPN elige el proceso 3 por ser más corto. Al terminar el proceso 3 en `t=12`, han llegado los procesos 6 (`burst=2`) y 7 (`burst=4`), ambos más cortos que el proceso 4. SPN elige al proceso 6, luego al 7, luego al 8 (`burst=6` que llegó en `t=14`), y así sucesivamente. El proceso 4 con `burst=8` queda postergado cada vez que llega alguien más corto, acumulando una espera de 234 ticks. Esto es inanición (**starvation**): el proceso 4 no hace nada malo, simplemente tiene un burst grande en un sistema donde constantemente llegan procesos más cortos que tienen prioridad sobre él.

El mismo patrón explica los procesos 5 y 10 con `burst=10`, que acumulan exactamente 392 ticks de espera cada uno, el máximo del sistema, siendo siempre los últimos elegidos en cada ronda de decisiones.


### Conclusión

GS es ideal para sistemas interactivos donde ningún proceso puede quedar sin atención. Su Response Time de 0 garantiza que todo proceso arranca inmediatamente al llegar, pero el costo es un turnaround promedio de 300.70 porque la CPU hace cambios de contexto constantes y todos los procesos avanzan lentamente en paralelo.
SPN es ideal para sistemas por lotes (batch) donde interesa terminar el mayor número de procesos lo antes posible. Su turnaround promedio de 184.50 es significativamente menor que el de GS porque saca rápidamente del sistema a los procesos cortos en lugar de hacerlos esperar rondas. Sin embargo su Response Time de 178.50 lo hace inaceptable para sistemas interactivos, y su inanición estructural es un riesgo real para procesos con burst alto en entornos de carga continua.
