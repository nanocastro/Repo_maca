cls
clear

% Datos de referencia y MACA para los períodos 5/1/2018 al 12/1/2018 (8dias) y 15/12/2018 al 2/2/2018 (18 dias)

%%LECTURA DE LOS DATOS PARA ANALIZAR
%Lectura de datos del equipo Referencia
%Dia,mes,año,hora,minuto,NO,NO2,NOx,PM10,O3,Temperatura,DV,Velocidad,HR,CO,SO2
DatosE = dlmread('./INV2018.csv',";")(4:end,1:16);
EtiqE = {"NO","NO2","NOx","PM10","O3","Temperatura","DV","Velocidad","HR","CO","SO2"};


%Lectura de datos del MACA CON bluetooth
%Aclaración: los datos de PM son en realidad datos de LPO del Shinyei
%Dia,mes,año,hora,minuto,O3,NO2,CO,dO3,dNO2,dCO,HR,T,PM2.5,PM10
DatosMc=dlmread('./macaconb.csv',";")(:,3:end);

%Lectura de datos del MACA SIN bluetooth
%Dia,mes,año,hora,minuto,O3,NO2,CO,dO3,dNO2,dCO,HR,T, (calcular O3,NO2,CO)
DatosMs=dlmread('./macasinb.csv',";")(:,4:end);

%%Imprime la primer linea para verificar que estan ok
%DatosE(1,:);
%DatosMc(1,:);
%DatosMs(1,:);

%% Transforma a DateNum
%DAYS = datenum (YEAR, MONTH, DAY, HOUR, MINUTE)
FechasE=datenum(DatosE(:,3)+2000,DatosE(:,2),DatosE(:,1),DatosE(:,4),DatosE(:,5));
FechasMc=datenum(DatosMc(:,3)+2000,DatosMc(:,2),DatosMc(:,1),DatosMc(:,4),DatosMc(:,5));
FechasMs=datenum(DatosMs(:,3)+2000,DatosMs(:,2),DatosMs(:,1),DatosMs(:,4),DatosMs(:,5));

% Encontrar las fechas para las que hay datos de ambos equipos
FechasE_Match = ismember(FechasE, FechasMc);	%Fechas camion para las que hay datos del MACA.
MatchDates = FechasE(FechasE_Match);
FechasMc_Match = ismember(unique(FechasMc), MatchDates);	%Fechas del MACA para la que hay coincidencia.

graphics_toolkit ("gnuplot")

%Grafica todos los datos del camión (diarios)

for j=1:11
h=figure(j)
clf
for i=min(DatosE(:,1)):max(DatosE(:,1))
Dia=find(DatosE(:,1)==i);
Horaplot=FechasE(Dia)-fix(FechasE(Dia));
plot(Horaplot,DatosE(Dia,j+5));
hold on
endfor
axE=gca();
set(axE,"xtick",min(Horaplot):max(Horaplot)/6:max(Horaplot));
set(axE, "xticklabel", [ 0 4 8 12 16 20 24]);
xlabel( "Tiempo","fontname", "DejaVuSans-Bold.ttf", "fontsize", 10);
ylabel( EtiqE{j},"fontname", "DejaVuSans-Bold.ttf", "fontsize", 10, "color", [0,1,0])
nombre=EtiqE{j};
print(h,nombre,"-dpng");
endfor


%Grafica de todos los datos camión dia por dia (NO, NO2, O3)
% Toma periodos de 8 dias
figure(2)
Enero=find(DatosE(:,2)==1);
for i=min(DatosE(Enero,1)):(10+min(DatosE(Enero,1)))
Dia=find(DatosE(Enero,1)==i);
Horaplot=FechasE(Dia)-fix(FechasE(Dia));
subplot(5,2,i);
plot(Horaplot,DatosE(Dia,6));
hold on
endfor

% Grafica serie de datos de PM10 Camion
graphics_toolkit ("gnuplot")
figure(2)
clf
plotyy(FechasE,DatosE(:,9),FechasE,DatosE(:,13))
axA=gca();
set(axA,"xtick",FechasPlot)
set(axA,"xlabel",datestr

%Comparacion de MACAb con referencia

% Ozono
figure(3)
clf
subplot(2,1,1)
graphics_toolkit ("gnuplot")
plotyy(FechasE,DatosE(:,10),FechasMc,DatosMc(:,6))
subplot(2,1,2)
graphics_toolkit ("qt")
scatter(DatosE(FechasE_Match,10),DatosMc(FechasMc_Match,6))

% Temperatura
figure(4)
clf
subplot(2,1,1)
graphics_toolkit ("gnuplot")
plotyy(FechasE,DatosE(:,11),FechasMc,DatosMc(:,13))
subplot(2,1,2)
graphics_toolkit ("qt")
scatter(DatosE(FechasE_Match,11),DatosMc(FechasMc_Match,13))

P = polyfit (DatosE(FechasE_Match,11),DatosMc(FechasMc_Match,13),1)


clf
scatter(DatosMc(:,13),DatosMc(:,6))

%Grafica los datos del MACA con Blue
figure(2)
clf
for j=1:3
subplot(3,1,j);
for i=min(DatosMc(:,1)):max(DatosMc(:,1))
Dia=find(DatosMc(:,1)==i);
Horaplot=FechasMc(Dia)-fix(FechasMc(Dia));
plot(Horaplot,DatosMc(Dia,j+5));
hold on
endfor
axE=gca();
set(axE,"xtick",min(Horaplot):max(Horaplot)/6:max(Horaplot));
set(axE, "xticklabel", [ 0 4 8 12 16 20 24]);
xlabel( "Tiempo","fontname", "DejaVuSans-Bold.ttf", "fontsize", 10);
%ylabel( EtiqE{j},"fontname", "DejaVuSans-Bold.ttf", "fontsize", 10, "color", [0,1,0])
endfor

graphics_toolkit("gt")
Intervalo=6;
FechasPlot = floor(FechasMc(1)):1/24:ceil(FechasMc(end));
figure(4)
clf
plot(FechasMc, DatosMc(:,6), "-b", "linewidth", 3);
box off
axM = gca();
set(axM, "yaxislocation", "left")
set(axM, "linewidth", 1)
LimM = get(axM, "xlim");	%Asignamos el mismo lÂ´imete a ambos ejes X
set(axM, "xtick", [0])		%Ocultamos uno de los ejes.
ylblM = ylabel( "Resistencia [ohm]","fontname", "DejaVuSans-Bold.ttf", "fontsize", 18, "color", [0,0,1]);
set(axM, "fontname", "DejaVuSans-Bold.ttf", "fontsize", 16)
axE = axes();	%Nuevo juego de ejes para graficar la otra variable.
plot(FechasE, DatosE(:,10), "-r", "linewidth", 3);
set(axE, "xlim", LimM)
box off
set(axE, "xtick", FechasPlot(1:Intervalo:end))
set(axE, "xticklabel", datestr(FechasPlot(1:Intervalo:end), "HH"))	%Colocamos las tickmarks y sus etiquetas.
xlblE = xlabel( "Tiempo","fontname", "DejaVuSans-Bold.ttf", "fontsize", 18);
set(axE, "yaxislocation", "right")
set(axE, "linewidth", 1)
set(axE, "fontname", "DejaVuSans-Bold.ttf", "fontsize", 16)
ylblE = ylabel( "Ozono (ppb)","fontname", "DejaVuSans-Bold.ttf", "fontsize", 18, "color", [0,1,0]);

