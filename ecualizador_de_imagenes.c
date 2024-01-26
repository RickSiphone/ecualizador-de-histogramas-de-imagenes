#include <stdio.h>//Permite realizar operaciones básicas
#include <stdlib.h>//Esta libreria la requiere STB para poder utilizar sus funciones
#include <omp.h>//Permite programar en paralelo para trabajar con hilos
#include <math.h>//Se utiliza también en la libreria STB
#include <string.h>//Se utiliza para trabajar con cadenas
#define STB_IMAGE_IMPLEMENTATION//Estas líneas son la sintaxis para utilizar la librería STB
#include "stb-master/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb-master/stb_image_write.h"
#define MaxValue 256//Define la cantidad de colores con los que se trabaja
//Variables globales
int histograma[MaxValue];//Este arreglo almacena el histograma de la imagen cargada
int histograma_Nuevo[MaxValue];//Almacena el histograma de la nueva imagen ecualizada
int histo_CDF[MaxValue];//Este arreglo almacena el histograma acumulado
int eqCdf[MaxValue];//Es un arreglo que almacena el nuevo histograma
int canales,tamImagen,cdf_min,ancho, alto;//Almacenan la informacion de la imagen
unsigned char *imagen;//Se utilizan para cargar la imagen
//Prototipos de funciones
// Funciones Secuenciales
void generarHistograma(unsigned char *capa,int *histograma);
void generarCDF();
unsigned char* generarNuevoCDF();
// Funciones Paralelas
void generarHistograma_P(unsigned char *capa,int *histograma);
void generarCDF_P();
unsigned char* generarNuevoCDF_P();

