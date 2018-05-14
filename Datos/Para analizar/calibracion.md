# Notas para calibración - ML

La idea de este Readme es proporcionar la info necesaria para poder colaborar en la calibración de sensores de bajo costo de tipo MOx (metal oxide) midiendo en campo junto a un equipo de referencia.  
El objetivo sería dejar una especie de template que sirva para que otrxs que necesiten calibrar sensores low-cost cuenten con una especia de guía.   
La idea es hacerlo en JupyterLab o similar (ej. https://github.com/patex1987/Temperature-Calibration/blob/master/Temperature_calibration.ipynb)  

## Datos
- Contamos con datos de medición en simultaneo para los períodos 5/1/2018 al 12/1/2018 (8dias) y 15/12/2018 al 2/2/2018 (18 dias) de 1 equipo de referencia (equipos Thermo Environment) y 2 equipos con sensores MOx. 
- Las variables medidas por ambos equipos son: NO2 (dióxido de nitrógeno), O3 (ozono), PM10 (material particulado d<10um), HR (humedad relativa) y T (temperatura).
- Los datos de CO del equipo de referencia no son confiables por lo que se descartan de la calibración. Es decir que solo se calibrarían datos de:
  - NO2 
  - O3
  - PM10 (solo uno de los equipos low-cost tiene estos datos) ver abajo. 

## Diccionario de datos 

- Datos de referencia en archivo [INV2018.csv](https://github.com/nanocastro/Repo_maca/blob/master/Datos/Para%20analizar/INV2018.csv)  

|Dia | Mes | Anio | Hora | Minuto | NO | NO2 | NOx | PM10 | O3 | Temperatura |  DV | Velocidad | HR | CO | SO2|
| -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| | |  |  |  |ppb | ppb | ppb | ug/m3 | ppb | °C |   | m/s | % | ppm | ppb|  

DV: dirección del viento

- Datos de Primer equipo con sensores MOx en archivo [macaconb.csv](https://github.com/nanocastro/Repo_maca/blob/master/Datos/Para%20analizar/macaconb.csv) 

|Dia | Mes | Anio | Hora | Minuto | O3 | NO2 | CO | dO3 | dNO2 | dCO |  HR | Temperatura | PM2.5 | PM10 |
| -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| | | | | | ohm | ohm | ohm | digital | digital | digital | % | °C | LPO | LPO |  

- Datos de Segundo equipo con sensores MOx en archivo [macasinb.csv](https://github.com/nanocastro/Repo_maca/blob/master/Datos/Para%20analizar/macasinb.csv) 

|Dia | Mes | Anio | Hora | Minuto | O3 | NO2 | CO | dO3 | dNO2 | dCO |  HR | Temperatura |
| -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| | | | | | ohm | ohm | ohm | digital | digital | digital | % | °C |  

ohm: valor de resistencia medida de los sensores (usar estos en la calibración!!)  
digital: tension medida (0 a 5V) convertida a un valor de 0-1024  

## Consideraciones para la calibraciòn 
- Los equipos low-cost guardan 1 dato por minuto. Los de referencia 1 dato cada 15 minutos. A los fines de comparar Se tendrían que promediar los valores de los sensores MOx para tener 1 dato cada 15 minutos. 
- se usa entre el 15-20 % de los datos para calibración y el resto para predecir (segun biblio, el tema seria elegir ese porcentaje, lo mejor parece elegir en la mitad del periodo de 18 dias)
- Los datos de humedad relativa y temperatura low-cost se tienen que calibrar también (aunque quizas se deban usar en las regresiones de calibración de los gases los datos de T y HR de la referencia y no los de cada equipo low-cost)  
- los datos de ambos equipos low-cost deberian correlacionar entre si, aunque es posible que no lo hagan debido a procesos de manufactura que llevan a baja reproducibilidad. Entonces cada sensor se calibra a la referencia por separado)

## Métodos utilizados para la calibración en bibliografía 
[Review](https://github.com/nanocastro/Repo_maca/blob/master/Referencias/Gases%20-%20ML/air-quality-sensors-and-data-adjustment-algorithms.pdf) de los algoritmos usados para el ajuste de datos 

### Para NO2 (sensor MICS-2710)
Según [Spinelle et al. 2015](https://github.com/nanocastro/Repo_maca/blob/master/Referencias/Gases%20-%20ML/Spinelle%20-%202015%20-%20Calibration%20low%20cost%20sensors%20O3%20y%20NO2.pdf) 
  - Regresión lineal: 
  La función de calibración es: Rs=aX+b donde Rs es la respuesta del sensor, X es la medicion de referencia. 
  - Regresión lineal multivariable usando método de minimos cuadrados 
    - Para NO2 : 
      Rs = a*NO2 + b*O3 + c*T + d
      Donde: Rs es la respuesta del sensor; NO2, O3 y T mediciones de referencia; a,b,c y d los parámetros de la regresión (una duda: una vez hecha la regresión y obtenido los parámetros O3 y T deberían ser tb medidos con el equipo low-cost no? entonces porque calibra con los valores de referencia?)  
  - Artificial Neural Network (ANN) 
    - Leo pero no entiendo mucho... en todo caso veamos esto juntos. 

### Para O3


### Para PM10


## Limitaciones conocidas de los sensores de gases MOx (metal oxide)
- output signal of MOx sensors is inﬂuenced by the concentrations of both the target and interfering gases,as well as the temperature and humidity effects
- baseline drift over time, caused by either changes in the heat output of the sensing element or due to poisoning (irreversible bonding) to the the sensor surface

## Notas adicionales: Calculos de resistencia equipos low-cost
Los valores de resistencia (ohm) se obtienen de la siguiente forma:  
Rs=( RL/(Vcc-Vs) )*Vs  
- Vcc: 5.0  
- Vs: tensión salida del sensor. dX*5.0/1023 (dX: digital)
- RL: 
  - CO:	100.000 ohm (para Segundo equipo) 200.000 (para Primer equipo)
  - O3:	22.000 ohm
  - NO2: 2.200 ohm