int main(int argc, char *argv[]){
    //Si no esta el argumento de la ruta arroja el error
    if(argc < 2){
        printf("Error: Ingrese una ruta para poder abrir la imagen \n");
        return -1;
    }
    double t1,t2,tiempo_s,tiempo_p,speedup,cargar,generar,csv;//Miden los tiempos y se almacenan en otras variables
    char *ruta = argv[1],nombre[50];//Guarda la ruta ingresada por consola
    strcpy(nombre, ruta);//Se crea una copia del argumento ingresado por terminal para trabajar con el
    char *delimitadores = "/.";//Ayuda a indicar con que caracteres se va a dividir la cadena
    char *token = strtok(nombre, delimitadores); // Almacena la palabra del directorio
    if (strcmp(token, "Imagenes_para_ecualizar") != 0){ // Me indica si la imagen a ecualizar esta en el directorio
        printf("ERROR: Ingrese la imagen a la carpeta \"Imagenes_para_ecualizar\" y corrija la ruta\n");
        return -1;
    }
    t1 = omp_get_wtime();//Mide el tiempo antes de ejecutar la función
    //Se carga la imagen
    imagen = stbi_load(ruta,&ancho,&alto,&canales,0);
    t2 = omp_get_wtime();//Mide el tiempo después de ejecutar la función
    cargar = t2 - t1;//Guarda el tiempo final en cargar la imagen al programa
    if(imagen == NULL){//Si no existe la imagen, arroja el siguiente error
        printf("No se ha podido cargar la imagen %s  >:( \n\n",ruta);
        return -1;
    }
    char destino[70],destino_p[70],archivo1[70],archivo2[70];//Guardan los nombres finales de las imagenes
    //Se crean los nombres para las imagenes generadas
    token = strtok(NULL, delimitadores);//Se actualiza el token y carga la siguiente palabra hasta antes del delimitador
    strcpy(nombre, token); // Se almacena el nombre de la imagen en la variable nombre
    // Crea el nombre para el archivo que se genera
    char *formatoImagen = "_ecualizada_secuencial.jpg";
    char *formatoImagen_p = "_ecualizada_paralela.jpg";
    char ubicacion[70] = "Imagenes_ecualizadas/";
    char ubicacion_p[70] = "Imagenes_ecualizadas/";
    strcat(ubicacion, nombre);
    strcat(archivo1,ubicacion);//Almacena el nombre del directorio y de la imagen
    strcat(archivo2,ubicacion);//Almacena el nombre del directorio y de la imagen
    strcat(ubicacion, formatoImagen);
    strcpy(destino, ubicacion);//Nombre final del archivo que se va a crear para la imagen utilizando la forma secuencial
    strcat(ubicacion_p, nombre);
    strcat(ubicacion_p, formatoImagen_p);
    strcpy(destino_p, ubicacion_p);//Nombre final del archivo que se va a crear para la imagen utilizando la forma paralela
    strcat(archivo1,"_histo_secuencial.csv");//Se añade el resto del nombre y la extensión del archivo
    strcat(archivo2,"_histo_paralelo.csv");//Se añade el resto del nombre y la extensión del archivo
    unsigned char *eqImage;//Se utiliza para genera la nueva imagen ecualizada
    int temp;//Almacena los canales que tiene la imagen
    printf("\nLa imagen se cargo con exito\n\n");
    tamImagen = alto * ancho;
    printf("Sus dimensiones son: %d x %d pixeles\nCanales: %d\tTamaño: %d pixeles\n",ancho,alto,canales,tamImagen);
    //Forma secuencial ****************************************************************************************************
    t1 = omp_get_wtime();//Mide el tiempo antes de ejecutar la función
    generarHistograma(imagen,histograma);
    generarCDF();
    eqImage = generarNuevoCDF();
    temp = canales;//Guarda temporalmente el valor de los canales
    canales = 1;//Se asigna a un canal para que cree el histograma y el archivo csv utilizando un canal
    generarHistograma(eqImage,histograma_Nuevo);
    t2 = omp_get_wtime();//Mide el tiempo después de ejecutar la función
    tiempo_s = t2 - t1;//Guarda el tiempo de todo el proceso que se hizo de forma secuencial
    t1 = omp_get_wtime();//Mide el tiempo antes de ejecutar la función
    stbi_write_jpg(destino,ancho,alto,canales,eqImage,100);
    t2 = omp_get_wtime();//Mide el tiempo después de ejecutar la función
    generar = t2 - t1;//Guarda el tiempo en que tarda en generar la nueva imagen
    canales = temp;//Se regresa a la cantidad original de canales que tenia la imagen
    stbi_image_free(imagen);//Se libera la memoria de la imagen y la nueva imagen
    stbi_image_free(eqImage);
    //Se crea el archivo CSV de la imagen creada de forma secuencial
    t1 = omp_get_wtime();//Mide el tiempo antes de ejecutar la función
    FILE *archivo = fopen(archivo1, "w+");//w+ significa que se pueden añadir cosas sin sobrescribir
    fprintf(archivo, "%s,%s,%s \n", "VALOR", "HISTOGRAMA", "HISTOGRAMA ECUALIZADO");//Se guarda primero la cabecera
    for (int i = 0; i < MaxValue; i++)
        fprintf(archivo, "%d,%d,%d \n", i, histograma[i], histograma_Nuevo[i]);//Guarda los dos histogramas
    fclose(archivo);//Se cierra el archivo
    t2 = omp_get_wtime();//Mide el tiempo después de ejecutar la función
    csv = t2 - t1; //Guarda el tiempo en que tarda en generar ese archivo
    //Forma paralela  ****************************************************************************************************
    imagen = stbi_load(ruta,&ancho,&alto,&canales,0);//Se abre nuevamente la imagen
    t1 = omp_get_wtime();//Mide el tiempo antes de ejecutar la función
    generarHistograma_P(imagen,histograma);
    generarCDF_P();
    eqImage = generarNuevoCDF_P();
    temp = canales;//Guarda temporalmente el valor de los canales
    canales = 1;//Se asigna a un canal para que cree el histograma y el archivo csv utilizando un canal
    generarHistograma(eqImage,histograma_Nuevo);
    t2 = omp_get_wtime();//Mide el tiempo después de ejecutar la función
    tiempo_p = t2 - t1;//Guarda el tiempo de todo el proceso que se hizo de forma paralela
    stbi_write_jpg(destino_p,ancho,alto,canales,eqImage,100);//Crea la nueva imagen
    stbi_image_free(eqImage);//Libera la memoria de la imagen y la nueva imagen
    stbi_image_free(imagen);
    canales = temp;//Se regresa a la cantidad original de canales que tenia la imagen
    archivo = fopen(archivo2, "w+");//Se abre otro archivo para guardar los histogramas
    fprintf(archivo, "%s,%s,%s \n", "VALOR", "HISTOGRAMA", "HISTOGRAMA ECUALIZADO");//Guarda la cabecera
    for (int i = 0; i < MaxValue; i++)
        fprintf(archivo, "%d,%d,%d \n", i, histograma[i], histograma_Nuevo[i]);//Guarda los histogramas de las imagenes
    fclose(archivo);//Se cierra el archivo
    printf("\nImagen generada con exito \n");
    //Se muestra información extra en pantalla
    int numT = omp_get_num_procs();
    speedup = tiempo_s/tiempo_p;
    printf("\nTiempos de ejecucion: \n");
    printf("  Secuencial: %f [s]\n  Paralelo:   %f [s]\n",tiempo_s,tiempo_p);
    printf("\nTiene %d procesadores\n",numT);
    printf("Speedup: %lf\n",speedup);
    printf("Eficiencia: %lf\n",speedup/numT);
    printf("Overhead: %lf\n",tiempo_p-(tiempo_s/numT));
    printf("Tiempo de carga de imagen: %f [s]\n",cargar);
    printf("Tiempo de generacion de imagen: %f [s]\n",generar);
    printf("Tiempo de generacion de archivos CSV: %f [s]\n",csv);
    return 0;
}

void generarHistograma(unsigned char *capa,int *histograma){
    int i;
    for(i = 0; i < MaxValue; i++)//Se inicializa el arreglo en ceros
        histograma[i] = 0;
    if(canales == 1){//Si la imagen es de un canal
        for(i = 0; i < tamImagen; i++)
            histograma[capa[i]]++;//Aumenta el contador de cada valor dependiendo el pixel que sea
    }
    if(canales == 3){//Si es de 3 canales
        for(i = 0; i < tamImagen; i++)
            histograma[capa[i*canales]]++;//Se utiliza solo la capa roja para generar el histograma
    }
}
void generarCDF(){
    int i;
    for(i = 0; i < MaxValue; i++) //Se inicializa el arreglo en ceros
        histo_CDF[i] = 0;
    histo_CDF[0] = histograma[0];//El histograma acumulado guarda el primer valor del histograma
    for(i = 1; i < MaxValue; i++)//Se genera el histograma acumulado
        histo_CDF[i] = histograma[i] + histo_CDF[i-1];
    cdf_min = histo_CDF[0];//Para encontrar el valor mínimo se asigna el primer valor del acumulado
    //Se encuentra el valor minimo
    if (cdf_min == 0){//Si su valor es cero
        i = 1;//Empieza a buscar desde el segundo elemeno
        while (histo_CDF[i] == 0){//Cuando el elemento sea igual a cero
            i++;//recorre el arreglo
        }
        cdf_min = histo_CDF[i];//Como los valores van crecientes entonces el primer valor encontrado es el mas pequeño
    }
}
unsigned char* generarNuevoCDF(){
    int i;
    unsigned char *nuevo =  malloc(tamImagen);
    double divisor = (double)tamImagen - (double)cdf_min;//Se utiliza una formula para obtener el nuevo histograma
    for( i = 0; i < MaxValue; i++){
        double resultado = ((double)histo_CDF[i] - (double)cdf_min)/divisor;
        eqCdf[i] = (int)round(resultado*(double)(MaxValue-2)) + 1;
    }//Luego de obtener el histograma de la nueva imagen
    if(canales == 1){
        for(i = 0 ; i < tamImagen; i++)
            nuevo[i] = eqCdf[imagen[i]];//Se "sobrescribe el valor de cada pixel dependiendo su valor"
    }
    if(canales == 3){
        for(i = 0 ; i < tamImagen; i++)
            nuevo[i] = eqCdf[imagen[i*canales]];//Se "sobrescribe el valor de cada pixel dependiendo su valor"
    }
    return nuevo;
}

void generarHistograma_P(unsigned char *capa,int *histograma){
    int i, temp[MaxValue];
    #pragma omp parallel private(i)
    {
        #pragma omp for //Se inicializan los arreglos en ceros
        for(i = 0; i < MaxValue; i++){
            temp[i] = 0;
            histograma[i] = 0;
        }
        if(canales == 1){
            #pragma omp for reduction(+:temp)
            for(i = 0; i < tamImagen; i++)
                temp[capa[i]]++;//Se crea el histograma de la imagen
        }
        if(canales == 3){
            #pragma omp for reduction(+:temp)
            for(i = 0; i < tamImagen; i++)
                temp[capa[i*canales]]++;//Se crea el histograma de la imagen utilizando la capa roja (R)
        }
        #pragma omp for
        for(i = 0; i < MaxValue; i++)
            histograma[i] = temp[i];
    }
}
void generarCDF_P(){
    int i;
    #pragma omp parallel private(i)
    {
        #pragma omp for
        for(i = 0; i < MaxValue; i++) //Se inicializa el arreglo en ceros
            histo_CDF[i] = 0;
        #pragma omp barrier //Se espera a que terminen todos los hilos
        histo_CDF[0] = histograma[0]; //Se asigna el primer valor del histograma al acumulado
        #pragma omp single //Un solo hilo hace la operación para evitar condición de carrera
        { // y tambien porque unos datos dependen de otros
            for(i = 1; i < MaxValue; i++)
                histo_CDF[i] = histograma[i] + histo_CDF[i-1];
        }
        cdf_min = histo_CDF[0];//Se supone que  el primer valor del acumulado es el más pequeño
        if (cdf_min == 0){//Si es igual a cero
            i = 1;
            #pragma omp single 
            {
                while (histo_CDF[i] == 0)
                    i++;
                cdf_min = histo_CDF[i];
            }
        }
    }
}
unsigned char* generarNuevoCDF_P(){
    int i;
    unsigned char *nuevo =  malloc(tamImagen);
    double divisor = (double)tamImagen - (double)cdf_min; //Se usa una fórmula para calcular el nuevo histograma
    #pragma omp parallel private(i)
    {
        #pragma omp for //Se inicializa el arreglo en ceros
        for(i = 0; i < MaxValue; i++)
            eqCdf[i] = 0;
        #pragma omp for reduction(+:eqCdf)
        for(i = 0; i < MaxValue; i++){
            double resultado = ((double)histo_CDF[i] - (double)cdf_min)/divisor;
            eqCdf[i] = (int)round(resultado*(double)(MaxValue-2)) + 1;
        }
        if(canales == 1){
            #pragma omp for
            for(i = 0 ; i < tamImagen; i++)
                nuevo[i] = eqCdf[imagen[i]];//Se asignan los nuevos valores a cada pixel
        }
        if(canales == 3){
            #pragma omp for
            for(i = 0 ; i < tamImagen; i++)
                nuevo[i] = eqCdf[imagen[i*canales]];//Se asignan los nuevos valores a cada pixel
        }
    }
    return nuevo;
}